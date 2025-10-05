#include "WaveManager.hpp"
#include "pathfinding/Pathfinder.hpp"
#include <algorithm> // std::max

void WaveManager::update(float dt, Map& map, std::vector<CreatureData>& creatures)
{
    // --- SAFETY GUARDS ---
    const auto& spawns = map.spawns();
    const auto& exits  = map.exits();
    auto        res    = map.resource();
    if (spawns.empty() || exits.empty() || res.first < 0) {
        return; // map not initialized yet; skip this tick
    }
    // ----------------------

    // If we've spawned all for this wave, wait until field is clear, then advance wave
    if (spawned_ >= toSpawn_) {
        bool anyActive = false;
        for (const auto& c : creatures) if (c.gridX >= 0) { anyActive = true; break; }

        if (!anyActive) {
            betweenTimer_ += dt;
            if (betweenTimer_ >= 2.0f) {       // small break between waves
                betweenTimer_ = 0.0f;
                spawned_ = 0;
                ++waveNo_;
                toSpawn_ = std::min(5 + waveNo_, 20); // slight ramp
            }
        }
        // No spawns during inter-wave pause
    } else {
        // spawn creatures every ~2s
        spawnTimer_ += dt;
        if (spawnTimer_ > 2.0f && spawned_ < toSpawn_) {
            spawnTimer_ = 0.0f;
            // cycle through multiple spawns if available
            static size_t spawnIndex = 0;
            if (spawnIndex >= spawns.size()) spawnIndex = 0;
            auto s = spawns[spawnIndex++];
            CreatureData c;
            c.gridX = s.first;
            c.gridY = s.second;
            // Scale HP a little by wave
            c.hp = 20.0f + (waveNo_ - 1) * 6.0f;
            creatures.push_back(c);
            ++spawned_;
        }
    }

    // ensure each creature has a path to its current target
    auto ex = exits.front();
    for (auto& c : creatures) {
        if (c.gridX < 0) continue; // despawned
        const bool targetIsResource = !c.carrying;
        int gx = targetIsResource ? res.first  : ex.first;
        int gy = targetIsResource ? res.second : ex.second;

        if (c.path.empty()) {
            if (targetIsResource) {
                // target the single resource cell normally
                c.path = Pathfinder::shortestPath(map, c.gridX, c.gridY, gx, gy, /*ignoreTowers=*/false);
                if (c.path.empty()) {
                    c.path = Pathfinder::shortestPath(map, c.gridX, c.gridY, gx, gy, /*ignoreTowers=*/true);
                }
            } else {
                // carrying: target nearest of any exits
                c.path = Pathfinder::shortestPathToAny(map, c.gridX, c.gridY, exits, /*ignoreTowers=*/false);
                if (c.path.empty()) {
                    c.path = Pathfinder::shortestPathToAny(map, c.gridX, c.gridY, exits, /*ignoreTowers=*/true);
                }
            }
        }
    }

    // move creatures respecting slow
    const float baseStep = 0.3f;
    for (auto& c : creatures) {
        if (c.gridX < 0) continue; // already despawned

        // slow decay
        if (c.slowTimer > 0.f) {
            c.slowTimer -= dt;
            if (c.slowTimer <= 0.f) {
                c.slowTimer = 0.f;
                c.speedMultiplier = 1.0f;
            }
        }

        // accumulate movement time
        c.moveAccum += dt;
        const float stepPeriod = baseStep / std::max(0.1f, c.speedMultiplier);

        if (c.moveAccum > stepPeriod && !c.path.empty()) {
            c.moveAccum -= stepPeriod;

            auto [nx, ny] = c.path.front();
            c.path.pop_front();
            c.gridX = nx;
            c.gridY = ny;

            // reached resource: steal one unit, then retarget to exit
            if (!c.carrying && nx == res.first && ny == res.second) {
                c.carrying = true;
                c.path.clear();
                int left = map.resourceUnits();
                map.resourceUnits() = std::max(0, left - 1);
            }
            // reached exit while carrying: despawn
            else if (c.carrying && nx == ex.first && ny == ex.second) {
                c.path.clear();
                c.gridX = -1000;
                c.gridY = -1000; // mark as despawned
            }
        }
    }
}
