#include "ConsoleGame.hpp"

ConsoleGame::ConsoleGame()
: map_(20, 15, 1) // cellPx is irrelevant for console
{}

void ConsoleGame::run() {
    using clock = std::chrono::steady_clock;
    auto prev = clock::now();
    bool running = true;
    int steps = 0;
    while (running && steps < 300) { // 300 ticks demo
        auto now = clock::now();
        std::chrono::duration<float> d = now - prev;
        prev = now;
        update(d.count());
        render();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++steps;
    }
}

void ConsoleGame::update(float dt) {
    simTime_ += dt;
    waves_.update(dt, map_, map_.creatures());
}

void ConsoleGame::render() {
    // Clear console (basic)
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    // Simple ASCII grid
    for (int y = 0; y < map_.height(); ++y) {
        for (int x = 0; x < map_.width(); ++x) {
            const Cell& c = map_.at(x, y);
            char ch = '.';
            switch (c.type) {
                case CellType::Path: ch = '='; break;
                case CellType::OpenZone: ch = '.'; break;
                case CellType::Spawn: ch = 'S'; break;
                case CellType::Exit: ch = 'E'; break;
                case CellType::Resource: ch = 'R'; break;
            }
            if (c.blocked) ch = '#';
            // Creature on cell?
            for (const auto& cr : map_.creatures()) {
                if (cr.gridX == x && cr.gridY == y) ch = 'C';
            }
            std::cout << ch;
        }
        std::cout << "\n";
    }
    std::cout << "\nTime: " << simTime_ << "  Creatures: " << map_.creatures().size() << "\n";
}
