#pragma once

namespace transport_catalogue {
    namespace detail {
        struct Coordinates {
            double lat = 0.0;
            double lng = 0.0;
            bool operator==(const Coordinates& other) const {
                return lat == other.lat && lng == other.lng;
            }
            bool operator!=(const Coordinates& other) const {
                return !(*this == other);
            }
        };

        double ComputeDistance(Coordinates from, Coordinates to);
    } // detail
} // transport_catalogue
