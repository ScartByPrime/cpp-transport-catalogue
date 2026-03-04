# Transport Catalogue

A functional C++ system for collecting and storing public transport stops and routes, with fast search for optimal passenger routes and SVG visualization.

## Features
- JSON-based configuration — populate database and make queries via JSON documents
- Optimal route search — weighted directed graph with adapted Dijkstra algorithm
- Route visualization — generate SVG maps with stops and routes
- Comprehensive statistics — bus routes, stop information, route calculations

## Building
**Linux / macOS**

*bash*
- git clone https://github.com/ScartByPrime/cpp-transport-catalogue
- cd cpp-transport-catalogue
- mkdir build && cd build
- cmake ..
- make

**Windows**
  
*bash*
- git clone https://github.com/ScartByPrime/cpp-transport-catalogue
- cd cpp-transport-catalogue
- mkdir build && cd build
- cmake ..
- cmake --build . --config Release
## Usage
The program reads in.json from the executable directory and writes results to out.json and out.svg.

**Linux / macOS**

*bash*
- cd build
- ./transport_catalogue

**Windows**
  
*bash*
- cd build\Release
- transport_catalogue.exe

*The input file in.json is automatically copied to the build directory during compilation.*

### Input Structure
The JSON document contains four sections:
- base_requests — stops and bus routes definition
- render_settings — SVG map appearance parameters
- routing_settings — bus wait time and velocity
- stat_requests — queries for bus info, stop info, or route between stops

*You can find the example inside the in.json*

### Output
The program generates:
- out.json — JSON responses matching each query
- out.svg — SVG visualization
