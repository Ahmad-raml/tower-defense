#ifdef HAVE_SFML
#include "SfmlGame.hpp"
#include "towers/Tower.hpp"
#include "econ/ResourceManager.hpp"
#include <algorithm>
#include <cmath>
#include <string>

// Costs & rewards
static const Cost kBasicCost{1,0,0};
static const Cost kKillReward{1,0,0};
static const float kSellRefund = 0.5f; // 50%

SfmlGame::SfmlGame(unsigned w, unsigned h, const char* title)
: map_(20, 15, 40) {
    sf::VideoMode mode({w, h}, 32);   // SFML 3: size + bitsPerPixel
    window_.create(mode, title);
    window_.setFramerateLimit(60);
    resources_.add(40, 0, 0);         // seed materials
}

void SfmlGame::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        handleEvents();
        float dt = clock.restart().asSeconds();
        if (!paused_ && !map_.isGameOver()) update(dt);
        render();
    }
}

void SfmlGame::handleEvents() {
    while (auto evt = window_.pollEvent()) {         // optional<Event>
        if (evt->is<sf::Event::Closed>()) {
            window_.close();
            continue;
        }
        if (auto* k = evt->getIf<sf::Event::KeyPressed>()) {
            if (k->code == sf::Keyboard::Key::P) paused_ = !paused_;
            if (k->code == sf::Keyboard::Key::Num1) selectedType = 1;
            if (k->code == sf::Keyboard::Key::Num2) selectedType = 2;
            if (k->code == sf::Keyboard::Key::Num3) selectedType = 3;
            if (k->code == sf::Keyboard::Key::R) showRanges_ = !showRanges_;
            if (k->code == sf::Keyboard::Key::V) showRoutes_ = !showRoutes_;
        }
        if (auto* m = evt->getIf<sf::Event::MouseButtonPressed>()) {
            const auto pos = m->position; // pixels
            const int cx = pos.x / map_.cellPx();
            const int cy = pos.y / map_.cellPx();

            // ignore clicks outside the grid
            if (!map_.inBounds(cx, cy)) {
                continue;
            }

            if (m->button == sf::Mouse::Button::Left) {
                if (map_.isBuildable(cx, cy) && !map_.hasTowerAt(cx,cy) && resources_.canAfford(kBasicCost)) {
                    if (map_.placeTower(cx, cy) && resources_.spend(kBasicCost)) {
                        Tower t{};
                        t.cx = cx; t.cy = cy;
                        if (selectedType == 1) { // Basic
                            t.type = TowerType::Basic;  t.rangeCells = 3.0f; t.dps = 8.0f;
                        } else if (selectedType == 2) { // Slow
                            t.type = TowerType::Slow;   t.rangeCells = 3.0f; t.dps = 0.0f; t.slowFactor = 0.5f; t.slowDuration = 1.5f;
                        } else { // Splash
                            t.type = TowerType::Splash; t.rangeCells = 3.0f; t.dps = 6.0f; t.splashRadius = 1;
                        }
                        map_.addTower(t);
                        for (auto& cr : map_.creatures()) cr.path.clear(); // force recompute if needed
                    }
                }
            } else if (m->button == sf::Mouse::Button::Right) {
                if (map_.removeTowerAt(cx, cy)) {
                    resources_.add(static_cast<int>(kBasicCost.A * kSellRefund), 0, 0);
                    for (auto& cr : map_.creatures()) cr.path.clear();
                }
            }
        }
    }
}

void SfmlGame::update(float dt) {
    // spawn & movement
    waves_.update(dt, map_, map_.creatures());

    // towers deal damage/effects
    auto& creatures = map_.creatures();
    const auto& towers = map_.towers();

    for (const auto& t : towers) {
        for (auto& cr : creatures) {
            if (cr.gridX < 0) continue; // despawned
            float dx = static_cast<float>(cr.gridX - t.cx);
            float dy = static_cast<float>(cr.gridY - t.cy);
            float dist2 = dx*dx + dy*dy;
            if (dist2 > t.rangeCells * t.rangeCells) continue;

            if (t.type == TowerType::Basic) {
                cr.hp -= t.dps * dt;
            } else if (t.type == TowerType::Slow) {
                cr.speedMultiplier = std::min(cr.speedMultiplier, t.slowFactor);
                cr.slowTimer = std::max(cr.slowTimer, t.slowDuration);
            } else if (t.type == TowerType::Splash) {
                cr.hp -= t.dps * dt;
                for (auto& cr2 : creatures) {
                    if (&cr2 == &cr || cr2.gridX < 0) continue;
                    int gx = std::abs(cr2.gridX - cr.gridX);
                    int gy = std::abs(cr2.gridY - cr.gridY);
                    if (std::max(gx, gy) <= t.splashRadius) {
                        cr2.hp -= 0.5f * t.dps * dt;
                    }
                }
            }
        }
    }

    // collect kills -> reward materials
    for (auto& cr : creatures) {
        if (cr.hp <= 0.f && cr.gridX >= 0) {
            resources_.add(kKillReward.A, kKillReward.B, kKillReward.C);
            cr.gridX = -1000; cr.gridY = -1000;
            cr.path.clear();
            ++kills_;
        }
    }

    // HUD in title
    std::string title = "TD  |  A:" + std::to_string(resources_.a()) +
                        "  B:" + std::to_string(resources_.b()) +
                        "  C:" + std::to_string(resources_.c()) +
                        "  |  Wave:" + std::to_string(waves_.wave()) +
                        "  Kills:" + std::to_string(kills_) +
                        "  Sel:" + std::to_string(selectedType) +
                        (paused_ ? "  [PAUSED]" : "");
    if (map_.isGameOver()) title += "  |  GAME OVER";
    window_.setTitle(title);
}

void SfmlGame::render() {
    window_.clear(sf::Color(20,20,25));
    map_.draw(window_);
    // overlays
    const int cell = map_.cellPx();
    if (showRanges_) {
        sf::CircleShape circle(1.f);
        circle.setFillColor(sf::Color(0,0,0,0));
        circle.setOutlineColor(sf::Color(200,60,60,90));
        circle.setOutlineThickness(2.f);
        for (const auto& t : map_.towers()) {
            float r = t.rangeCells * static_cast<float>(cell);
            circle.setRadius(r);
            circle.setOrigin(sf::Vector2f(r, r));
            circle.setPosition(sf::Vector2f(static_cast<float>(t.cx * cell + cell / 2),
                                            static_cast<float>(t.cy * cell + cell / 2)));
            window_.draw(circle);
        }
    }
    if (showRoutes_) {
        sf::RectangleShape seg(sf::Vector2f(static_cast<float>(cell), static_cast<float>(cell)));
        seg.setFillColor(sf::Color(80,180,240,55));
        for (const auto& cr : map_.creatures()) {
            int x = cr.gridX, y = cr.gridY;
            // draw current cell slightly brighter
            if (x >= 0) {
                seg.setFillColor(sf::Color(120,200,255,80));
                seg.setPosition(sf::Vector2f(static_cast<float>(x * cell), static_cast<float>(y * cell)));
                window_.draw(seg);
                seg.setFillColor(sf::Color(80,180,240,55));
            }
            for (const auto& p : cr.path) {
                seg.setPosition(sf::Vector2f(static_cast<float>(p.first * cell), static_cast<float>(p.second * cell)));
                window_.draw(seg);
            }
        }
    }
    window_.display();
}
#endif
