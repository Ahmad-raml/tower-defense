#include "Map.hpp"
#include <cassert>
#include "towers/Tower.hpp"

// ----- constructor & core (no SFML here) -----
Map::Map(int w, int h, int cellPx) : w_(w), h_(h), cellPx_(cellPx) {
    cells_.reserve(w_ * h_);
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            cells_.push_back({x, y, CellType::OpenZone, false});
        }
    }

    // create a single spawn with a fork into two branches that rejoin at the resource
    int yMid = h_ / 2;
    int rx = w_ / 2;

    // mark resource and exit first
    cells_[yMid * w_ + rx].type = CellType::Resource;
    resourceCell_ = {rx, yMid};
    cells_[yMid * w_ + (w_ - 1)].type = CellType::Exit;
    exitCells_.push_back({w_ - 1, yMid});

    // spawn at left middle
    cells_[yMid * w_ + 0].type = CellType::Spawn;
    spawnCells_.push_back({0, yMid});

    // short upper branch: from spawn to junction, go up, across, then down into resource
    int junctionX = 2;
    int yu = std::max(0, yMid - 2); // make upper branch shorter
    for (int x = 1; x <= junctionX; ++x) cells_[yMid * w_ + x].type = CellType::Path; // to junction (keep spawn at x=0)
    for (int y = yMid - 1; y >= yu; --y) cells_[y * w_ + junctionX].type = CellType::Path; // up
    for (int x = junctionX; x <= rx; ++x) cells_[yu * w_ + x].type = CellType::Path; // across
    for (int y = yu + 1; y <= yMid; ++y) cells_[y * w_ + rx].type = CellType::Path; // down into resource

    // longer lower branch: down further, across, then up to just left of resource and into it
    int yl = std::min(h_ - 1, yMid + 5); // make lower branch longer
    for (int y = yMid + 1; y <= yl; ++y) cells_[y * w_ + junctionX].type = CellType::Path; // down
    for (int x = junctionX; x <= rx - 1; ++x) cells_[yl * w_ + x].type = CellType::Path; // across
    for (int y = yl; y >= yMid; --y) cells_[y * w_ + (rx - 1)].type = CellType::Path; // up
    cells_[yMid * w_ + rx].type = CellType::Resource; // ensure resource stays

    // path from resource to exit along the mid row
    for (int x = rx; x < w_; ++x) cells_[yMid * w_ + x].type = CellType::Path;
    cells_[yMid * w_ + (w_ - 1)].type = CellType::Exit; // keep exit

    // spawn one demo creature on the spawn cell; give it a straight path
    CreatureData c;
    c.gridX = 0; c.gridY = yMid;
    // leave path empty so runtime pathfinder chooses the current shortest branch
    creatures_.push_back(c);
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
        || c.type == CellType::Exit || c.type == CellType::Resource;
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
    static sf::Texture texGrass, texDirt, texSpawn, texExit, texResource, texTowerBasic, texTowerSlow, texTowerSplash, texCreature;
    static bool hasGrassTex=false, hasDirtTex=false, hasSpawnTex=false, hasExitTex=false, hasResourceTex=false,
                hasBasicTex=false, hasSlowTex=false, hasSplashTex=false, hasCreatureTex=false;
    if (!texTried) {
        texTried = true;
        auto tryLoad = [](sf::Texture& tex, const char* p1, const char* p2) -> bool {
            if (tex.loadFromFile(p1)) return true;
            return tex.loadFromFile(p2);
        };

        hasGrassTex    = tryLoad(texGrass,       "assets/grass.png",        "src/assets/grass.png");
        hasDirtTex     = tryLoad(texDirt,        "assets/dirt.png",         "src/assets/dirt.png");
        hasSpawnTex    = tryLoad(texSpawn,       "assets/spawn.png",        "src/assets/spawn.png");
        hasExitTex     = tryLoad(texExit,        "assets/exit.png",         "src/assets/exit.png");
        hasResourceTex = tryLoad(texResource,    "assets/resource.png",     "src/assets/resource.png");
        hasBasicTex    = tryLoad(texTowerBasic,  "assets/tower_basic.png",  "src/assets/tower_basic.png");
        hasSlowTex     = tryLoad(texTowerSlow,   "assets/tower_slow.png",   "src/assets/tower_slow.png");
        hasSplashTex   = tryLoad(texTowerSplash, "assets/tower_splash.png", "src/assets/tower_splash.png");
        hasCreatureTex = tryLoad(texCreature,    "assets/creature.png",     "src/assets/creature.png");
    }

    sf::RectangleShape tile(sf::Vector2f(static_cast<float>(cellPx_), static_cast<float>(cellPx_)));
    sf::RectangleShape gridLine;
    gridLine.setFillColor(sf::Color(0,0,0,50));

    for (const auto& c : cells_) {
        auto drawCellSprite = [&](const sf::Texture& tex){
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
        } else if ((c.type == CellType::Path) && hasDirtTex) {
            drawCellSprite(texDirt); drewSprite = true;
        } else if (c.type == CellType::Spawn && hasSpawnTex) {
            drawCellSprite(texSpawn); drewSprite = true;
        } else if (c.type == CellType::Exit && hasExitTex) {
            drawCellSprite(texExit); drewSprite = true;
        } else if (c.type == CellType::Resource && hasResourceTex) {
            drawCellSprite(texResource); drewSprite = true;
        } else {
            tile.setPosition(sf::Vector2f(static_cast<float>(c.x * cellPx_), static_cast<float>(c.y * cellPx_)));
            switch (c.type) {
                case CellType::Path:     tile.setFillColor(sf::Color(85, 85, 95));  break;
                case CellType::OpenZone: tile.setFillColor(sf::Color(30, 40, 50));  break;
                case CellType::Spawn:    tile.setFillColor(sf::Color(0, 140, 20));  break;
                case CellType::Exit:     tile.setFillColor(sf::Color(160, 30, 30)); break;
                case CellType::Resource: tile.setFillColor(sf::Color(210, 170, 20));break;
            }
            if (c.blocked) tile.setFillColor(sf::Color(120,120,160));
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
        auto drawTowerSprite = [&](const sf::Texture& tex){
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
            float r  = static_cast<float>(cellPx_) * 0.28f;
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
            } else if (t.type == TowerType::Slow) {
                sf::RectangleShape arm(sf::Vector2f(r * 1.4f, r * 0.22f));
                arm.setOrigin(sf::Vector2f(r * 0.7f, r * 0.11f));
                arm.setPosition(sf::Vector2f(cx, cy));
                arm.setFillColor(sf::Color(120, 200, 255));
                win.draw(arm);
                arm.setRotation(sf::degrees(90.f));
                win.draw(arm);
            } else {
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
            win.draw(spr);
        }
    } else {
        // fallback: existing circle rendering
        for (const auto& cr : creatures_) {
            if (cr.gridX < 0) continue; // despawned
            sf::CircleShape shape(static_cast<float>(cellPx_) * 0.4f);
            shape.setOrigin(sf::Vector2f(static_cast<float>(cellPx_) * 0.4f,
                                         static_cast<float>(cellPx_) * 0.4f));
            shape.setPosition(sf::Vector2f(static_cast<float>(cr.gridX * cellPx_ + cellPx_ / 2),
                                           static_cast<float>(cr.gridY * cellPx_ + cellPx_ / 2)));
            win.draw(shape);
        }
    }
}
#endif

