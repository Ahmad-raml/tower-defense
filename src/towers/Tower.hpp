#pragma once
enum class TowerType { Basic, Slow, Splash };

struct Tower {
    int cx=0, cy=0;                // cell coords
    TowerType type=TowerType::Basic;

    // common
    float rangeCells=3.0f;         // detection radius
    float dps=8.0f;                // damage per second
    float fireCooldown=0.0f;       // time until can fire again
    float fireRate=1.0f;           // shots per second

    // slow tower
    float slowFactor=0.5f;         // creature speed multiplier (<=1)
    float slowDuration=1.5f;       // seconds

    // splash not overlap tower
    int   splashRadius=1;          // cells around hit target
};

// Bullet structure for visual projectiles
struct Bullet {
    float x, y;                    // current position (world coords)
    float targetX, targetY;         // target position
    float speed;                    // pixels per second
    float damage;                   // damage to deal on hit
    int targetCreatureIndex;        // index of target creature (-1 if targeting position)
    TowerType towerType;            // type of tower that fired it
    float splashRadius;             // splash damage radius (for splash towers)
    bool active;                    // is this bullet active?

    Bullet() : x(0), y(0), targetX(0), targetY(0), speed(400.0f), damage(0),
               targetCreatureIndex(-1), towerType(TowerType::Basic), splashRadius(0), active(false) {}
};
