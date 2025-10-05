#pragma once
#include <vector>
#include "Map.hpp"

class WaveManager {
public:
    void update(float dt, Map& map, std::vector<CreatureData>& creatures);

    // HUD needs this:
    int wave() const { return waveNo_; }

private:
    float spawnTimer_   = 0.0f;
    int   spawned_      = 0;     // spawned so far in this wave
    int   toSpawn_      = 5;     // how many to spawn per wave
    int   waveNo_       = 1;     // current wave number (starts at 1)
    float betweenTimer_ = 0.0f;  // inter-wave delay timer
};
