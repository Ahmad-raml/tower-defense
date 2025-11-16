#ifdef HAVE_SFML
#include "SfmlGame.hpp"
#include "towers/Tower.hpp"
#include "econ/ResourceManager.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

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
                            t.type = TowerType::Basic;  t.rangeCells = 3.0f; t.dps = 8.0f; t.fireRate = 1.5f;
                        } else if (selectedType == 2) { // Slow
                            t.type = TowerType::Slow;   t.rangeCells = 3.0f; t.dps = 0.0f; t.fireRate = 2.0f; t.slowFactor = 0.5f; t.slowDuration = 1.5f;
                        } else { // Splash
                            t.type = TowerType::Splash; t.rangeCells = 3.0f; t.dps = 6.0f; t.fireRate = 1.0f; t.splashRadius = 1;
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

    // towers fire bullets
    auto& creatures = map_.creatures();
    const auto& towers = map_.towers();
    const int cellPx = map_.cellPx();

    // Need to track tower cooldowns separately since towers are const
    static std::vector<float> towerCooldowns;
    // Resize to match current tower count (handles tower removal)
    if (towerCooldowns.size() != towers.size()) {
        towerCooldowns.resize(towers.size(), 0.0f);
    }

    for (size_t ti = 0; ti < towers.size(); ++ti) {
        const auto& t = towers[ti];

        // Update fire cooldown
        if (towerCooldowns[ti] > 0.0f) {
            towerCooldowns[ti] -= dt;
        }

        // Find target in range
        int bestTarget = -1;
        float bestDist = t.rangeCells * t.rangeCells;

        for (size_t ci = 0; ci < creatures.size(); ++ci) {
            const auto& cr = creatures[ci];
            if (cr.gridX < 0) continue; // despawned
            float dx = static_cast<float>(cr.gridX - t.cx);
            float dy = static_cast<float>(cr.gridY - t.cy);
            float dist2 = dx*dx + dy*dy;
            if (dist2 <= bestDist && dist2 <= t.rangeCells * t.rangeCells) {
                bestDist = dist2;
                bestTarget = static_cast<int>(ci);
            }
        }

        // Fire bullet if target found and cooldown ready
        if (bestTarget >= 0 && towerCooldowns[ti] <= 0.0f) {
            const auto& target = creatures[bestTarget];
            float towerX = static_cast<float>(t.cx * cellPx + cellPx / 2);
            float towerY = static_cast<float>(t.cy * cellPx + cellPx / 2);
            float targetX = static_cast<float>(target.gridX * cellPx + cellPx / 2);
            float targetY = static_cast<float>(target.gridY * cellPx + cellPx / 2);

            // Create bullet
            Bullet b;
            b.x = towerX;
            b.y = towerY;
            b.targetX = targetX;
            b.targetY = targetY;
            b.targetCreatureIndex = bestTarget;
            b.towerType = t.type;
            b.speed = 400.0f; // pixels per second
            b.active = true;

            // Calculate damage per shot (dps / fireRate)
            if (t.type == TowerType::Basic) {
                b.damage = t.dps / t.fireRate;
                b.splashRadius = 0.0f;
            } else if (t.type == TowerType::Slow) {
                b.damage = 0.0f; // slow doesn't do damage
                b.splashRadius = 0.0f;
            } else if (t.type == TowerType::Splash) {
                b.damage = t.dps / t.fireRate;
                b.splashRadius = static_cast<float>(t.splashRadius * cellPx);
            }

            bullets_.push_back(b);
            towerCooldowns[ti] = 1.0f / t.fireRate; // reset cooldown
        }
    }

    // Update bullets
    updateBullets(dt);

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
                        "  Gold:" + std::to_string(map_.totalTreasureGold()) +
                        "  Sel:" + std::to_string(selectedType) +
                        (paused_ ? "  [PAUSED]" : "");
    if (map_.isGameOver()) title += "  |  GAME OVER";
    window_.setTitle(title);
}

void SfmlGame::updateBullets(float dt) {
    auto& creatures = map_.creatures();
    const int cellPx = map_.cellPx();

    for (auto& b : bullets_) {
        if (!b.active) continue;

        // Update target position if creature is still alive
        if (b.targetCreatureIndex >= 0 && b.targetCreatureIndex < static_cast<int>(creatures.size())) {
            const auto& target = creatures[b.targetCreatureIndex];
            if (target.gridX >= 0) {
                b.targetX = static_cast<float>(target.gridX * cellPx + cellPx / 2);
                b.targetY = static_cast<float>(target.gridY * cellPx + cellPx / 2);
            }
        }

        // Move bullet toward target
        float dx = b.targetX - b.x;
        float dy = b.targetY - b.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < 5.0f) { // Hit target (within 5 pixels)
            // Apply damage/effects
            if (b.targetCreatureIndex >= 0 && b.targetCreatureIndex < static_cast<int>(creatures.size())) {
                auto& target = creatures[b.targetCreatureIndex];

                if (b.towerType == TowerType::Basic) {
                    target.hp -= b.damage;
                } else if (b.towerType == TowerType::Slow) {
                    // Find the tower to get slow parameters
                    const auto& towers = map_.towers();
                    for (const auto& t : towers) {
                        if (t.type == TowerType::Slow) {
                            target.speedMultiplier = std::min(target.speedMultiplier, t.slowFactor);
                            target.slowTimer = std::max(target.slowTimer, t.slowDuration);
                            break;
                        }
                    }
                } else if (b.towerType == TowerType::Splash) {
                    // Apply splash damage
                    target.hp -= b.damage;
                    for (auto& cr : creatures) {
                        if (&cr == &target || cr.gridX < 0) continue;
                        float crX = static_cast<float>(cr.gridX * cellPx + cellPx / 2);
                        float crY = static_cast<float>(cr.gridY * cellPx + cellPx / 2);
                        float splashDx = crX - b.targetX;
                        float splashDy = crY - b.targetY;
                        float splashDist = std::sqrt(splashDx*splashDx + splashDy*splashDy);
                        if (splashDist <= b.splashRadius) {
                            cr.hp -= 0.5f * b.damage;
                        }
                    }
                }
            }
            b.active = false;
        } else {
            // Move bullet
            float moveDist = b.speed * dt;
            if (moveDist > dist) moveDist = dist;
            b.x += (dx / dist) * moveDist;
            b.y += (dy / dist) * moveDist;
        }
    }

    // Remove inactive bullets
    bullets_.erase(
        std::remove_if(bullets_.begin(), bullets_.end(),
            [](const Bullet& b) { return !b.active; }),
        bullets_.end()
    );
}

void SfmlGame::renderBullets() {
    for (const auto& b : bullets_) {
        if (!b.active) continue;

        // Draw bullet based on tower type
        sf::CircleShape bullet(4.0f);
        bullet.setOrigin(sf::Vector2f(4.0f, 4.0f));
        bullet.setPosition(sf::Vector2f(b.x, b.y));

        if (b.towerType == TowerType::Basic) {
            bullet.setFillColor(sf::Color(255, 100, 100)); // red
        } else if (b.towerType == TowerType::Slow) {
            bullet.setFillColor(sf::Color(100, 200, 255)); // blue
        } else if (b.towerType == TowerType::Splash) {
            bullet.setFillColor(sf::Color(255, 200, 100)); // orange
        }

        window_.draw(bullet);
    }
}

void SfmlGame::render() {
    window_.clear(sf::Color(20,20,25));
    map_.draw(window_);
    renderBullets();
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
