#include <iostream>
#include <iomanip>

#include "stat_reader.h"
#include "transport_catalogue.h"

void interface::ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request,
    std::ostream& output) {
    size_t begin = request.find_first_not_of(' ');
    size_t end = request.find_last_not_of(' ') + 1;
    std::string_view command = request.substr(begin, end - begin);

    if (command[0] == 'B') {
        begin = command.find_first_of(' ');
        std::string_view id = command.substr(command.find_first_not_of(' ', begin), end - begin);

        if (transport_catalogue.CheckDaBus(id)) {
            output << "Bus " << id << ": " << transport_catalogue.GetRouteSize(id) << " stops on route, "
                << transport_catalogue.GetUniqueStopsCount(id) << " unique stops, " 
                << std::setprecision(6) << transport_catalogue.CalculateRouteDistance(id)
                << " route length" << '\n';
        }
        else {
            output << "Bus " << id << ": not found" << '\n';
        }
    }
    else if (command[0] == 'S') {
        begin = command.find_first_of(' ');
        std::string_view id = command.substr(command.find_first_not_of(' ', begin), end - begin);

        if (transport_catalogue.CheckDaStop(id)) {
            output << "Stop " << id << ": ";
            std::vector<std::string_view> buses = transport_catalogue.GetBusesForStop(id);
            if (!buses.empty()) {
                output << "buses ";
                auto it = buses.begin();
                auto last = std::prev(buses.end());
                for (; it != last; ++it) {
                    output << *it << ' ';
                }
                output << *last << '\n';
            }
            else {
                output << "no buses" << '\n';
                }
        }
        else {
            output << "Stop " << id << ": not found" << '\n';
        }
    }
}