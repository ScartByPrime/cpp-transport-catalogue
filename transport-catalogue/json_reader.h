#pragma once

#include "json_builder.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace interface {
	using BufferRelatedStops = std::unordered_map<std::string_view, std::unordered_map<std::string_view, int>>;
	using BufferBuses = std::unordered_map<std::string_view, std::vector<std::string_view>>;

	struct Sections {
		json::Document base_requests;
		json::Document stat_requests;
		json::Document render_settings;
		json::Document routing_settings;
	};

	struct RouterStats {
		int bus_wait_time = 0;
		double bus_velocity = 0.0;
	};

	class JsonReader {
	public:
		JsonReader() = default;

		explicit JsonReader(const json::Document& doc);

		void Read(const json::Document& doc);

		json::Document GetResponse(const transport_catalogue::TransportCatalogue& catalogue,
			const graph::TransportRouter& router, const svg::Document& map) const;

		void ApplyRequests(transport_catalogue::TransportCatalogue& catalogue);

		svg::RenderSettings GetRenderSettings() const;

		const RouteList& GetRouteList() const;

		RouterStats GetRouterStats() const;

	private:
		void ApplyBuffer(BufferRelatedStops& buffer_related_stops, const BufferBuses& buffer_buses,
			transport_catalogue::TransportCatalogue& catalogue);

		Sections sections_;
		RouteList route_list_;
	};
} // Interface

