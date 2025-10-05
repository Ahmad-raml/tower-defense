#pragma once
#include <deque>
#include <vector>
#include <utility>

class Map;        // fwd
struct Cell;      // fwd

// Dijkstra on grid (4-neighbors). When ignoreTowers=true, blocked tiles don't block.
class Pathfinder {
public:
    // returns sequence of (x,y) you should visit NEXT (doesn't include start)
    static std::deque<std::pair<int,int>>
    shortestPath(Map& map, int sx, int sy, int gx, int gy, bool ignoreTowers);

    // multi-goal variant: returns shortest path to the nearest of any goals
    static std::deque<std::pair<int,int>>
    shortestPathToAny(Map& map, int sx, int sy,
                      const std::vector<std::pair<int,int>>& goals,
                      bool ignoreTowers);
};
