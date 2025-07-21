#include "json_reader.h"

using namespace transport_catalogue;
using namespace json;
using namespace interface;


void JsonReader::Read(const Document& doc) {
	Node root = doc.GetRoot();
	if (!root.IsMap()) {
		throw std::invalid_argument("Unexpected json format");
	}
	for (const auto& request_type : root.AsMap()) {
		if (request_type.first == "base_requests") {
			base_requests_ = Document(request_type.second);
		}
		else if (request_type.first == "stat_requests") {
			stat_requests_ = Document(request_type.second);
		}
		else if (request_type.first == "render_settings") {
			render_settings_ = Document(request_type.second);
		}
		else throw std::invalid_argument("Unexpected json format");
	}

}

JsonReader::JsonReader(const Document& doc) {
	Read(doc);
}

void JsonReader::ApplyBuffer(BufferRelatedStops& buffer_related_stops, const BufferBuses& buffer_buses, TransportCatalogue& catalogue) {
	// Дополняем буфер ближайших остановок недостающими отношениями остановок друг с другом
	if (!buffer_related_stops.empty()) {
		// Работаем сначала с копией буфера, дополняя оригинал, чтобы не изменять контейнер во время итерации по нему
		BufferRelatedStops copy = buffer_related_stops;
		for (const auto& [stop, related_stops] : copy) {
			for (const auto& [related_stop, distance] : related_stops) {
				buffer_related_stops[related_stop].insert({ stop, distance });
			}
		}
		// Основной этап дополнения оригинала
		for (const auto& [stop, related_stops] : buffer_related_stops) {
			for (const auto& [related_stop, distance] : related_stops) {
				if (!buffer_related_stops.at(related_stop).contains(stop)) {
					buffer_related_stops[related_stop].insert({ stop, distance });
				}
				else continue;
			}
		}
		// Реализуем буфер
		for (const auto& [root_stop, related_stops] : buffer_related_stops) {
			for (const auto& [related_stop, distance] : related_stops) {
				catalogue.ApplyRelatedStop(root_stop, related_stop, distance);
			}
		}
	}
	if (!buffer_buses.empty()) {
		// Передаем маршруты
		for (auto& [bus, stops] : buffer_buses) {
			catalogue.AddRoute(bus, stops);
		}
	}
}

void JsonReader::ApplyRequests(TransportCatalogue& catalogue) {
	BufferRelatedStops buffer_related_stops;
	BufferBuses buffer_buses;

	if (!base_requests_.GetRoot().IsArray()) {
		throw std::invalid_argument("Unexpected json format");
	}

	for (const auto& value : base_requests_.GetRoot().AsArray()) {
		if (!value.IsMap()) {
			throw std::invalid_argument("Unexpected json format");
		}

		if (value.AsMap().at("type") == "Bus") {
			std::string_view bus_name(value.AsMap().at("name").AsString());
			// Буферизация
			std::vector<std::string_view> stops;

			const auto& stops_array = value.AsMap().at("stops").AsArray();
			for (const auto& stop : stops_array) {
				stops.push_back(stop.AsString());
			}

			route_list_.insert({ bus_name, {stops, value.AsMap().at("is_roundtrip").AsBool()} });

			if (!value.AsMap().at("is_roundtrip").AsBool() && stops.size() > 1) {
				for (size_t i = stops.size() - 2; i != static_cast<size_t>(-1); --i) {
					stops.push_back(stops[i]);
					if (i == 0) break;
				}
			}
			buffer_buses.insert({ bus_name, stops });
		}

		else if (value.AsMap().at("type") == "Stop") {
			detail::Coordinates coords;
			coords.lat = value.AsMap().at("latitude").AsDouble();
			coords.lng = value.AsMap().at("longitude").AsDouble();
			catalogue.AddStop(value.AsMap().at("name").AsString(), coords);
			std::string_view weak_name = catalogue.GetStop(value.AsMap().at("name").AsString())->name;
			// Буферизация
			if (!value.AsMap().at("road_distances").AsMap().empty()) {
				for (const auto& [stop, distance] : value.AsMap().at("road_distances").AsMap()) {
					buffer_related_stops[weak_name].insert({ stop, distance.AsInt() });
				}
			}
		}
		else throw std::invalid_argument("Unexpected json format");
		}

	ApplyBuffer(buffer_related_stops, buffer_buses, catalogue);
}

json::Document JsonReader::GetResponse(const TransportCatalogue& catalogue, const svg::Document& map) const {

	Node stat = stat_requests_.GetRoot();
	Array result;

	if (!stat.IsArray()) {
		throw std::invalid_argument("Unexpected json format");
	}
	if (stat.AsArray().empty()) {
		return Document(result);
	}

	for (const auto& request : stat.AsArray()) {
		if (!request.IsMap()) {
			throw std::invalid_argument("Unexpected json format");
		}
		Dict element;

		if (request.AsMap().at("type").AsString() == "Bus") {
			if (!catalogue.GetBus(request.AsMap().at("name").AsString())) {
				element["error_message"] = "not found";
			}
			else {
				std::optional<RouteStatistics> statistics_opt = catalogue.GetRouteInfo(request.AsMap().at("name").AsString());
				if (!statistics_opt.has_value()) {
					throw std::logic_error("Statistics gathering fail");
				}
				const RouteStatistics& statistics = statistics_opt.value();
				element["curvature"] = statistics.curvature;

				element["route_length"] = statistics.route_distance;
				element["stop_count"] = statistics.route_size;
				element["unique_stop_count"] = statistics.unique_stops;
			}
		}
		else if (request.AsMap().at("type").AsString() == "Stop") {
			if (!catalogue.GetStop(request.AsMap().at("name").AsString())) {
				element["error_message"] = "not found";
			}
			else {
				std::set<std::string_view> buses = catalogue.GetBusesForStop(request.AsMap().at("name").AsString());
				Array val;
				for (const auto& bus : buses) {
					val.push_back(std::string(bus));
				}
				element["buses"] = std::move(val);
			}
		}
		else if (request.AsMap().at("type").AsString() == "Map") {
			std::ostringstream ostr;
			map.Render(ostr);
			element["map"] = ostr.str();
		}

		element["request_id"] = request.AsMap().at("id");
		result.push_back(std::move(element));
	}

	Document result_doc(result);
	return result_doc;
}

svg::RenderSettings JsonReader::GetRenderSettings() const {
	svg::RenderSettings result;
	Node root = render_settings_.GetRoot();

	if (!root.IsMap()) {
		throw std::invalid_argument("Unexpected json format");
	}
	if (root.AsMap().empty()) {
		return result;
	}
	try {
		const Dict& settings = root.AsMap();
		result.width = settings.at("width").AsDouble();
		result.height = settings.at("height").AsDouble();
		result.padding = settings.at("padding").AsDouble();
		if (result.padding >= std::min(result.width, result.height) / 2) {
			throw std::logic_error("Padding has incorrect value");
		}
		result.line_width = settings.at("line_width").AsDouble();
		result.stop_radius = settings.at("stop_radius").AsDouble();

		result.bus_label_font_size = settings.at("bus_label_font_size").AsInt();
		result.bus_label_offset = { settings.at("bus_label_offset").AsArray()[0].AsDouble(),
		settings.at("bus_label_offset").AsArray()[1].AsDouble() };

		result.stop_label_font_size = settings.at("stop_label_font_size").AsInt();
		result.stop_label_offset = { settings.at("stop_label_offset").AsArray()[0].AsDouble(),
		settings.at("stop_label_offset").AsArray()[1].AsDouble() };

		// Разбор цвета
		auto parse_color = [](const Node& color_node) -> svg::Color {
			if (color_node.IsString()) {
				return color_node.AsString();
			}
			else if (color_node.IsArray()) {
				const auto& arr = color_node.AsArray();
				if (arr.size() == 3) {
					return svg::Rgb{
						static_cast<uint8_t>(arr[0].AsInt()),
						static_cast<uint8_t>(arr[1].AsInt()),
						static_cast<uint8_t>(arr[2].AsInt())
					};
				}
				else if (arr.size() == 4) {
					return svg::Rgba{
						static_cast<uint8_t>(arr[0].AsInt()),
						static_cast<uint8_t>(arr[1].AsInt()),
						static_cast<uint8_t>(arr[2].AsInt()),
						arr[3].AsDouble()
					};
				}
			}
			throw std::invalid_argument("Invalid color format in render settings");
			};

		result.underlayer_color = parse_color(settings.at("underlayer_color"));

		result.underlayer_width = settings.at("underlayer_width").AsDouble();

		for (const auto& color : settings.at("color_palette").AsArray()) {
			result.color_palette.push_back(parse_color(color));
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Unexpected RenderSettings caused exception: " << e.what() << std::endl;
		std::abort();
	}
	return result;
}

const RouteList& JsonReader::GetRouteList() const {
	return route_list_;
}

