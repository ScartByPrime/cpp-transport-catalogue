#include <iostream>
#include <iomanip>

#include "stat_reader.h"
#include "transport_catalogue.h"

void interface::ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request,
    std::ostream& output) {
    size_t begin = request.find_first_not_of(' ');
    size_t end = request.find_last_not_of(' ') + 1;
    std::string_view command = request.substr(begin, end - begin);

    if (command[0] == 'B') {
        begin = command.find_first_of(' ');
        std::string_view id = command.substr(command.find_first_not_of(' ', begin), end - begin);

        if (transport_catalogue.GetBus(id) != nullptr) {
            std::optional<transport_catalogue::RouteStatistics> stat = transport_catalogue.GetRouteInfo(id);
            if (!stat.has_value()) {
                throw std::logic_error("Statistics gathering fail");
            }
            output << "Bus " << id << ": " << stat.value().route_size << " stops on route, "
                << stat.value().unique_stops << " unique stops, " 
                << std::setprecision(6) << stat.value().route_distance
                << " route length, " << stat.value().curvature << " curvature" << '\n';
        }
        else {
            output << "Bus " << id << ": not found" << '\n';
        }
    }
    else if (command[0] == 'S') {
        begin = command.find_first_of(' ');
        std::string_view id = command.substr(command.find_first_not_of(' ', begin), end - begin);

        if (transport_catalogue.GetStop(id) != nullptr) {
            output << "Stop " << id << ": ";
            std::unordered_set<std::string_view> buses = transport_catalogue.GetBusesForStop(id);
            if (!buses.empty()) {
                output << "buses ";
                auto it = buses.begin();
                auto last = std::prev(buses.end());
                for (; it != last; ++it) {
                    output << *it << ' ';
                }
                output << *last << '\n';
            }
            else {
                output << "no buses" << '\n';
                }
        }
        else {
            output << "Stop " << id << ": not found" << '\n';
        }
    }
}