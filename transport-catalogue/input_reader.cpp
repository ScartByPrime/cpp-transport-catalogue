#include "input_reader.h"

#include <iterator>


using namespace transport_catalogue;
using namespace interface;

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота) и индекс запятой после координат
 */
std::pair<detail::Coordinates, size_t> ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return { { nan, nan }, str.npos };
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);
    auto comma2 = str.find(',', not_space2);

    const double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));

    if (comma2 == str.npos) {
        const double lng = std::stod(std::string(str.substr(not_space2)));
        return { { lat, lng }, str.npos };
    }

    const double lng = std::stod(std::string(str.substr(not_space2, comma2 - not_space2)));

    return { { lat, lng }, comma2 };
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

std::pair<detail::Coordinates, std::unordered_map<std::string_view, int>> ParseStopDescription(std::string_view line) {
    auto coords_and_index = ParseCoordinates(line);
    const detail::Coordinates coordinates = coords_and_index.first;
    std::unordered_map<std::string_view, int> stops_to_distance = {};

    if (coords_and_index.second != line.npos) {
    int distance = 0;
    std::string_view stop_name;
    auto related_stops_line = line.find_first_not_of(' ', coords_and_index.second + 1);
        for (auto& related_stop : Split(line.substr(related_stops_line), ',')) {
            if (std::isdigit(related_stop[0])) {
                auto distance_end = related_stop.find('m');
                distance = std::stoi(std::string(related_stop.substr(0, distance_end)));
                auto name_start = related_stop.find("to", distance_end + 1);
                stop_name = Trim(related_stop.substr(name_start + 2));
                stops_to_distance.insert({ stop_name, distance });
            }
            else assert(false);
        }
    }
    return { coordinates, stops_to_distance };
}
// Дополняет буфер ближайших остановок недостающими отношениями остановок друг с другом
void CompleteStopBuffer(std::unordered_map<std::string_view, std::unordered_map<std::string_view, int>>& buffer) {
    for (const auto& [stop, related_stops] : buffer) {
        for (const auto& [related_stop, distance] : related_stops) {
            if (!buffer.at(related_stop).contains(stop)) {
                buffer[related_stop].insert({stop, distance});
            }
            else continue;
        }
    }
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return { std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1)) };
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    std::unordered_map<std::string_view, std::unordered_map<std::string_view, int>> buffer_related_stops;
    std::unordered_map<std::string_view, std::vector<std::string_view>> buffer_buses;
    // Буферизуем маршруты, отправляем остановки с координатами, буферизуем список ближайших остановок,
    // пока catalogue не будет владеть всеми данными по остановкам, вносим отношения остановок
    for (const CommandDescription& input : commands_) {

        if (input.command[0] == 'S') {
            auto coords_and_related_stops = ParseStopDescription(input.description);
            const std::string_view weak_name = Trim(input.id);
            buffer_related_stops.insert({ weak_name, coords_and_related_stops.second });
            std::string strong_name = std::string(weak_name);
            catalogue.AddStop(std::move(strong_name), coords_and_related_stops.first);
        }

        else if (input.command[0] == 'B') {
            buffer_buses.insert({ Trim(input.id), ParseRoute(input.description) });
        }
        else continue;
    }
    // Дополняем и передаем буфер ближайших остановок для добавления в структуры
    assert(!buffer_related_stops.empty() && "Buffering error");
    CompleteStopBuffer(buffer_related_stops);
    catalogue.ApplyRelatedStops(buffer_related_stops);

    // Передаем маршруты
    for (auto& [bus, stops] : buffer_buses) {
        catalogue.AddRoute(bus, stops);
    }
}