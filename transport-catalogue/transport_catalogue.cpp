#include "transport_catalogue.h"
#include "input_reader.h"

#include <algorithm>

using namespace transport_catalogue;

void TransportCatalogue::AddStop(std::string&& name, detail::Coordinates coordinates) {
	Stop result;
	result.name = std::move(name);
	result.coordinates = coordinates;
	stops_.push_back(std::move(result));
	stopname_to_stop_.insert({ stops_.back().name, &stops_.back() });
}

void TransportCatalogue::AddRoutes(std::unordered_map<std::string_view, std::vector<std::string_view>> && buffer) {
	for (auto& [bus, stops] : buffer) {
		Bus result;
		result.name = std::string(bus);

		for (std::string_view stop : stops) {
			result.route.push_back(stopname_to_stop_.at(stop));
		}

		buses_.push_back(std::move(result));
		busname_to_bus_.insert({ buses_.back().name, &buses_.back() });

		for (std::string_view stop : stops) {
			stop_to_routes_[stopname_to_stop_.at(stop)].insert(&buses_.back());
		}
	}
}

StopPtr TransportCatalogue::CheckDaStop(std::string_view stop_name) const {
	if (stopname_to_stop_.count(stop_name)) {
		return stopname_to_stop_.at(stop_name);
	}
	return nullptr;
}

BusPtr TransportCatalogue::CheckDaBus(std::string_view bus_name) const {
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