#pragma once
enum class TowerType { Basic, Slow, Splash };

struct Tower {
    int cx=0, cy=0;                // cell coords
    TowerType type=TowerType::Basic;

    // common
    float rangeCells=3.0f;         // detection radius
    float dps=8.0f;                // damage per second

    // slow tower
    float slowFactor=0.5f;         // creature speed multiplier (<=1)
    float slowDuration=1.5f;       // seconds

    // splash tower
    int   splashRadius=1;          // cells around hit target
};
