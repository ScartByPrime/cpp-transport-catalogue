#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace interface {
    void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& tansport_catalogue, std::string_view request,
        std::ostream& output);
}