#include "Pathfinder.hpp"
#include "Map.hpp"
#include <queue>
#include <limits>
#include <unordered_map>
#include <cmath>
#include <climits>

namespace {
    struct Node { int x, y; };
    struct KeyHash {
        size_t operator()(const long long& k) const noexcept { return std::hash<long long>()(k); }
    };
    inline long long key(int x,int y){ return (static_cast<long long>(x)<<32) ^ (y & 0xffffffffLL); }

    const int DX[4] = {1,-1,0,0};
    const int DY[4] = {0,0,1,-1};

    inline float dangerCost(const Map& map, int x, int y) {
        // Cost grows when inside any tower's range; simple additive model
        // Tunables:
        const float perTowerCost = 0.6f;   // cost per tower covering the tile
        const float maxExtra     = 4.0f;   // cap to avoid runaway costs

        float extra = 0.f;
        for (const auto& t : map.towers()) {
            float dx = static_cast<float>(t.cx - x);
            float dy = static_cast<float>(t.cy - y);
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist <= t.rangeCells + 0.5f) { // +0.5 to approximate cell area
                extra += perTowerCost;
                if (extra >= maxExtra) return maxExtra;
            }
        }
        return extra;
    }

    inline int coverageCount(const Map& map, int x, int y) {
        int cnt = 0;
        for (const auto& t : map.towers()) {
            float dx = static_cast<float>(t.cx - x);
            float dy = static_cast<float>(t.cy - y);
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist <= t.rangeCells + 0.5f) {
                ++cnt;
            }
        }
        return cnt;
    }
}

std::deque<std::pair<int,int>>
Pathfinder::shortestPath(Map& map, int sx, int sy, int gx, int gy, bool ignoreTowers) {
    if (sx==gx && sy==gy) return {};

    const int W = map.width(), H = map.height();
    const float INF = std::numeric_limits<float>::infinity();

    std::vector<float> dist(W*H, INF);
    std::vector<int>   towerExp(W*H, INT_MAX); // total towers encountered along path
    std::vector<long long> parent(W*H, -1);

    auto idx = [&](int x,int y){ return y*W + x; };
    auto inb = [&](int x,int y){ return x>=0 && y>=0 && x<W && y<H; };

    struct QN { int x,y; int t; float d; };
    struct Cmp {
        bool operator()(const QN& a,const QN& b) const {
            if (a.t != b.t) return a.t > b.t;  // prioritize fewer towers first
            return a.d > b.d;                  // then shorter/safer distance
        }
    };
    std::priority_queue<QN,std::vector<QN>,Cmp> pq;

    dist[idx(sx,sy)] = 0.f;
    towerExp[idx(sx,sy)] = 0;
    pq.push({sx,sy,0,0.f});

    while(!pq.empty()){
        auto cur = pq.top(); pq.pop();
        int x = cur.x, y = cur.y; float d = cur.d; int tcur = cur.t;
        int cid = idx(x,y);
        if (d!=dist[cid] || tcur!=towerExp[cid]) continue;
        if (x==gx && y==gy) break;

        for(int k=0;k<4;k++){
            int nx=x+DX[k], ny=y+DY[k];
            if(!inb(nx,ny)) continue;
            if(!map.isPath(nx,ny)) continue;

            const Cell& c = map.at(nx,ny);
            if(!ignoreTowers && c.blocked) continue; // towers block unless fallback

            float nd = d + 1.f + dangerCost(map, nx, ny);
            int nt = tcur + coverageCount(map, nx, ny);
            int id = idx(nx,ny);
            if (nt < towerExp[id] || (nt == towerExp[id] && nd < dist[id])){
                towerExp[id] = nt;
                dist[id]=nd;
                parent[id]=key(x,y);
                pq.push({nx,ny,nt,nd});
            }
        }
    }

    if (dist[idx(gx,gy)]==INF) return {}; // unreachable

    // reconstruct
    std::deque<std::pair<int,int>> out;
    int cx=gx, cy=gy;
    while(!(cx==sx && cy==sy)){
        out.push_front({cx,cy});
        long long p = parent[idx(cx,cy)];
        int px = static_cast<int>(p>>32);
        int py = static_cast<int>(p & 0xffffffffLL);
        cx=px; cy=py;
    }
    return out;
}

std::deque<std::pair<int,int>>
Pathfinder::shortestPathToAny(Map& map, int sx, int sy,
                              const std::vector<std::pair<int,int>>& goals,
                              bool ignoreTowers) {
    if (goals.empty()) return {};

    const int W = map.width(), H = map.height();
    const float INF = std::numeric_limits<float>::infinity();

    auto idx = [&](int x,int y){ return y*W + x; };
    auto inb = [&](int x,int y){ return x>=0 && y>=0 && x<W && y<H; };

    std::vector<float> dist(W*H, INF);
    std::vector<int>   towerExp(W*H, INT_MAX);
    std::vector<long long> parent(W*H, -1);

    struct QN { int x,y; int t; float d; };
    struct Cmp {
        bool operator()(const QN& a,const QN& b) const {
            if (a.t != b.t) return a.t > b.t;
            return a.d > b.d;
        }
    };
    std::priority_queue<QN,std::vector<QN>,Cmp> pq;

    dist[idx(sx,sy)] = 0.f;
    towerExp[idx(sx,sy)] = 0;
    pq.push({sx,sy,0,0.f});

    auto isGoal = [&](int x,int y){
        for (const auto& g : goals) if (g.first==x && g.second==y) return true;
        return false;
    };

    int foundX = -1, foundY = -1;
    while(!pq.empty()){
        auto cur = pq.top(); pq.pop();
        int x = cur.x, y = cur.y; float d = cur.d; int tcur = cur.t;
        int cid = idx(x,y);
        if (d!=dist[cid] || tcur!=towerExp[cid]) continue;
        if (isGoal(x,y)) { foundX=x; foundY=y; break; }

        for(int k=0;k<4;k++){
            int nx=x+DX[k], ny=y+DY[k];
            if(!inb(nx,ny)) continue;
            if(!map.isPath(nx,ny)) continue;

            const Cell& c = map.at(nx,ny);
            if(!ignoreTowers && c.blocked) continue;

            float nd = d + 1.f + dangerCost(map, nx, ny);
            int nt = tcur + coverageCount(map, nx, ny);
            int id = idx(nx,ny);
            if (nt < towerExp[id] || (nt == towerExp[id] && nd < dist[id])){
                towerExp[id] = nt;
                dist[id]=nd;
                parent[id]=key(x,y);
                pq.push({nx,ny,nt,nd});
            }
        }
    }

    if (foundX<0) return {}; // unreachable

    std::deque<std::pair<int,int>> out;
    int cx=foundX, cy=foundY;
    while(!(cx==sx && cy==sy)){
        out.push_front({cx,cy});
        long long p = parent[idx(cx,cy)];
        int px = static_cast<int>(p>>32);
        int py = static_cast<int>(p & 0xffffffffLL);
        cx=px; cy=py;
    }
    return out;
}