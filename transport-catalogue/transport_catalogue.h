#pragma once

#include <stack>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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
		void AddStop(std::string&& name, detail::Coordinates coordinates);

		void AddRoutes(std::unordered_map<std::string_view, std::vector<std::string_view>>&& buffer);

		StopPtr CheckDaStop(std::string_view stop_name) const;

		BusPtr CheckDaBus(std::string_view bus_name) const;

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