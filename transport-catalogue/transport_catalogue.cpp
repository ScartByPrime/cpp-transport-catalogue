#include "transport_catalogue.h"
#include "input_reader.h"

#include <algorithm>
#include <iostream>

using namespace transport_catalogue;

void TransportCatalogue::ApplyRelatedStops(const std::unordered_map<std::string_view, std::unordered_map<std::string_view, int>>& buffer_related_stops) {
    for (const auto& name_to_stops : buffer_related_stops) {
		assert((!stopname_to_stop_.empty() || stopname_to_stop_.contains(name_to_stops.first)) && "Ключи буфера не совпадают со списком остановок");
        StopPtr stop = stopname_to_stop_.at(name_to_stops.first);  
        std::unordered_map<StopPtr, int> related_stops_local;

        for (const auto& [stop_name, distance] : name_to_stops.second) {
			if (!stopname_to_stop_.contains(stop_name)) {
				std::cerr << std::string(stop_name) << " - Неизвестная остановка" << '\n';
			}
			assert(stopname_to_stop_.contains(stop_name) && "Значения буфера не совпадают со списком остановок");
            related_stops_local[stopname_to_stop_.at(stop_name)] = distance;  
        }

        stop_to_related_stops_[stop] = std::move(related_stops_local);
    }
}

StopPtr TransportCatalogue::GetStop(std::string_view stop_name) const {
	if (stopname_to_stop_.contains(stop_name)) {
		return stopname_to_stop_.at(stop_name);
	}
	return nullptr;
}

BusPtr TransportCatalogue::GetBus(std::string_view bus_name) const {
	if (busname_to_bus_.contains(bus_name)) {
		return busname_to_bus_.at(bus_name);
	}
	return nullptr;
}

RouteStatistics TransportCatalogue::GetRouteInfo(std::string_view bus_name) const {
	RouteStatistics result;
	auto iter = busname_to_bus_.find(bus_name);
	if (iter == busname_to_bus_.end()) {
		return result;
	}

	BusPtr bus = iter->second;
	const std::vector<StopPtr> route = bus->route;
	result.route_size = route.size();

	std::unordered_set<StopPtr> unique_stops(route.begin(), route.end());
	result.unique_stops = static_cast<int>(unique_stops.size());

	double geo_distance = 0.0;
	double route_distance = 0.0;
	for (auto it = route.begin(); it != route.end() - 1; ++it) {
		geo_distance += detail::ComputeDistance((*it)->coordinates, (*next(it))->coordinates);
		std::unordered_map<StopPtr, int> related_stops = stop_to_related_stops_.at(*it);

		if (!related_stops.contains(*next(it))) {
			std::cerr << "WARNING: " << (*it)->name << " has no related stop by name " << (*next(it))->name << '\n';
			continue;
		}

		route_distance += related_stops.at(*next(it));
		}

	result.route_distance = route_distance;
	result.curvature =  route_distance / geo_distance;

	return result;
}

std::vector<std::string_view> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
	std::vector<std::string_view> result;
	StopPtr stop = stopname_to_stop_.at(stop_name);

	if (stop_to_routes_.contains(stop) && !stop_to_routes_.at(stop).empty()) {
		for (const auto* const bus : stop_to_routes_.at(stop)) {
			result.push_back(bus->name);
		}
		std::sort(result.begin(), result.end());
	}
	return result;
}