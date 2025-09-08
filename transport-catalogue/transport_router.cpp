#include "transport_router.h"

using namespace transport_catalogue;
using namespace interface;

namespace graph {
    TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue,
        const interface::RouteList& routes, const int bus_wait_time, const double bus_velocity)
        : db_(catalogue), bus_wait_time_(bus_wait_time), bus_velocity_(bus_velocity), router_(std::nullopt) {

        BuildRouter(routes);
    }

    void TransportRouter::FillStopIdAndVertexCount(const RouteList& routes) {
        stop_to_vertexes_.clear();
        id_to_stop_.clear();

        size_t next_vertex = 0;
        for (const auto& [bus_name, route] : routes) {
            for (std::string_view stop_name : route.first) {
                if (!stop_to_vertexes_.contains(stop_name)) {
                    VertexId wait = next_vertex++;
                    VertexId ready = next_vertex++;
                    stop_to_vertexes_[stop_name] = { wait, ready };

                    id_to_stop_[wait] = stop_name;
                    id_to_stop_[ready] = stop_name;
                }
            }
        }

        total_vertex_count_ = next_vertex;
    }

    void TransportRouter::BuildRouter(const interface::RouteList& routes) {
        FillStopIdAndVertexCount(routes);

        graph_ = graph::DirectedWeightedGraph<double>(total_vertex_count_);

        // Сбрасываем metadata
        edges_info_.clear();

        // Конвертация скорости: km/h -> meters per minute
        const double v_m_per_min = bus_velocity_ * 1000.0 / 60.0;

        for (const auto& kv : stop_to_vertexes_) {
            const std::string_view stop_name = kv.first;
            const StopVertexes verts = kv.second;

            graph::Edge<double> wait_edge{
                verts.wait,
                verts.ready,
                static_cast<double>(bus_wait_time_) // минуты
            };
            // Добавляем ребро в граф, затем сохраняем метаданные по индексу EdgeId
            graph_.AddEdge(wait_edge);
            edges_info_.push_back(RouteItem{
                EdgeType::WAIT,
                stop_name, // event_name для WAIT = имя остановки
                0u,
                static_cast<double>(bus_wait_time_)
                });
        }

        for (const auto& [bus_name_key, _] : routes) {
            const auto bus_ptr = db_.GetBus(bus_name_key);

            const std::string_view bus_name_view = bus_ptr->name;
            const auto& route_full = bus_ptr->route;
            const size_t n = route_full.size();
            if (n < 2) continue;

            // префиксные расстояния в метрах: pref_dist[k] = суммарная дистанция от 0 до k (0-based),
            // pref_dist[0] = 0, pref_dist[1] = dist(0->1), pref_dist[2] = dist(0->1)+dist(1->2), ...
            std::vector<int> pref_dist(n, 0);
            for (size_t i = 0; i + 1 < n; ++i) {
                const std::string_view a = route_full[i]->name;
                const std::string_view b = route_full[i + 1]->name;
                const int d = db_.GetDistance(a, b); // метры
                pref_dist[i + 1] = pref_dist[i] + d;
            }

            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    const int distance_m = pref_dist[j] - pref_dist[i]; // метры
                    const double ride_time_min = static_cast<double>(distance_m) / v_m_per_min;

                    const graph::VertexId from_v = stop_to_vertexes_.at(route_full[i]->name).ready;
                    const graph::VertexId to_v = stop_to_vertexes_.at(route_full[j]->name).wait;

                    graph::Edge<double> bus_edge{ from_v, to_v, ride_time_min };
                    graph_.AddEdge(bus_edge);
                    edges_info_.push_back(RouteItem{
                        EdgeType::BUS,
                        bus_name_view,   // event_name
                        j - i,           // span_count
                        ride_time_min
                        });
                }
            }
        }
        router_.emplace(graph_);
    }

    std::optional<RouteInfoSpecified> TransportRouter::GetRoute(std::string_view from, std::string_view to) const {
        if (!router_) {
            throw std::logic_error("Router is uninitialized");
        }

        auto it_from = stop_to_vertexes_.find(from);
        auto it_to = stop_to_vertexes_.find(to);
        if (it_from == stop_to_vertexes_.end() || it_to == stop_to_vertexes_.end()) {
            return std::nullopt;
        }

        const graph::VertexId from_v = it_from->second.wait;
        const graph::VertexId to_v = it_to->second.wait;

        const auto route_opt = router_->BuildRoute(from_v, to_v);
        if (!route_opt) {
            return std::nullopt;
        }

        const auto& r = *route_opt;
        RouteInfoSpecified result;
        result.total_time = r.weight;

        result.items.reserve(r.edges.size());
        for (graph::EdgeId eid : r.edges) {
            // берём сохранённый RouteItem (metadata) по EdgeId
            const RouteItem& ri = edges_info_.at(eid);
            result.items.push_back(ri);
        }

        return result;
    }
} // graph