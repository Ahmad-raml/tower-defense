#ifndef MAP_HPP
#define MAP_HPP

#include <vector>
#include <deque>
#include "towers/Tower.hpp"   // must include the full type (no forward-decl)

enum class CellType { Path, OpenZone, Spawn, Exit, Resource };

// (single definition of Cell lives below with dirMask)

struct CreatureData {
    int gridX = 0;
    int gridY = 0;
    bool carrying = false;                 // flipped after reaching resource
    float hp = 20.0f;                      // health

    // movement / status
    float speedMultiplier = 1.0f;          // slow effect (<=1)
    float slowTimer = 0.0f;                // remaining slow duration (seconds)
    float moveAccum = 0.0f;                // per-creature movement accumulator

    std::deque<std::pair<int,int>> path;
};

enum DirBits : uint8_t {
    DIR_NONE = 0,
    DIR_U = 1 << 0,
    DIR_D = 1 << 1,
    DIR_L = 1 << 2,
    DIR_R = 1 << 3,
    DIR_ALL = DIR_U | DIR_D | DIR_L | DIR_R
};

struct Cell {
    int x, y;
    CellType type;
    bool blocked = false;      // tower sitting here (only for OpenZone)
    uint8_t dirMask = DIR_ALL; // which directions can LEAVE this cell (for paths)
};

#ifdef HAVE_SFML
namespace sf { class RenderWindow; }
#endif

class Map {

public:

    void neighbors(int x, int y, bool ignoreTowers,
               std::vector<std::pair<int,int>>& out) const;
    Map(int w, int h, int cellPx);

    int width()  const { return w_; }
    int height() const { return h_; }
    int cellPx() const { return cellPx_; }

    bool inBounds(int x, int y) const;
    bool isBuildable(int x, int y) const;
    bool isPath(int x, int y) const;

    const Cell& at(int x, int y) const;
    Cell&       at(int x, int y);

    std::vector<CreatureData>&       creatures()       { return creatures_; }
    const std::vector<CreatureData>& creatures() const { return creatures_; }

#ifdef HAVE_SFML
    void draw(sf::RenderWindow& win) const;
#endif

    // towers
    bool placeTower(int cx, int cy);
    void addTower(const Tower& t) { towers_.push_back(t); }
    const std::vector<Tower>& towers() const { return towers_; }
    bool hasTowerAt(int cx, int cy) const;
    bool removeTowerAt(int cx, int cy);

    // spawn / exit / resource
    const std::vector<std::pair<int,int>>& spawns() const { return spawnCells_; }
    const std::vector<std::pair<int,int>>& exits()  const { return exitCells_;  }
    std::pair<int,int> resource() const { return resourceCell_; }

    // resource units + game over
    int& resourceUnits() { return resourceUnits_; }
    int  resourceUnits() const { return resourceUnits_; }
    bool isGameOver()    const { return resourceUnits_ <= 0; }

private:
    int w_, h_, cellPx_;
    std::vector<Cell> cells_;
    std::vector<CreatureData> creatures_;
    std::vector<Tower> towers_;

    std::vector<std::pair<int,int>> spawnCells_;
    std::vector<std::pair<int,int>> exitCells_;
    std::pair<int,int> resourceCell_{-1,-1};
    int resourceUnits_ = 20;
};

#endif // MAP_HPP
