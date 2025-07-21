#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

namespace interface {
	using BufferRelatedStops = std::unordered_map<std::string_view, std::unordered_map<std::string_view, int>>;
	using BufferBuses = std::unordered_map<std::string_view, std::vector<std::string_view>>;

	class JsonReader {
	public:
		JsonReader() = default;

		explicit JsonReader(const json::Document& doc);

		void Read(const json::Document& doc);

		json::Document GetResponse(const transport_catalogue::TransportCatalogue& catalogue, const svg::Document& map) const;

		void ApplyRequests(transport_catalogue::TransportCatalogue& catalogue);

		svg::RenderSettings GetRenderSettings() const;

		const interface::RouteList& GetRouteList() const;

	private:
		void ApplyBuffer(BufferRelatedStops& buffer_related_stops, const BufferBuses& buffer_buses,
			transport_catalogue::TransportCatalogue& catalogue);

		json::Document base_requests_;
		json::Document stat_requests_;
		json::Document render_settings_;
		interface::RouteList route_list_;
	};
}

