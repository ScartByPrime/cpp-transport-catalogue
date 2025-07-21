#include "transport_catalogue.h"

#include <algorithm>
#include <iostream>

using namespace transport_catalogue;

void TransportCatalogue::ApplyRelatedStop(const std::string_view& stop, const std::string_view& related_stop, int distance) {
	if (!stopname_to_stop_.contains(stop)) {
		throw std::logic_error("ApplyRelatedStop fail");
	}
	StopPtr stop_ptr = stopname_to_stop_.at(stop);
	stop_to_related_stops_[stop_ptr].insert({ stopname_to_stop_.at(related_stop), distance });
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

std::optional<RouteStatistics> TransportCatalogue::GetRouteInfo(std::string_view bus_name) const {
	RouteStatistics result;
	auto iter = busname_to_bus_.find(bus_name);
	if (iter == busname_to_bus_.end()) {
		return std::nullopt;
	}

	BusPtr bus = iter->second;
	const std::vector<StopPtr> route = bus->route;
	result.route_size = static_cast<int>(route.size());

	std::unordered_set<StopPtr> unique_stops(route.begin(), route.end());
	result.unique_stops = static_cast<int>(unique_stops.size());

	double geo_distance = 0.0;
	double route_distance = 0.0;
	for (auto it = route.begin(); it != route.end() - 1; ++it) {
		geo_distance += detail::ComputeDistance((*it)->coordinates, (*next(it))->coordinates);
		if (!stop_to_related_stops_.contains(*it)) {
			throw std::logic_error("GetRouteInfo fail");
		}
		std::unordered_map<StopPtr, int> related_stops = stop_to_related_stops_.at(*it);

		if (!related_stops.contains(*next(it))) {
			std::cerr << "WARNING: " << (*it)->name << " has no related stop by name " << (*next(it))->name << '\n';
			throw std::logic_error("GetRouteInfo fail");
		}

		route_distance += related_stops.at(*next(it));
		}

	result.route_distance = route_distance;
	result.curvature =  route_distance / geo_distance;

	return std::optional<RouteStatistics>(result);
}

std::set<std::string_view> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
	std::set<std::string_view> result;
	if (!stopname_to_stop_.contains(stop_name)) {
		throw std::logic_error("GetBusesForStop fail");
	}
	StopPtr stop = stopname_to_stop_.at(stop_name);

	if (stop_to_routes_.contains(stop) && !stop_to_routes_.at(stop).empty()) {
		for (const auto* const bus : stop_to_routes_.at(stop)) {
			result.insert(bus->name);
		}
	}
	return result;
}