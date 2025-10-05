#include <memory>
#include <iostream>

#ifdef HAVE_SFML
  #include "SfmlGame.hpp"
#else
  #include "ConsoleGame.hpp"
#endif

int main() {
#ifdef HAVE_SFML
    SfmlGame game(1024, 768, "Tower Defense (SFML)");
#else
    ConsoleGame game;
#endif
    game.run();
    return 0;
}
