#pragma once

#include <stack>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "geo.h"

namespace transport_catalogue {

	struct Bus;

	struct Stop {
		std::string name;
		std::unordered_set<Bus*> buses;
		detail::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<Stop*> route;
		int unique_stops = 0;
	};

	class TransportCatalogue {
	public:
		void AddStop(Stop&& input);

		void AddRoutes(std::unordered_map<std::string_view, std::vector<std::string_view>>&& buffer);

		bool CheckDaStop(std::string_view stop_name) const;

		bool CheckDaBus(std::string_view bus_name) const;

		size_t GetRouteSize(std::string_view bus_name) const;

		int GetUniqueStopsCount(std::string_view bus_name) const;

		std::vector<std::string_view> GetBusesForStop(std::string_view stop_name) const;

		double CalculateRouteDistance(std::string_view bus_name) const;

	private:
		// остановки
		std::deque<Stop> stops_;
		// хеш-таблица для ускорения поиска остановки
		std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
		std::deque<Bus> buses_;
		// хеш-таблица для ускорения поиска автобуса
		std::unordered_map<std::string_view, Bus*> busname_to_bus_;
	};
}