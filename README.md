# cpp-transport-catalogue
Transport Catalogue.
A functional C++ system for collecting and storing stops and routes data with fast search for optimal routes for passengers.

Solution
The algorithm is based on a weighted directed graph and uses adapted Dijkstra and Floyd-Warshall algorithms.
The directory is populated via a JSON document.
Route visualization is implemented in SVG format.
Fully IDE build.

Result
An efficient and scalable solution was developed, fully prepared for further integration. However, the solution is already completely independent:
- Efficiently populates the database from a JSON document.
- Fast, comprehensive database search is implemented.
- The application can find the most efficient route and visualize it, generating instructions in SVG format.
