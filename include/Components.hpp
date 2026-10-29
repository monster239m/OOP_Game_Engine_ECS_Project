#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <string>
#include <iostream>

// ============================================================================
// CORE ECS PRINCIPLE: Pure Data Structs (Composition over Inheritance)
// Components hold state ONLY. No logic, no methods other than constructors.
// ============================================================================

// Transform / Position Component
struct TransformComponent {
    float x{0.0f};
    float y{0.0f};
    
    TransformComponent() = default;
    TransformComponent(float startX, float startY) : x(startX), y(startY) {}
};

// Physics / Velocity Component
struct VelocityComponent {
    float vx{0.0f};
    float vy{0.0f};
    float speed{1.0f};

    VelocityComponent() = default;
    VelocityComponent(float velX, float velY, float spd = 1.0f) 
        : vx(velX), vy(velY), speed(spd) {}
};

// Render / Sprite Component (ASCII visualization for terminal)
struct RenderComponent {
    char glyph{'?'};
    std::string name{"Unknown"};
    std::string color{"WHITE"};

    RenderComponent() = default;
    RenderComponent(char g, const std::string& n, const std::string& c = "WHITE")
        : glyph(g), name(n), color(c) {}
};

// Health Component
struct HealthComponent {
    int currentHealth{100};
    int maxHealth{100};
    bool isAlive{true};

    HealthComponent() = default;
    HealthComponent(int maxHp) 
        : currentHealth(maxHp), maxHealth(maxHp), isAlive(true) {}

    void takeDamage(int amount) {
        currentHealth -= amount;
        if (currentHealth <= 0) {
            currentHealth = 0;
            isAlive = false;
        }
    }
};

// Collider Component for Axis-Aligned Bounding Box (AABB)
struct ColliderComponent {
    float width{1.0f};
    float height{1.0f};
    bool isTrigger{false};

    ColliderComponent() = default;
    ColliderComponent(float w, float h, bool trigger = false)
        : width(w), height(h), isTrigger(trigger) {}
};

// Tag Component to identify entities (Player, Enemy, Item, Wall)
enum class EntityType {
    Player,
    Enemy,
    Obstacle,
    Collectible
};

struct TagComponent {
    EntityType type{EntityType::Player};
    std::string tag{"Default"};

    TagComponent() = default;
    TagComponent(EntityType t, const std::string& str) : type(t), tag(str) {}
};

#endif // COMPONENTS_HPP
