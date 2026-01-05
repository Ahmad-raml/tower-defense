#include "Pathfinder.hpp"
#include "Map.hpp"

#include <vector>
#include <set>
#include <deque>
#include <cmath>

using namespace std;

struct CellNode {
    float dist;
	int parentX; // -1 = none, parent cell coords
	int parentY; // -1 = none

	CellNode() : dist(-1.f), parentX(-1), parentY(-1) {} // unvisited
};

static float dangerCost(const Map& map, int x, int y) { // extra cost for being in tower range
    float extra = 0.f;

    for (const auto& t : map.towers()) {
        float dx = float(t.cx - x);
        float dy = float(t.cy - y);
        float d = std::sqrt(dx * dx + dy * dy);

        if (d <= t.rangeCells + 0.5f)
            extra += 0.6f;
    }
    return extra;
}

static const int DX[4] = { 1,-1,0,0 };
static const int DY[4] = { 0,0,1,-1 };

deque<pair<int, int>>
Pathfinder::shortestPath(  // from (sx,sy) to (gx,gy)
    Map& map,
	int sx, int sy, //sy is y coordinate of start
	int gx, int gy, //gy is y coordinate of goal
    bool ignoreTowers
) {
    int W = map.width();
    int H = map.height();

    vector<CellNode> nodes(W * H);

    auto idx = [&](int x, int y) {
        return y * W + x;
        };

	set<pair<float, pair<int, int>>> active;  // dist, (x,y)

    nodes[idx(sx, sy)].dist = 0.f;
    active.insert({ 0.f, {sx,sy} });

    while (!active.empty()) {
        auto it = active.begin();
        float curDist = it->first;
        int x = it->second.first;
        int y = it->second.second;
        active.erase(it);

        if (x == gx && y == gy)
            break;

        for (int k = 0; k < 4; ++k) {
            int nx = x + DX[k];
            int ny = y + DY[k];

            if (!map.inBounds(nx, ny)) continue;
            if (!map.isPath(nx, ny))  continue;

            const Cell& c = map.at(nx, ny);
            if (!ignoreTowers && c.blocked) continue;

            float nd = curDist + 1.f;
            if (!ignoreTowers)
                nd += dangerCost(map, nx, ny);

            int id = idx(nx, ny);

            if (nodes[id].dist < 0 || nd < nodes[id].dist) {
                if (nodes[id].dist >= 0)
                    active.erase({ nodes[id].dist, {nx, ny} });

                nodes[id].dist = nd;
                nodes[id].parentX = x;
                nodes[id].parentY = y;
                active.insert({ nd, {nx, ny} });
            }
        }
    }

    deque<pair<int, int>> path; 
    if (nodes[idx(gx, gy)].dist < 0)
        return path;

    int cx = gx, cy = gy;
    while (!(cx == sx && cy == sy)) {
        path.push_front({ cx,cy });
        int px = nodes[idx(cx, cy)].parentX;
        int py = nodes[idx(cx, cy)].parentY;
        cx = px;
        cy = py;
    }

    return path;
}

deque<pair<int, int>>
Pathfinder::shortestPathToAny( // from (sx,sy) to any of goals
    Map& map,
    int sx, int sy,
    const vector<pair<int, int>>& goals,
    bool ignoreTowers
) {
    if (goals.empty()) return {};

    int W = map.width();
    int H = map.height();

    vector<CellNode> nodes(W * H);

    auto idx = [&](int x, int y) {
        return y * W + x;
        };

    auto isGoal = [&](int x, int y) {
        for (const auto& g : goals)
            if (g.first == x && g.second == y)
                return true;
        return false;
        };

    set<pair<float, pair<int, int>>> active;

    nodes[idx(sx, sy)].dist = 0.f;
    active.insert({ 0.f, {sx,sy} });

    int foundX = -1, foundY = -1;

    while (!active.empty()) {
        auto it = active.begin();
        float curDist = it->first;
        int x = it->second.first;
        int y = it->second.second;
        active.erase(it);

        if (isGoal(x, y)) {
            foundX = x;
            foundY = y;
            break;
        }

        for (int k = 0; k < 4; ++k) {
            int nx = x + DX[k];
            int ny = y + DY[k];

            if (!map.inBounds(nx, ny)) continue;
            if (!map.isPath(nx, ny))  continue;

            const Cell& c = map.at(nx, ny);
            if (!ignoreTowers && c.blocked) continue;

            float nd = curDist + 1.f;
            if (!ignoreTowers)
                nd += dangerCost(map, nx, ny);

            int id = idx(nx, ny);

            if (nodes[id].dist < 0 || nd < nodes[id].dist) {
                if (nodes[id].dist >= 0)
                    active.erase({ nodes[id].dist, {nx, ny} });

                nodes[id].dist = nd;
                nodes[id].parentX = x;
                nodes[id].parentY = y;
                active.insert({ nd, {nx, ny} });
            }
        }
    }

    deque<pair<int, int>> path;
    if (foundX < 0) return path;

    int cx = foundX, cy = foundY;
    while (!(cx == sx && cy == sy)) {
        path.push_front({ cx,cy });
        int px = nodes[idx(cx, cy)].parentX;  
        int py = nodes[idx(cx, cy)].parentY;
        cx = px;
        cy = py;
    }

    return path;
}
