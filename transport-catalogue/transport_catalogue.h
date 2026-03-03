#pragma once

#include "domain.h"

#include <stdexcept>
#include <stack>
#include <set>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <optional>


namespace transport_catalogue {
	class TransportCatalogue {
	public:

		template <typename StringType>
		void AddStop(StringType&& name, detail::Coordinates coordinates) {
			Stop result;
			result.name = std::forward<StringType>(name);
			result.coordinates = coordinates;
			stops_.push_back(std::move(result));
			stopname_to_stop_.insert({ stops_.back().name, &stops_.back() });
		}

		template <typename StringType>
		void AddRoute(StringType&& route, const std::vector<std::string_view>& stops) {
			Bus result;
			result.name = std::forward<StringType>(route);

			for (std::string_view stop : stops) {
				if (!stopname_to_stop_.contains(stop)) {
					throw std::logic_error("AddRoute fail");
				}
				result.route.push_back(stopname_to_stop_.at(stop));
			}

			buses_.push_back(std::move(result));
			busname_to_bus_.insert({ buses_.back().name, &buses_.back() });

			for (std::string_view stop : stops) {
				if (!stopname_to_stop_.contains(stop)) {
					throw std::logic_error("AddRoute fail");
				}
				stop_to_routes_[stopname_to_stop_.at(stop)].insert(&buses_.back());
			}
		}

		void ApplyRelatedStop(const std::string_view& stop, const std::string_view& related_stop, int distance);

		StopPtr GetStop(std::string_view stop_name) const;

		BusPtr GetBus(std::string_view bus_name) const;

		std::optional<RouteStatistics> GetRouteInfo(std::string_view bus_name) const;

		std::set<std::string_view> GetBusesForStop(std::string_view stop_name) const;

		int GetDistance(std::string_view stop_from, std::string_view stop_to) const;

	private:
		// остановки
		std::deque<Stop> stops_;
		// хеш-таблица для ускорения поиска остановки
		std::unordered_map<std::string_view, StopPtr> stopname_to_stop_;
		// хеш-таблица для быстрого поиска ближайших остановок
		std::unordered_map<StopPtr, std::unordered_map<StopPtr, int>> stop_to_related_stops_;
		std::deque<Bus> buses_;
		// хеш-таблица для ускорения поиска автобуса
		std::unordered_map<std::string_view, BusPtr> busname_to_bus_;
		// хеш-таблица отношения остановок к маршрутам
		std::unordered_map<StopPtr, std::unordered_set<BusPtr>> stop_to_routes_;
	};
}