#include "WaveManager.hpp"
#include "pathfinding/Pathfinder.hpp"
#include <algorithm> // std::max
#include <limits>
#include <cmath>

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
                toSpawn_ = 10 + waveNo_ * 4;
            }
        }
        // No spawns during inter-wave pause
    } else {
        // spawn creatures every ~1.5s
        spawnTimer_ += dt;
        if (spawnTimer_ > 1.5f && spawned_ < toSpawn_) {
            spawnTimer_ = 0.0f;
            // cycle through multiple spawns if available
            static size_t spawnIndex = 0;
            if (spawnIndex >= spawns.size()) spawnIndex = 0;
            auto s = spawns[spawnIndex++];
            CreatureData c;
            c.gridX = s.first;
            c.gridY = s.second;
            // Scale HP a little by wave
            c.hp = 80.0f + (waveNo_ - 1) * 10.0f;
            creatures.push_back(c);
            ++spawned_;
        }
    }

    // ensure each creature has a path to its current target
    auto ex = exits.front();
    const auto& treasures = map.treasures();
    for (auto& c : creatures) {
        if (c.gridX < 0) continue; // despawned
        const bool targetIsResource = !c.carrying;

        // Find nearest treasure with gold
        int bestTreasureIndex = -1;
        float bestDist = std::numeric_limits<float>::max();
        if (targetIsResource && !treasures.empty()) {
            for (size_t i = 0; i < treasures.size(); ++i) {
                if (map.treasureGold(static_cast<int>(i)) > 0) {
                    float dx = static_cast<float>(treasures[i].first - c.gridX);
                    float dy = static_cast<float>(treasures[i].second - c.gridY);
                    float dist = dx*dx + dy*dy;
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestTreasureIndex = static_cast<int>(i);
                    }
                }
            }
        }

        int gx, gy;
        if (targetIsResource && bestTreasureIndex >= 0) {
            // Target nearest treasure
            gx = treasures[bestTreasureIndex].first;
            gy = treasures[bestTreasureIndex].second;
            c.targetTreasureIndex = bestTreasureIndex;
        } else if (targetIsResource) {
            // Target resource
            gx = res.first;
            gy = res.second;
            c.targetTreasureIndex = -1;
        } else {
            // Target exit
            gx = ex.first;
            gy = ex.second;
        }

        // If path is empty or target changed (e.g., treasure ran out), recompute path
        bool needsNewPath = c.path.empty();
        if (!needsNewPath && targetIsResource) {
            // Check if we're targeting treasure but it's empty, or targeting resource but treasure is available
            if (!c.path.empty() && c.targetTreasureIndex >= 0) {
                auto [nextX, nextY] = c.path.back();
                const auto& targetTr = treasures[c.targetTreasureIndex];
                bool currentlyTargetingTreasure = (nextX == targetTr.first && nextY == targetTr.second);
                bool shouldTargetTreasure = (bestTreasureIndex >= 0);
                if (currentlyTargetingTreasure != shouldTargetTreasure ||
                    (shouldTargetTreasure && c.targetTreasureIndex != bestTreasureIndex)) {
                    needsNewPath = true;
                }
            }
        }

        if (needsNewPath) {
            if (targetIsResource) {
                // target treasure or resource
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

            // Check if we're targeting treasure BEFORE we move (check if treasure is in our path)
            const auto& treasures = map.treasures();
            bool targetingTreasure = false;
            int targetTreasureIdx = -1;
            if (!c.carrying && c.targetTreasureIndex >= 0 && c.targetTreasureIndex < static_cast<int>(treasures.size())) {
                const auto& tr = treasures[c.targetTreasureIndex];
                // Check if treasure is the destination (last in path) or anywhere in path
                if (!c.path.empty()) {
                    // Check if next step is treasure
                    if (nx == tr.first && ny == tr.second) {
                        targetingTreasure = true;
                        targetTreasureIdx = c.targetTreasureIndex;
                    } else {
                        // Check if treasure is anywhere in remaining path
                        for (const auto& [px, py] : c.path) {
                            if (px == tr.first && py == tr.second) {
                                targetingTreasure = true;
                                targetTreasureIdx = c.targetTreasureIndex;
                                break;
                            }
                        }
                    }
                }
            }

            c.path.pop_front();
            c.gridX = nx;
            c.gridY = ny;

            // reached treasure: steal gold, then retarget to exit
            if (!c.carrying && targetTreasureIdx >= 0 && targetTreasureIdx < static_cast<int>(treasures.size())) {
                const auto& tr = treasures[targetTreasureIdx];
                if (nx == tr.first && ny == tr.second) {
                    if (map.treasureGold(targetTreasureIdx) > 0) {
                        c.carrying = true;
                        c.path.clear();
                        int gold = map.treasureGold(targetTreasureIdx);
                        map.treasureGold(targetTreasureIdx) = std::max(0, gold - 5); // steal 5 gold per creature
                    } else {
                        // Treasure is empty, retarget to resource
                        c.path.clear();
                        c.targetTreasureIndex = -1;
                    }
                }
            }
            // reached resource: steal one unit, then retarget to exit (only if not targeting treasure)
            else if (!c.carrying && !targetingTreasure && nx == res.first && ny == res.second) {
                c.carrying = true;
                c.path.clear();
                c.targetTreasureIndex = -1;
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
