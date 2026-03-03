#include "request_handler.h"

using namespace transport_catalogue;
using namespace json;
using namespace interface;

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db,
	const svg::MapRenderer& renderer, const interface::JsonReader& reader, const graph::TransportRouter& router)
: db_(db), reader_(reader), renderer_(renderer), router_(router) {
}

RouteListPtr RequestHandler::GetRouteListPtr() const {
	RouteListPtr result;
	const RouteList& route_list = reader_.GetRouteList();

	for (const auto& [route, stops_and_roundtrip] : route_list) {
		std::vector<StopPtr> stops_ptr;
		for (const auto& stop : stops_and_roundtrip.first) {
			stops_ptr.push_back(db_.GetStop(stop));
		}
		result[db_.GetBus(route)] = { stops_ptr, stops_and_roundtrip.second };
	}
	return result;
}

void RequestHandler::PrintResponse(std::ostream& out) const {
	svg::Document map = renderer_.RenderMap(GetRouteListPtr());
	json::Document result_json = reader_.GetResponse(db_, router_, map);
	Print(result_json, out);
}

svg::Document RequestHandler::RenderMap() const {
	return renderer_.RenderMap(GetRouteListPtr());
}