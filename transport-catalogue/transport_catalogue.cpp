#include "transport_catalogue.h"
#include "input_reader.h"

#include <algorithm>

using namespace transport_catalogue;

void TransportCatalogue::AddStop(Stop&& input) {
	stops_.push_back(std::move(input));
	stopname_to_stop_.insert({ stops_.back().name, &stops_.back() });
}

void TransportCatalogue::AddRoutes(std::unordered_map<std::string_view, std::vector<std::string_view>> && buffer) {
	for (auto& [bus, stops] : buffer) {
		Bus result;
		result.name = std::string(bus);
		std::unordered_set<std::string_view> unique_stops(stops.begin(), stops.end());
		result.unique_stops = static_cast<int>(unique_stops.size());

		for (std::string_view stop : stops) {
			result.route.push_back(stopname_to_stop_.at(stop));
		}

		buses_.push_back(std::move(result));
		busname_to_bus_.insert({ buses_.back().name, &buses_.back() });

		for (std::string_view stop : stops) {
			stopname_to_stop_.at(stop)->buses.insert(&buses_.back());
		}
	}
}

bool TransportCatalogue::CheckDaStop(std::string_view stop_name) const {
	if (stopname_to_stop_.count(stop_name)) {
		return true;
	}
	return false;
}

bool TransportCatalogue::CheckDaBus(std::string_view bus_name) const {
	if (busname_to_bus_.count(bus_name)) {
		return true;
	}
	return false;
}

size_t TransportCatalogue::GetRouteSize(std::string_view bus_name) const {
	auto it = busname_to_bus_.find(bus_name);
	if (it == busname_to_bus_.end()) {
		return 0;
	}

	Bus* bus = it->second;
	return bus->route.size();
}

int TransportCatalogue::GetUniqueStopsCount(std::string_view bus_name) const {
	auto it = busname_to_bus_.find(bus_name);
	if (it == busname_to_bus_.end()) {
		return 0;
	}

	Bus* bus = it->second;
	return bus->unique_stops;
}

std::vector<std::string_view> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
	std::vector<std::string_view> result;
	if (!stopname_to_stop_.at(stop_name)->buses.empty()) {
		for (const auto bus : stopname_to_stop_.at(stop_name)->buses) {
			result.push_back(bus->name);
		}
		std::sort(result.begin(), result.end());
	}
	return result;
}

double TransportCatalogue::CalculateRouteDistance(std::string_view bus_name) const {
	auto bus_it = busname_to_bus_.find(bus_name);
	if (bus_it == busname_to_bus_.end()) {
		return 0;	
	}

	Bus* bus = bus_it->second;

	double result = 0.0;
	for (auto it = bus->route.begin(); it != bus->route.end() - 1; ++it) {
		result += detail::ComputeDistance((*it)->coordinates, (*next(it))->coordinates);
	}
	return result;
}