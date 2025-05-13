#pragma once

#include <stack>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "geo.h"

namespace transport_catalogue {
	//отношение остановок к маршрутам перемещено в поля класса TransportCatalogue
	struct Stop {
		std::string name;
		detail::Coordinates coordinates;
	};
	using StopPtr = const Stop*;
	// удален параметр unique_stops, применен алиас
	struct Bus {
		std::string name;
		std::vector<StopPtr> route;
	};
	using BusPtr = const Bus*;
	// структура для аккумуляции данных о конкретном маршруте
	struct RouteStatistics {
		size_t route_size = 0;
		size_t unique_stops = 0;
		double route_distance = 0.0;
	};

	class TransportCatalogue {
	public:
		/*
		Правильно ли я понимаю, что замечания направлены на улучшение универсальности и интуитивности интерфейса?
		Я попытался добиться большей универсальности за счет использования forwarding reference, сохранив производительность методов.
		Теперь база способна как принимать владение обработанными интерфейсом данными, избегая копирования строк, так и создавать их самостоятельно
		*/ 

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
				result.route.push_back(stopname_to_stop_.at(stop));
			}

			buses_.push_back(std::move(result));
			busname_to_bus_.insert({ buses_.back().name, &buses_.back() });

			for (std::string_view stop : stops) {
				stop_to_routes_[stopname_to_stop_.at(stop)].insert(&buses_.back());
			}
		}

		StopPtr GetStop(std::string_view stop_name) const;

		BusPtr GetBus(std::string_view bus_name) const;

		RouteStatistics GetRouteInfo(std::string_view bus_name) const;

		std::vector<std::string_view> GetBusesForStop(std::string_view stop_name) const;

		

	private:
		// остановки
		std::deque<Stop> stops_;
		// хеш-таблица для ускорения поиска остановки
		std::unordered_map<std::string_view, StopPtr> stopname_to_stop_;
		std::deque<Bus> buses_;
		// хеш-таблица для ускорения поиска автобуса
		std::unordered_map<std::string_view, BusPtr> busname_to_bus_;
		// хеш-таблица отношения остановок к маршрутам
		std::unordered_map<StopPtr, std::unordered_set<BusPtr>> stop_to_routes_;
	};
}