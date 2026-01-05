#include "Map.hpp"
#include <cassert>
#include "towers/Tower.hpp"

// ----- constructor & core (no SFML here) -----
Map::Map(int w, int h, int cellPx) : w_(w), h_(h), cellPx_(cellPx) {
    cells_.reserve(w_ * h_);
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            cells_.push_back({ x, y, CellType::OpenZone, false });
        }
    }

    
    // Global reference points
    int yMid = h_ / 2;
    int xSpawn = 0;
    int xResource = w_ / 2;
    int xExit = w_ - 1;

	// Spawn, resource and exit cells
    cells_[yMid * w_ + xSpawn].type = CellType::Spawn;
    spawnCells_.push_back({ xSpawn, yMid });

    cells_[yMid * w_ + xResource].type = CellType::Resource;
    resourceCell_ = { xResource, yMid };

    cells_[yMid * w_ + xExit].type = CellType::Exit;
    exitCells_.push_back({ xExit, yMid });

	// Shared path segment: spawn to choke point
    int chokeX = 2;
    for (int x = 1; x <= chokeX; ++x)
        cells_[yMid * w_ + x].type = CellType::Path;

    
    // ROUTE 1 — VERY SHORT / VERY DANGEROUS (TOP)

    int yTop1 = std::max(1, yMid - 2);

    for (int y = yMid - 1; y >= yTop1; --y)
        cells_[y * w_ + chokeX].type = CellType::Path;

    for (int x = chokeX; x <= xResource; ++x)
        cells_[yTop1 * w_ + x].type = CellType::Path;

    for (int y = yTop1 + 1; y <= yMid; ++y)
        cells_[y * w_ + xResource].type = CellType::Path;

    cells_[yTop1 * w_ + (xResource - 1)].type = CellType::Treasure;
    treasureCells_.push_back({ xResource - 1, yTop1 });
    treasureGold_.push_back(120);

    // ROUTE 2 — LONG SAFE LOOP (BOTTOM)
    int yBottom = std::min(h_ - 2, yMid + 5);

    for (int y = yMid + 1; y <= yBottom; ++y)
        cells_[y * w_ + chokeX].type = CellType::Path;

    for (int x = chokeX; x <= xResource - 1; ++x)
        cells_[yBottom * w_ + x].type = CellType::Path;

    for (int y = yBottom; y >= yMid; --y)
        cells_[y * w_ + (xResource - 1)].type = CellType::Path;

    cells_[yBottom * w_ + (xResource - 2)].type = CellType::Treasure;
    treasureCells_.push_back({ xResource - 2, yBottom });
    treasureGold_.push_back(80);

   
    // ROUTE 3 — ZIG-ZAG (ANTI-SPLASH)
   
    int yZ = yMid - 1;

    for (int x = chokeX; x <= chokeX + 2; ++x)
        cells_[yZ * w_ + x].type = CellType::Path;

    for (int y = yZ; y <= yZ + 2; ++y)
        cells_[y * w_ + (chokeX + 2)].type = CellType::Path;

    for (int x = chokeX + 2; x <= xResource; ++x)
        cells_[(yZ + 2) * w_ + x].type = CellType::Path;

    for (int y = yZ + 2; y >= yMid; --y)
        cells_[y * w_ + xResource].type = CellType::Path;

    cells_[(yZ + 2) * w_ + (xResource - 1)].type = CellType::Treasure;
    treasureCells_.push_back({ xResource - 1, yZ + 2 });
    treasureGold_.push_back(100);

    // ROUTE 4 — FAST CENTRAL LANE (LOW REWARD)
    
    for (int x = chokeX; x <= xResource; ++x)
        cells_[yMid * w_ + x].type = CellType::Path;

    cells_[yMid * w_ + (xResource - 1)].type = CellType::Treasure;
    treasureCells_.push_back({ xResource - 1, yMid });
    treasureGold_.push_back(50);

    // RESOURCE TO EXIT (COMMON FINAL SEGMENT)
   
    for (int x = xResource; x < xExit; ++x) {
        if (cells_[yMid * w_ + x].type != CellType::Treasure)
            cells_[yMid * w_ + x].type = CellType::Path;
    }

	// Demo creature at spawn
    
    CreatureData c;
    c.gridX = xSpawn;
    c.gridY = yMid;
    creatures_.push_back(c);
}


int Map::treasureGold(int index) const {
    if (index >= 0 && index < static_cast<int>(treasureGold_.size())) {
        return treasureGold_[index];
    }
    return 0;
}

int& Map::treasureGold(int index) {
    if (index >= 0 && index < static_cast<int>(treasureGold_.size())) {
        return treasureGold_[index];
    }
    static int dummy = 0;
    return dummy;
}

int Map::totalTreasureGold() const {
    int total = 0;
    for (int gold : treasureGold_) {
        total += gold;
    }
    return total;
}

bool Map::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < w_ && y < h_;
}

bool Map::hasTowerAt(int cx, int cy) const {
    for (const auto& t : towers_) if (t.cx == cx && t.cy == cy) return true;
    return false;
}

bool Map::removeTowerAt(int cx, int cy) {
    if (!inBounds(cx, cy)) return false;
    for (size_t i = 0; i < towers_.size(); ++i) {
        if (towers_[i].cx == cx && towers_[i].cy == cy) {
            towers_.erase(towers_.begin() + static_cast<long long>(i));
            auto& c = at(cx, cy);
            if (c.type == CellType::OpenZone) c.blocked = false;
            return true;
        }
    }
    return false;
}

bool Map::isBuildable(int x, int y) const {
    const auto& c = at(x, y);
    return c.type == CellType::OpenZone && !c.blocked;
}

bool Map::isPath(int x, int y) const {
    const auto& c = at(x, y);
    return c.type == CellType::Path || c.type == CellType::Spawn
        || c.type == CellType::Exit || c.type == CellType::Resource
        || c.type == CellType::Treasure;
}

const Cell& Map::at(int x, int y) const {
    assert(inBounds(x, y));
    return cells_[y * w_ + x];
}
Cell& Map::at(int x, int y) {
    assert(inBounds(x, y));
    return cells_[y * w_ + x];
}

bool Map::placeTower(int cx, int cy) {
    if (!inBounds(cx, cy)) return false;
    auto& c = at(cx, cy);
    if (c.type != CellType::OpenZone || c.blocked) return false; // only open zones
    c.blocked = true;
    return true;
}

// ----- SFML drawing (only compiled when HAVE_SFML is defined) -----
#ifdef HAVE_SFML
#include <SFML/Graphics.hpp>
void Map::draw(sf::RenderWindow& win) const {
    // Lazy-load textures (optional). If files are missing, fall back to vector shapes.
    static bool texTried = false;
    static sf::Texture texGrass, texDirt, texSpawn, texExit, texResource, texTreasure, texTowerBasic, texTowerSlow, texTowerSplash, texCreature;
    static bool hasGrassTex = false, hasDirtTex = false, hasSpawnTex = false, hasExitTex = false, hasResourceTex = false, hasTreasureTex = false,
        hasBasicTex = false, hasSlowTex = false, hasSplashTex = false, hasCreatureTex = false;
    if (!texTried) {
        texTried = true;
        auto tryLoad = [](sf::Texture& tex, const char* p1, const char* p2) -> bool {
            if (tex.loadFromFile(p1)) return true;
            return tex.loadFromFile(p2);
            };

        hasGrassTex = tryLoad(texGrass, "assets/grass.png", "src/assets/grass.png");
        hasDirtTex = tryLoad(texDirt, "assets/dirt.png", "src/assets/dirt.png");
        hasSpawnTex = tryLoad(texSpawn, "assets/spawn.png", "src/assets/spawn.png");
        hasExitTex = tryLoad(texExit, "assets/exit.png", "src/assets/exit.png");
        hasResourceTex = tryLoad(texResource, "assets/resource.png", "src/assets/resource.png");
        hasTreasureTex = tryLoad(texTreasure, "assets/treasure.png", "src/assets/treasure.png");
        hasBasicTex = tryLoad(texTowerBasic, "assets/tower_basic.png", "src/assets/tower_basic.png");
        hasSlowTex = tryLoad(texTowerSlow, "assets/tower_slow.png", "src/assets/tower_slow.png");
        hasSplashTex = tryLoad(texTowerSplash, "assets/tower_splash.png", "src/assets/tower_splash.png");
        hasCreatureTex = tryLoad(texCreature, "assets/creature.png", "src/assets/creature.png");
    }

    sf::RectangleShape tile(sf::Vector2f(static_cast<float>(cellPx_), static_cast<float>(cellPx_)));
    sf::RectangleShape gridLine;
    gridLine.setFillColor(sf::Color(0, 0, 0, 50));

    for (const auto& c : cells_) {
        auto drawCellSprite = [&](const sf::Texture& tex) {
            sf::Sprite spr(tex);
            const auto sz = tex.getSize();
            if (sz.x > 0 && sz.y > 0) {
                float sx = static_cast<float>(cellPx_) / static_cast<float>(sz.x);
                float sy = static_cast<float>(cellPx_) / static_cast<float>(sz.y);
                spr.setScale(sf::Vector2f(sx, sy));
            }
            spr.setPosition(sf::Vector2f(static_cast<float>(c.x * cellPx_), static_cast<float>(c.y * cellPx_)));
            win.draw(spr);
            };

        bool drewSprite = false;
        if (c.type == CellType::OpenZone && hasGrassTex) {
            drawCellSprite(texGrass); drewSprite = true;
        }
        else if ((c.type == CellType::Path) && hasDirtTex) {
            drawCellSprite(texDirt); drewSprite = true;
        }
        else if (c.type == CellType::Spawn && hasSpawnTex) {
            drawCellSprite(texSpawn); drewSprite = true;
        }
        else if (c.type == CellType::Exit && hasExitTex) {
            drawCellSprite(texExit); drewSprite = true;
        }
        else if (c.type == CellType::Resource && hasResourceTex) {
            drawCellSprite(texResource); drewSprite = true;
        }
        else if (c.type == CellType::Treasure && hasTreasureTex) {
            drawCellSprite(texTreasure); drewSprite = true;
        }
        else {
            tile.setPosition(sf::Vector2f(static_cast<float>(c.x * cellPx_), static_cast<float>(c.y * cellPx_)));
            switch (c.type) {
            case CellType::Path:     tile.setFillColor(sf::Color(85, 85, 95));  break;
            case CellType::OpenZone: tile.setFillColor(sf::Color(30, 40, 50));  break;
            case CellType::Spawn:    tile.setFillColor(sf::Color(0, 140, 20));  break;
            case CellType::Exit:     tile.setFillColor(sf::Color(160, 30, 30)); break;
            case CellType::Resource: tile.setFillColor(sf::Color(210, 170, 20)); break;
            case CellType::Treasure: tile.setFillColor(sf::Color(255, 215, 0));  break; // gold color
            }
            if (c.blocked) tile.setFillColor(sf::Color(120, 120, 160));
            win.draw(tile);
        }
    }

    // grid lines
    for (int x = 0; x <= w_; ++x) {
        gridLine.setSize(sf::Vector2f(1.f, static_cast<float>(h_ * cellPx_)));
        gridLine.setPosition(sf::Vector2f(static_cast<float>(x * cellPx_), 0.f));
        win.draw(gridLine);
    }
    for (int y = 0; y <= h_; ++y) {
        gridLine.setSize(sf::Vector2f(static_cast<float>(w_ * cellPx_), 1.f));
        gridLine.setPosition(sf::Vector2f(0.f, static_cast<float>(y * cellPx_)));
        win.draw(gridLine);
    }
    // creatures are drawn later (sprite if available, else fallback circle)
    // towers: sprites if available, else vector glyphs
    for (const auto& t : towers_) {
        bool drew = false;
        auto drawTowerSprite = [&](const sf::Texture& tex) {
            sf::Sprite spr(tex);
            const auto sz = tex.getSize();
            if (sz.x > 0 && sz.y > 0) {
                float target = static_cast<float>(cellPx_) * 0.9f; // 90% of cell
                float scale = std::min(target / static_cast<float>(sz.x), target / static_cast<float>(sz.y));
                spr.setScale(sf::Vector2f(scale, scale));
                // center origin and place at cell center
                const auto lb = spr.getLocalBounds();
                spr.setOrigin(sf::Vector2f(lb.size.x * 0.5f, lb.size.y * 0.5f));
                spr.setPosition(sf::Vector2f(static_cast<float>(t.cx * cellPx_ + cellPx_ / 2),
                    static_cast<float>(t.cy * cellPx_ + cellPx_ / 2)));
            }
            win.draw(spr);
            };

        if (t.type == TowerType::Basic && hasBasicTex) { drawTowerSprite(texTowerBasic); drew = true; }
        else if (t.type == TowerType::Slow && hasSlowTex) { drawTowerSprite(texTowerSlow); drew = true; }
        else if (t.type == TowerType::Splash && hasSplashTex) { drawTowerSprite(texTowerSplash); drew = true; }
        if (!drew) {
            float cx = static_cast<float>(t.cx * cellPx_ + cellPx_ / 2);
            float cy = static_cast<float>(t.cy * cellPx_ + cellPx_ / 2);
            float r = static_cast<float>(cellPx_) * 0.28f;
            sf::CircleShape base(r);
            base.setOrigin(sf::Vector2f(r, r));
            base.setPosition(sf::Vector2f(cx, cy));
            base.setFillColor(sf::Color(200, 200, 220));
            win.draw(base);
            if (t.type == TowerType::Basic) {
                sf::RectangleShape barrel(sf::Vector2f(r * 1.2f, r * 0.35f));
                barrel.setOrigin(sf::Vector2f(r * 0.6f, r * 0.175f));
                barrel.setPosition(sf::Vector2f(cx, cy));
                barrel.setFillColor(sf::Color(60, 60, 70));
                win.draw(barrel);
            }
            else if (t.type == TowerType::Slow) {
                sf::RectangleShape arm(sf::Vector2f(r * 1.4f, r * 0.22f));
                arm.setOrigin(sf::Vector2f(r * 0.7f, r * 0.11f));
                arm.setPosition(sf::Vector2f(cx, cy));
                arm.setFillColor(sf::Color(120, 200, 255));
                win.draw(arm);
                arm.setRotation(sf::degrees(90.f));
                win.draw(arm);
            }
            else {
                sf::ConvexShape star;
                star.setPointCount(4);
                star.setPoint(0, sf::Vector2f(cx, cy - r));
                star.setPoint(1, sf::Vector2f(cx + r, cy));
                star.setPoint(2, sf::Vector2f(cx, cy + r));
                star.setPoint(3, sf::Vector2f(cx - r, cy));
                star.setFillColor(sf::Color(230, 180, 60));
                win.draw(star);
            }
        }
    }

    // creatures: sprite if available
    if (hasCreatureTex) {
        for (const auto& cr : creatures_) {
            if (cr.gridX < 0) continue;
            sf::Sprite spr(texCreature);
            const auto sz = texCreature.getSize();
            if (sz.x > 0 && sz.y > 0) {
                float target = static_cast<float>(cellPx_) * 0.85f; // 85% of cell
                float scale = std::min(target / static_cast<float>(sz.x), target / static_cast<float>(sz.y));
                spr.setScale(sf::Vector2f(scale, scale));
                const auto lb = spr.getLocalBounds();
                spr.setOrigin(sf::Vector2f(lb.size.x * 0.5f, lb.size.y * 0.5f));
                spr.setPosition(sf::Vector2f(static_cast<float>(cr.gridX * cellPx_ + cellPx_ / 2),
                    static_cast<float>(cr.gridY * cellPx_ + cellPx_ / 2)));
            }
            // Tint gold if carrying
            if (cr.carrying) {
                spr.setColor(sf::Color(255, 215, 0)); // gold tint
            }
            else {
                spr.setColor(sf::Color::White); // reset to white
            }
            win.draw(spr);

            // Draw gold indicator above creature when carrying
            if (cr.carrying) {
                sf::CircleShape goldIndicator(static_cast<float>(cellPx_) * 0.15f);
                goldIndicator.setFillColor(sf::Color(255, 255, 0));
                goldIndicator.setOrigin(sf::Vector2f(static_cast<float>(cellPx_) * 0.15f, static_cast<float>(cellPx_) * 0.15f));
                goldIndicator.setPosition(sf::Vector2f(static_cast<float>(cr.gridX * cellPx_ + cellPx_ / 2),
                    static_cast<float>(cr.gridY * cellPx_ + cellPx_ * 0.2f)));
                win.draw(goldIndicator);
            }
        }
    }
    else {
        // fallback: existing circle rendering
        for (const auto& cr : creatures_) {
            if (cr.gridX < 0) continue; // despawned
            sf::CircleShape shape(static_cast<float>(cellPx_) * 0.4f);
            shape.setOrigin(sf::Vector2f(static_cast<float>(cellPx_) * 0.4f,
                static_cast<float>(cellPx_) * 0.4f));
            shape.setPosition(sf::Vector2f(static_cast<float>(cr.gridX * cellPx_ + cellPx_ / 2),
                static_cast<float>(cr.gridY * cellPx_ + cellPx_ / 2)));
            // Change color if carrying gold
            if (cr.carrying) {
                shape.setFillColor(sf::Color(255, 215, 0)); // gold
            }
            else {
                shape.setFillColor(sf::Color(200, 100, 100)); // normal creature color
            }
            win.draw(shape);

            // Draw gold indicator above creature when carrying
            if (cr.carrying) {
                sf::CircleShape goldIndicator(static_cast<float>(cellPx_) * 0.15f);
                goldIndicator.setFillColor(sf::Color(255, 255, 0));
                goldIndicator.setOrigin(sf::Vector2f(static_cast<float>(cellPx_) * 0.15f, static_cast<float>(cellPx_) * 0.15f));
                goldIndicator.setPosition(sf::Vector2f(static_cast<float>(cr.gridX * cellPx_ + cellPx_ / 2),
                    static_cast<float>(cr.gridY * cellPx_ + cellPx_ * 0.2f)));
                win.draw(goldIndicator);
            }
        }
    }

    // Draw treasure gold amount for all castles
    for (size_t i = 0; i < treasureCells_.size(); ++i) {
        const auto& tr = treasureCells_[i];
        if (tr.first < 0) continue;

        // Draw gold amount as a simple colored rectangle indicator
        sf::RectangleShape goldBar(sf::Vector2f(static_cast<float>(cellPx_) * 0.8f, 4.0f));
        goldBar.setPosition(sf::Vector2f(
            static_cast<float>(tr.first * cellPx_ + cellPx_ * 0.1f),
            static_cast<float>(tr.second * cellPx_ + cellPx_ * 0.9f)
        ));
        float goldRatio = std::min(1.0f, static_cast<float>(treasureGold_[i]) / 100.0f);
        goldBar.setFillColor(sf::Color(
            static_cast<unsigned char>(255 - goldRatio * 155),
            static_cast<unsigned char>(215 + goldRatio * 40),
            0
        ));
        win.draw(goldBar);
    }
}
#endif