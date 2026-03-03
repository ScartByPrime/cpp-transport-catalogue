#include "map_renderer.h"

using namespace svg;
using namespace transport_catalogue;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

MapRenderer::MapRenderer(const RenderSettings& settings, SphereProjector projector)
    : settings_(settings), projector_(projector) {
}


Document MapRenderer::RenderMap(const interface::RouteListPtr& routes) const {
    Document result;

    if (settings_.color_palette.empty()) {
        throw std::logic_error("Color palette is empty");
    }

    std::vector<std::unique_ptr<Polyline>> routes_lines_buffer;
    std::vector<std::unique_ptr<Text>> routes_text_buffer;
    std::vector<std::unique_ptr<Circle>> stops_circles_buffer;

    int index_colors = 0;

    std::set<StopPtr, StopPtrLexicComparator> sorted_stops;

    for (const auto& [bus, stops_and_roundtrip] : routes) {
        std::vector<StopPtr> stops = stops_and_roundtrip.first;
        bool is_roundtrip = stops_and_roundtrip.second;

        if (index_colors == static_cast<int>(settings_.color_palette.size())) {
            index_colors = 0;
        }

        if (bus->route.size() <= 1) {
            continue;
        }

        std::vector<detail::Coordinates> geo_coords;
        for (StopPtr stop : bus->route) {
            geo_coords.push_back(stop->coordinates);
        }

        // Polyline
        auto poly = CreateRouteLine(geo_coords, index_colors);
        routes_lines_buffer.push_back(std::move(poly));

        // Route text
        auto route_name_text = CreateRouteNames(stops, bus->name, is_roundtrip, index_colors);
        routes_text_buffer.insert(routes_text_buffer.end(),
            std::make_move_iterator(route_name_text.begin()),
            std::make_move_iterator(route_name_text.end()));

        // Буферизуем остановки
        sorted_stops.insert(std::make_move_iterator(stops.begin()), std::make_move_iterator(stops.end()));
        ++index_colors;
    }


    // Stops circles
    auto stops_circles = CreateStopCircles(sorted_stops);

    // Stops names
    auto stops_names = CreateStopsNames(sorted_stops);

    // Собираем очередь на рендер
    std::vector<std::unique_ptr<Object>> render_queue;

    for (auto& line : routes_lines_buffer) {
        render_queue.push_back(std::move(line));
    }

    for (auto& text : routes_text_buffer) {
        render_queue.push_back(std::move(text));
    }

    for (auto& circle : stops_circles) {
        render_queue.push_back(std::move(circle));
    }

    for (auto& text : stops_names) {
        render_queue.push_back(std::move(text));
    }

    for (auto& obj : render_queue) {
        result.AddPtr(std::move(obj));
    }
    return result;
}

std::unique_ptr<svg::Polyline> MapRenderer::CreateRouteLine(const std::vector<detail::Coordinates>& coords, int index_colors) const {
    auto poly = std::make_unique<Polyline>();
    poly->SetStrokeWidth(settings_.line_width)
        .SetFillColor(NoneColor)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND)
        .SetStrokeColor(settings_.color_palette[index_colors]);

    for (const auto& geo_coord : coords) {
        const svg::Point screen_coord = projector_(geo_coord);
        poly->AddPoint(screen_coord);
    }
    return poly;
}

std::vector<std::unique_ptr<svg::Text>> MapRenderer::CreateRouteNames(const std::vector<StopPtr>& stops,
    const std::string& route_name, bool is_roundtrip, int index_colors) const {

    std::vector<std::unique_ptr<svg::Text>> result;

    std::unique_ptr txt_sub= std::make_unique<Text>();
    const svg::Point screen_coord = projector_(stops[0]->coordinates);

    txt_sub->SetPosition(screen_coord).SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size).SetFontFamily("Verdana")
        .SetFontWeight("bold").SetData(route_name);

    std::unique_ptr txt_main = std::make_unique<Text>(*txt_sub);

    txt_sub->SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color).SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND).SetStrokeWidth(settings_.underlayer_width);
    txt_main->SetFillColor(settings_.color_palette[index_colors]);

    if (!is_roundtrip && (stops[0] != stops[stops.size() - 1])) {
        result.reserve(4);
        std::unique_ptr txt_sub_2 = std::make_unique<Text>(*txt_sub);
        std::unique_ptr txt_main_2 = std::make_unique<Text>(*txt_main);
        const svg::Point screen_coord_2 = projector_(stops[stops.size() - 1]->coordinates);
        txt_sub_2->SetPosition(screen_coord_2);
        txt_main_2->SetPosition(screen_coord_2);
        result.push_back(std::move(txt_sub));
        result.push_back(std::move(txt_main));
        result.push_back(std::move(txt_sub_2));
        result.push_back(std::move(txt_main_2));
    }
    else {
        result.reserve(2);
        result.push_back(std::move(txt_sub));
        result.push_back(std::move(txt_main));
    }

    return result;
}

std::vector<std::unique_ptr<svg::Circle>> MapRenderer::CreateStopCircles(const std::set<StopPtr, StopPtrLexicComparator>& stops) const {
    std::vector<std::unique_ptr<svg::Circle>> result;
    result.reserve(stops.size());

    for (const StopPtr& stop : stops) {
        std::unique_ptr circle = std::make_unique<Circle>();
        const svg::Point screen_coord = projector_(stop->coordinates);
        circle->SetCenter(screen_coord).SetRadius(settings_.stop_radius).SetFillColor("white");
        result.push_back(std::move(circle));
    }

    return result;
}

std::vector<std::unique_ptr<svg::Text>> MapRenderer::CreateStopsNames(const std::set<StopPtr, StopPtrLexicComparator>& stops) const {
    std::vector<std::unique_ptr<svg::Text>> result;
    result.reserve(stops.size());

    for (const StopPtr& stop : stops) {
        std::unique_ptr name_text_sub = std::make_unique<Text>();
        const svg::Point screen_coord = projector_(stop->coordinates);

        name_text_sub->SetPosition(screen_coord).SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size).SetFontFamily("Verdana")
            .SetData(stop->name);

        std::unique_ptr name_text_main = std::make_unique<Text>(*name_text_sub);

        name_text_sub->SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width).SetStrokeLineCap(StrokeLineCap::ROUND)
            .SetStrokeLineJoin(StrokeLineJoin::ROUND);

        name_text_main->SetFillColor("black");

        result.push_back(std::move(name_text_sub));
        result.push_back(std::move(name_text_main));
    }
    
    return result;
}
