## Tower Defense Starter - README

### Overview
This project is a simple tower defense starter built with C++ and SFML. It includes:
- Multiple lanes from a single spawn that branch and rejoin
- Dynamic pathfinding using Dijkstra that supports multiple goals (multi-exit)
- Tower-aware routing so creatures prefer safer routes (fewer towers, lower risk)
- Interactive SFML UI with overlays for tower ranges and creature routes (toggleable)
- Optional sprite-based rendering (PNG textures) with graceful fallback to vector shapes

### Build and Run
Requirements:
- CMake, a C++17 compiler
- SFML 3 (via vcpkg works well)

Typical Windows (MSVC + vcpkg) steps:
1) Configure with CMake using your vcpkg toolchain (run once):
   - cmake -S . -B build-msvc -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -A x64
2) Build:
   - cmake --build build-msvc --config Debug
3) Run:
   - .\build-msvc\Debug\tower_defense.exe

Note: A simple MinGW/Makefile build is also available under `build/` depending on your environment.

### Controls (SFML build)
- Left click on a buildable tile: place selected tower (if you can afford it)
- Right click on a tower: sell and refund partial cost
- 1 / 2 / 3: switch tower type (Basic / Slow / Splash)
- R: toggle tower range circles
- V: toggle route overlay (off by default)
- P: pause/unpause

### Assets
The game tries to load textures from these locations (first match wins):
- Tiles: `assets/*.png`, `src/assets/*.png`
  - grass.png, dirt.png, spawn.png, exit.png, resource.png
- Towers: `assets/tower_basic.png` (or `src/assets/tower_basic.png`), and also `assets/Tower/Basic.png` (or `src/assets/Tower/Basic.png`) for convenience. Similarly for Slow and Splash.
- Creature: `assets/creature.png`, `src/assets/creature.png`

Sprites are automatically scaled to fit within the grid:
- Tile sprites are scaled to exactly 1 cell
- Tower sprites are centered and scaled to ~90% of a cell
- Creature sprites are centered and scaled to ~85% of a cell

If a texture is missing, the renderer falls back to colored vector shapes.

### Map Layout
The map is generated with a single spawn that quickly forks into two branches of different lengths which rejoin at the resource before heading to the exit. This allows pathfinding to demonstrate shortest-route selection and adaptive rerouting when towers are placed.

### Pathfinding Algorithm
The routing is implemented in `src/pathfinding/Pathfinder.cpp` with two entry points:
- `shortestPath(map, sx, sy, gx, gy, ignoreTowers)`
- `shortestPathToAny(map, sx, sy, goals, ignoreTowers)` (multi-goal, e.g., multiple exits)

Core algorithm: Dijkstra on a 4-connected grid.
- Nodes are grid cells. Edges connect up/down/left/right neighbors that are valid path tiles.
- For `ignoreTowers=false`, cells with blocking towers are not traversable (except for the fallback search where we set `ignoreTowers=true` if needed).
- The cost per step is:
  - Primary key: total number of towers that cover the traversed cells (tower coverage count).
  - Secondary key (tie-breaker): distance with a danger-aware term.

Danger-aware term:
- For each step into `(nx, ny)`, compute an extra cost proportional to how many towers have the cell within their range (simple additive model). This makes routes through dense tower coverage more expensive even when lengths are equal.

Tower count priority:
- In addition to the danger-aware cost, the algorithm first minimizes the number of towers covering the path (total coverage hits along the path). Only when paths have the same coverage count does it compare the (1 + danger) distance.
- Practically: creatures prefer lanes with fewer towers; among lanes with similar tower presence, they choose the shorter/safer one.

Multi-goal search:
- `shortestPathToAny` stops at the first reached goal in a multi-source Dijkstra frontier ordered by (towerCoverage, distance).
- This is used for "carrying to nearest exit" behavior: creatures carrying resources aim for the nearest/safer exit among many.

Dynamic behavior:
- When a tower is placed or removed, creature paths are cleared so they re-run Dijkstra and adapt.
- If a path is blocked by towers, a fallback run with `ignoreTowers=true` allows creatures to still move (per spec), but the preferred run honors blocking to create tactical gameplay.

### UI Overlays
- Range circles: drawn around towers (toggle with R)
- Route overlay: highlights each creature’s current cell and upcoming route (toggle with V). Opacity is intentionally low to be unobtrusive.

### Known Notes
- This is a 2D SFML project. 3D assets like FBX models are not rendered directly; export 2D PNG sprites for use in the current pipeline.
- Click handling ignores clicks outside of the map to avoid assertions.

### File Pointers
- Map: `src/Map.cpp`, `src/Map.hpp`
- Pathfinding: `src/pathfinding/Pathfinder.cpp`, `src/pathfinding/Pathfinder.hpp`
- Game loop and rendering: `src/SfmlGame.cpp`, `src/SfmlGame.hpp`
- Waves: `src/WaveManager.cpp`, `src/WaveManager.hpp`
- Towers: `src/towers/Tower.hpp`

### License
Starter code intended for educational/demo use. Add a license of your choice for your project.