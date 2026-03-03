#include <iostream>
#include <fstream>
#include <cstdio>

#include "request_handler.h"


using namespace std;
using namespace transport_catalogue;
using namespace interface;
using namespace json;
using namespace graph;



int main() {
    if (!freopen("in.json", "r", stdin)) {
        cerr << "Cannot open in.json\n";
        return 1;
    }
    
    ofstream json_out("output/out.json");
    ofstream xml_out("output/out.xml");
    
    if (!json_out || !xml_out) {
        cerr << "Cannot open output files\n";
        return 1;
    }

    try {
        Document doc;
        doc = Load(cin);
        JsonReader reader(doc);
        TransportCatalogue catalogue;
        reader.ApplyRequests(catalogue);

        vector<detail::Coordinates> all_geo_coords;
        const RouteList& route_list = reader.GetRouteList();
        const svg::RenderSettings settings = reader.GetRenderSettings();

        for (const auto& [route, _] : route_list) {
            for (const StopPtr& stop : catalogue.GetBus(route)->route) {
                all_geo_coords.push_back(stop->coordinates);
            }
        }
        svg::SphereProjector projector(
            all_geo_coords.begin(), all_geo_coords.end(),
            settings.width, settings.height, settings.padding
        );

        const RouterStats stats = reader.GetRouterStats();
        TransportRouter router(catalogue, route_list, stats.bus_wait_time, stats.bus_velocity);

        svg::MapRenderer renderer(settings, projector);
        RequestHandler facade(catalogue, renderer, reader, router);
        facade.PrintResponse(json_out);
        svg::Document map = facade.RenderMap();
        map.Render(xml_out);
    }
    catch (const std::exception& e) {
        cerr << e.what() << endl;
        std::abort();
    }
}