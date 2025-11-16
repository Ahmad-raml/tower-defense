#ifdef HAVE_SFML
#include <SFML/Graphics.hpp>
#include "Map.hpp"
#include "WaveManager.hpp"
#include "econ/ResourceManager.hpp"
#include "towers/Tower.hpp" // can be in .cpp only; header doesn’t need it strictly

class SfmlGame {
public:
    SfmlGame(unsigned width, unsigned height, const char* title);
    void run();

private:
    bool paused_ = false;
    int  kills_  = 0;
    int  selectedType = 1; // 1=Basic, 2=Slow, 3=Splash
    bool showRanges_ = false;  // UI overlay: tower ranges
    bool showRoutes_ = false; // UI overlay: creature planned routes (V to toggle)

    void handleEvents();
    void update(float dt);
    void render();
    void updateBullets(float dt);
    void renderBullets();

    sf::RenderWindow window_;
    Map map_;
    WaveManager waves_;
    ResourceManager resources_;
    std::vector<Bullet> bullets_;
};
#endif
