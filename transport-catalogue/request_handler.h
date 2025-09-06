#pragma once

#include "json_reader.h"
#include "map_renderer.h"

namespace interface {
    class RequestHandler {
    public:

        RequestHandler(const transport_catalogue::TransportCatalogue& db,
            const svg::MapRenderer& renderer,
            const interface::JsonReader& reader,
            const graph::TransportRouter& router);

        void PrintResponse(std::ostream& out) const;

        svg::Document RenderMap() const;

    private:
        RouteListPtr GetRouteListPtr() const;

        const transport_catalogue::TransportCatalogue& db_;
        const interface::JsonReader& reader_;
        const svg::MapRenderer& renderer_;
        const graph::TransportRouter& router_;
    };
}