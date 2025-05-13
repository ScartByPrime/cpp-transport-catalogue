#include <string>
#include <string_view>
#include <iostream>
#include <sstream>

#include "input_reader.h"
#include "transport_catalogue.h"
#include "stat_reader.h"

using namespace std::literals::string_view_literals;
using namespace transport_catalogue;
using namespace interface;

// FRAMEWORK
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "(" << line << "): " << func << ": ";
        std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cerr << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cerr << " Hint: " << hint;
        }
        std::cerr << std::endl;
        std::abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cerr << file << "(" << line << "): " << func << ": ";
        std::cerr << "ASSERT(" << expr_str << ") failed.";
        if (!hint.empty()) {
            std::cerr << " Hint: " << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

// Удаляет символы новой строки с конца строки
std::string TrimTrailingNewline(const std::string& str) {
    std::string result = str;
    result.erase(result.find_last_not_of('\n') + 1);
    return result;
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const std::string& test_name) {
    func();
    std::cerr << test_name << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)
// FRAMEWORK END

void TestStopAdding() {
    InputReader reader;
    std::string input_single = "Stop Tolstopaltsevo : 55.611087, 37.208290";
    reader.ParseLine(input_single);
    std::string input_with_space = "Stop Biryulyovo Tovarnaya : 55.592028, 37.653656";
    reader.ParseLine(input_with_space);
    TransportCatalogue catalogue;
    reader.ApplyCommands(catalogue);
    ASSERT_HINT(catalogue.GetStop("Tolstopaltsevo"sv) != nullptr, "Stop with single-word name was added incorrectly");
    ASSERT_HINT(catalogue.GetStop("Biryulyovo Tovarnaya"sv) != nullptr, "Stop with multiple-words name was added incorrectly");
}

void TestBufferingAndCalculations() {
    InputReader reader;
    std::string input_stop_1 = "Stop Tolstopaltsevo : 55.611087, 37.208290";
    reader.ParseLine(input_stop_1);
    std::string input_regular_route = "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka";
    reader.ParseLine(input_regular_route);
    std::string input_ring_route = "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye";
    reader.ParseLine(input_ring_route);
    std::string input_stop_2 = "Stop Rasskazovka : 55.632761, 37.333324";
    reader.ParseLine(input_stop_2);
    std::string input_stop_3 = "Stop Marushkino: 55.595884, 37.209755";
    reader.ParseLine(input_stop_3);
    std::string input_stop_4 = "Stop Biryulyovo Zapadnoye: 55.574371, 37.651700";
    reader.ParseLine(input_stop_4);
    std::string input_stop_5 = "Stop Biryusinka: 55.581065, 37.648390";
    reader.ParseLine(input_stop_5);
    std::string input_stop_6 = "Stop Universam: 55.587655, 37.645687";
    reader.ParseLine(input_stop_6);
    std::string input_stop_7 = "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656";
    reader.ParseLine(input_stop_7);
    std::string input_stop_8 = "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164";
    reader.ParseLine(input_stop_8);
    TransportCatalogue catalogue;
    reader.ApplyCommands(catalogue);
    RouteStatistics stat1 = catalogue.GetRouteInfo("750"sv);
    RouteStatistics stat2 = catalogue.GetRouteInfo("256"sv);
    ASSERT_EQUAL_HINT(stat1.route_size, size_t(5), "Buffering malfunction");
    ASSERT_EQUAL_HINT(stat1.unique_stops, 3, "Incorrect unique stops calculation");
    ASSERT_EQUAL_HINT(stat2.route_size, size_t(6), "Buffering malfunction");
    ASSERT_EQUAL_HINT(stat2.unique_stops, 5, "Incorrect unique stops calculation");
}

void TestRequestsToCatalogue() {
    InputReader reader;
    std::string input_stop_1 = "Stop Tolstopaltsevo : 55.611087, 37.208290";
    reader.ParseLine(input_stop_1);
    std::string input_regular_route = "Bus 750 One Two: Tolstopaltsevo - Marushkino - Rasskazovka";
    reader.ParseLine(input_regular_route);
    std::string input_ring_route = "Bus Bus 256 123142 eqwo: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye";
    reader.ParseLine(input_ring_route);
    std::string input_stop_2 = "Stop Rasskazovka : 55.632761, 37.333324";
    reader.ParseLine(input_stop_2);
    std::string input_stop_3 = "Stop Marushkino: 55.595884, 37.209755";
    reader.ParseLine(input_stop_3);
    std::string input_stop_4 = "Stop Biryulyovo Zapadnoye: 55.574371, 37.651700";
    reader.ParseLine(input_stop_4);
    std::string input_stop_5 = "Stop Biryusinka: 55.581065, 37.648390";
    reader.ParseLine(input_stop_5);
    std::string input_stop_6 = "Stop Universam: 55.587655, 37.645687";
    reader.ParseLine(input_stop_6);
    std::string input_stop_7 = "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656";
    reader.ParseLine(input_stop_7);
    std::string input_stop_8 = "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164";
    reader.ParseLine(input_stop_8);
    TransportCatalogue catalogue;
    reader.ApplyCommands(catalogue);
    std::string request = "    Bus Bus 256 123142 eqwo ";
    std::ostringstream out;
    ParseAndPrintStat(catalogue, request, out);
    std::string result = out.str();
    ASSERT_EQUAL_HINT(TrimTrailingNewline(result), std::string("Bus Bus 256 123142 eqwo: 6 stops on route, 5 unique stops, 4371.02 route length"),
        "Incorrect BUS request processing");
    out.str("");
    request = " Bus 750 One Twwo  ";
    ParseAndPrintStat(catalogue, request, out);
    result = out.str();
    ASSERT_EQUAL_HINT(TrimTrailingNewline(result), std::string("Bus 750 One Twwo: not found"),
        "Incorrect BUS request processing");
    out.str("");
    request = "    Bus 750 One Two   ";
    ParseAndPrintStat(catalogue, request, out);
    result = out.str();
    ASSERT_EQUAL_HINT(TrimTrailingNewline(result), std::string("Bus 750 One Two: 5 stops on route, 3 unique stops, 20939.5 route length"),
        "Incorrect BUS request processing");
    out.str("");
    request = "     Stop Rasskazovka     ";
    ParseAndPrintStat(catalogue, request, out);
    result = out.str();
    ASSERT_EQUAL_HINT(TrimTrailingNewline(result), std::string("Stop Rasskazovka: buses 750 One Two"),
        "Incorrect STOP request processing");
}

void TestTransportCatalogue() {
    RUN_TEST(TestStopAdding);
    RUN_TEST(TestBufferingAndCalculations);
    RUN_TEST(TestRequestsToCatalogue);
}