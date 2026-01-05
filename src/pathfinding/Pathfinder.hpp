#pragma once

#include <deque>
#include <vector>
#include <utility>

// Forward declaration
class Map;

/*
    Pathfinder class
    Provides Dijkstra-based shortest path computation on the map grid.
*/
class Pathfinder {
public:
    // Path from (sx, sy) to (gx, gy)
    static std::deque<std::pair<int, int>>
        shortestPath(
            Map& map,
            int sx, int sy,
            int gx, int gy,
            bool ignoreTowers = false
        );

    // Path from (sx, sy) to any goal in the list
    static std::deque<std::pair<int, int>>
        shortestPathToAny(
            Map& map,
            int sx, int sy,
            const std::vector<std::pair<int, int>>& goals,
            bool ignoreTowers = false
        );
};
