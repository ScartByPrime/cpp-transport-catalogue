#pragma once

#include "router.h"
#include "transport_catalogue.h"


namespace graph {
	enum class EdgeType {
		WAIT,
		BUS
	};

	struct RouteItem {
		EdgeType type;
		std::string_view event_name;
		size_t span_count = 0;
		double time = 0.0;
	};


	struct RouteInfoSpecified {
		std::vector<RouteItem> items;
		double total_time = 0.0;
	};

	class TransportRouter {
	public:
		explicit TransportRouter(const transport_catalogue::TransportCatalogue& catalogue,
			const interface::RouteList& routes, const int bus_wait_time, const double bus_velocity);

		std::optional<RouteInfoSpecified> GetRoute(std::string_view from, std::string_view to) const;

	private:
		// Две вершины на остановку
		struct StopVertexes {
			VertexId wait;
			VertexId ready;
		};

		void FillStopIdAndVertexCount(const interface::RouteList& routes);
		void BuildRouter(const interface::RouteList& routes);

		const transport_catalogue::TransportCatalogue& db_;
		const int bus_wait_time_;
		const double bus_velocity_;
		size_t total_vertex_count_ = 0;

		std::unordered_map<std::string_view, StopVertexes> stop_to_vertexes_;
		std::unordered_map<graph::VertexId, std::string_view> id_to_stop_;
		std::vector<RouteItem> edges_info_; // индекс = EdgeId

		DirectedWeightedGraph<double> graph_;
		std::optional<Router<double>> router_;
	};
} // graph

