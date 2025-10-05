#pragma once
#include "Map.hpp"
#include "WaveManager.hpp"
#include "ResourceManager.hpp"
#include <chrono>
#include <thread>
#include <iostream>

class ConsoleGame {
public:
    ConsoleGame();
    void run();

private:
    void update(float dt);
    void render();

    Map map_;
    WaveManager waves_;
    ResourceManager resources_;
    float simTime_ = 0.0f;
};
