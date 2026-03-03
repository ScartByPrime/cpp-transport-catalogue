#include "json_reader.h"

using namespace transport_catalogue;
using namespace json;
using namespace interface;
using namespace graph;


void JsonReader::Read(const Document& doc) {
	Node root = doc.GetRoot();
	if (!root.IsDict()) {
		throw std::invalid_argument("Unexpected json format");
	}
	for (const auto& request_type : root.AsDict()) {
		if (request_type.first == "base_requests") {
			sections_.base_requests = Document(request_type.second);
		}
		else if (request_type.first == "stat_requests") {
			sections_.stat_requests = Document(request_type.second);
		}
		else if (request_type.first == "render_settings") {
			sections_.render_settings = Document(request_type.second);
		}
		else if (request_type.first == "routing_settings") {
			sections_.routing_settings = Document(request_type.second);
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

RouterStats JsonReader::GetRouterStats() const {
	int bus_wait_time = sections_.routing_settings.GetRoot().AsDict().at("bus_wait_time").AsInt();
	double bus_velocity = sections_.routing_settings.GetRoot().AsDict().at("bus_velocity").AsDouble();
	return { bus_wait_time, bus_velocity };
}

void JsonReader::ApplyRequests(TransportCatalogue& catalogue) {
	BufferRelatedStops buffer_related_stops;
	BufferBuses buffer_buses;

	if (!sections_.base_requests.GetRoot().IsArray()) {
		throw std::invalid_argument("Unexpected json format");
	}

	for (const auto& value : sections_.base_requests.GetRoot().AsArray()) {
		if (!value.IsDict()) {
			throw std::invalid_argument("Unexpected json format");
		}

		if (value.AsDict().at("type") == "Bus") {
			std::string_view bus_name(value.AsDict().at("name").AsString());
			// Буферизация
			std::vector<std::string_view> stops;

			const auto& stops_array = value.AsDict().at("stops").AsArray();
			for (const auto& stop : stops_array) {
				stops.push_back(stop.AsString());
			}

			route_list_.insert({ bus_name, {stops, value.AsDict().at("is_roundtrip").AsBool()} });

			if (!value.AsDict().at("is_roundtrip").AsBool() && stops.size() > 1) {
				for (size_t i = stops.size() - 2; i != static_cast<size_t>(-1); --i) {
					stops.push_back(stops[i]);
					if (i == 0) break;
				}
			}
			buffer_buses.insert({ bus_name, stops });
		}

		else if (value.AsDict().at("type") == "Stop") {
			detail::Coordinates coords;
			coords.lat = value.AsDict().at("latitude").AsDouble();
			coords.lng = value.AsDict().at("longitude").AsDouble();
			catalogue.AddStop(value.AsDict().at("name").AsString(), coords);
			std::string_view weak_name = catalogue.GetStop(value.AsDict().at("name").AsString())->name;

			// Буферизация
			if (!value.AsDict().at("road_distances").AsDict().empty()) {
				for (const auto& [stop, distance] : value.AsDict().at("road_distances").AsDict()) {
					buffer_related_stops[weak_name].insert({ stop, distance.AsInt() });
				}
			}
		}
		else throw std::invalid_argument("Unexpected json format");
		}

	ApplyBuffer(buffer_related_stops, buffer_buses, catalogue);
}

json::Document JsonReader::GetResponse(const TransportCatalogue& catalogue,
	const TransportRouter& router, const svg::Document& map) const {

	const Node& stat = sections_.stat_requests.GetRoot();
	if (!stat.IsArray()) {
		throw std::invalid_argument("Unexpected json format");
	}

	json::Builder builder;
	auto& result_array = builder.StartArray();

	for (const auto& request : stat.AsArray()) {
		if (!request.IsDict()) {
			throw std::invalid_argument("Unexpected json format");
		}

		const auto& dict = request.AsDict();
		const std::string& type = dict.at("type").AsString();
		const int request_id = dict.at("id").AsInt();

		auto& dict_builder = result_array.StartDict();

		if (type == "Bus") {
			const auto* bus = catalogue.GetBus(dict.at("name").AsString());
			if (!bus) {
				dict_builder.Key("error_message").Value("not found");
			}
			else {
				std::optional<RouteStatistics> statistics_opt = catalogue.GetRouteInfo(dict.at("name").AsString());
				if (!statistics_opt.has_value()) {
					throw std::logic_error("Statistics gathering fail");
				}
				const auto& statistics = statistics_opt.value();
				dict_builder.Key("curvature").Value(statistics.curvature);
				dict_builder.Key("route_length").Value(statistics.route_distance);
				dict_builder.Key("stop_count").Value(statistics.route_size);
				dict_builder.Key("unique_stop_count").Value(statistics.unique_stops);
			}
		}
		else if (type == "Stop") {
			const auto* stop = catalogue.GetStop(dict.at("name").AsString());
			if (!stop) {
				dict_builder.Key("error_message").Value("not found");
			}
			else {
				std::set<std::string_view> buses = catalogue.GetBusesForStop(dict.at("name").AsString());
				auto& buses_array = dict_builder.Key("buses").StartArray();
				for (const auto& bus : buses) {
					buses_array.Value(std::string(bus));
				}
				buses_array.EndArray();
			}
		}
		else if (type == "Map") {
			std::ostringstream ostr;
			map.Render(ostr);
			dict_builder.Key("map").Value(ostr.str());
		}
		else if (type == "Route") {
			const std::string_view start(dict.at("from").AsString());
			const std::string_view finish(dict.at("to").AsString());

			const auto route_info_opt = router.GetRoute(start, finish);

			if (!route_info_opt.has_value()) {
				dict_builder.Key("error_message").Value("not found");
			}
			else {
				const auto route_info = route_info_opt.value();
				dict_builder.Key("total_time").Value(route_info.total_time);
				auto& items_array = dict_builder.Key("items").StartArray();
				for (const auto& item : route_info.items) {
					auto& item_dict = items_array.StartDict();
					if (item.type == EdgeType::WAIT) {
						item_dict.Key("type").Value("Wait");
						item_dict.Key("stop_name").Value(std::string(item.event_name));
					}
					else {
						item_dict.Key("type").Value("Bus");
						item_dict.Key("bus").Value(std::string(item.event_name));
						item_dict.Key("span_count").Value(static_cast<int>(item.span_count));
					}
					item_dict.Key("time").Value(item.time);
					item_dict.EndDict();
				}
				items_array.EndArray();
			}
		}

		dict_builder.Key("request_id").Value(request_id);
		dict_builder.EndDict();
	}

	result_array.EndArray();
	return json::Document(builder.Build());
}


svg::RenderSettings JsonReader::GetRenderSettings() const {
	svg::RenderSettings result;
	Node root = sections_.render_settings.GetRoot();

	if (!root.IsDict()) {
		throw std::invalid_argument("Unexpected json format");
	}
	if (root.AsDict().empty()) {
		return result;
	}
	try {
		const Dict& settings = root.AsDict();
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

