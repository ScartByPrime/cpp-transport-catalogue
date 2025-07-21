#pragma once

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */
#include "geo.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <unordered_map>

namespace transport_catalogue {

	struct Stop {
		std::string name;
		detail::Coordinates coordinates;
	};
	using StopPtr = const Stop*;

	struct Bus {
		std::string name;
		std::vector<StopPtr> route;
	};
	using BusPtr = const Bus*;

	struct BusPtrLexicComparator {
		bool operator()(BusPtr lhs, BusPtr rhs) const {
			return lhs->name < rhs->name;
		}
	};

	struct StopPtrLexicComparator {
		bool operator()(StopPtr lhs, StopPtr rhs) const {
			return lhs->name < rhs->name;
		}
	};

	// структура для аккумуляции данных о конкретном маршруте
	struct RouteStatistics {
		int route_size = 0;
		int unique_stops = 0;
		double route_distance = 0.0;
		double curvature = 0.0;
	};
} // transport_catalogue

namespace interface {
	using IsRoundtrip = bool;
	using RouteList = std::map<std::string_view, std::pair<std::vector<std::string_view>, IsRoundtrip>>;
	using RouteListPtr = std::map<transport_catalogue::BusPtr, std::pair<std::vector<transport_catalogue::StopPtr>, IsRoundtrip>,
	transport_catalogue::BusPtrLexicComparator>;

	struct CommandDescription {
		// Определяет, задана ли команда (поле command непустое)
		explicit operator bool() const {
			return !command.empty();
		}

		bool operator!() const {
			return !operator bool();
		}

		std::string command;      // Название команды
		std::string id;           // id маршрута или остановки
		std::string description;  // Параметры команды
	};
} // interface