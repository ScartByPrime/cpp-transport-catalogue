#include "transport_catalogue.h"
#include "input_reader.h"

#include <algorithm>

using namespace transport_catalogue;

StopPtr TransportCatalogue::GetStop(std::string_view stop_name) const {
	if (stopname_to_stop_.count(stop_name)) {
		return stopname_to_stop_.at(stop_name);
	}
	return nullptr;
}

BusPtr TransportCatalogue::GetBus(std::string_view bus_name) const {
	if (busname_to_bus_.count(bus_name)) {
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

	double distance = 0.0;
	for (auto it = route.begin(); it != route.end() - 1; ++it) {
		distance += detail::ComputeDistance((*it)->coordinates, (*next(it))->coordinates);
	}
	result.route_distance = distance;

	return result;
}

std::vector<std::string_view> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
	std::vector<std::string_view> result;
	StopPtr stop = stopname_to_stop_.at(stop_name);
	if (stop_to_routes_.count(stop) && !stop_to_routes_.at(stop).empty()) {
		for (const auto bus : stop_to_routes_.at(stop)) {
			result.push_back(bus->name);
		}
		std::sort(result.begin(), result.end());
	}
	return result;
}