#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "ECS.hpp"
#include "Components.hpp"
#include "Events.hpp"

// ============================================================================
// SYSTEMS: The "S" in ECS (Pure Logic Handlers)
// Demonstrates:
// - Runtime Polymorphism via ISystem base class
// - Separation of Data (Components) from Logic (Systems)
// - Event Subscription and Dispatching
// ============================================================================

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(Coordinator& coordinator, float dt) = 0;
};

// ----------------------------------------------------------------------------
// MovementSystem: Updates position based on velocity and delta time
// ----------------------------------------------------------------------------
class MovementSystem : public ISystem {
public:
    void update(Coordinator& coordinator, float dt) override {
        for (auto entity : coordinator.getAllEntities()) {
            if (coordinator.hasComponent<TransformComponent>(entity) &&
                coordinator.hasComponent<VelocityComponent>(entity)) {
                
                auto& transform = coordinator.getComponent<TransformComponent>(entity);
                auto& velocity = coordinator.getComponent<VelocityComponent>(entity);

                transform.x += velocity.vx * velocity.speed * dt;
                transform.y += velocity.vy * velocity.speed * dt;

                // Clamp to screen boundaries (0 to 19, 0 to 9)
                if (transform.x < 0.0f) transform.x = 0.0f;
                if (transform.x > 19.0f) transform.x = 19.0f;
                if (transform.y < 0.0f) transform.y = 0.0f;
                if (transform.y > 9.0f) transform.y = 9.0f;
            }
        }
    }
};

// ----------------------------------------------------------------------------
// CollisionSystem: Detects AABB bounding box overlaps & publishes events
// ----------------------------------------------------------------------------
class CollisionSystem : public ISystem {
public:
    CollisionSystem(EventBus& bus) : eventBus(bus) {}

    void update(Coordinator& coordinator, float dt) override {
        (void)dt;
        const auto& entities = coordinator.getAllEntities();
        std::size_t count = entities.size();

        for (std::size_t i = 0; i < count; ++i) {
            Entity a = entities[i];
            if (!coordinator.hasComponent<TransformComponent>(a) ||
                !coordinator.hasComponent<ColliderComponent>(a)) continue;

            const auto& tA = coordinator.getComponent<TransformComponent>(a);
            const auto& cA = coordinator.getComponent<ColliderComponent>(a);

            for (std::size_t j = i + 1; j < count; ++j) {
                Entity b = entities[j];
                if (!coordinator.hasComponent<TransformComponent>(b) ||
                    !coordinator.hasComponent<ColliderComponent>(b)) continue;

                const auto& tB = coordinator.getComponent<TransformComponent>(b);
                const auto& cB = coordinator.getComponent<ColliderComponent>(b);

                // Check Axis-Aligned Bounding Box (AABB) intersection
                bool overlapX = (std::abs(tA.x - tB.x) * 2 < (cA.width + cB.width));
                bool overlapY = (std::abs(tA.y - tB.y) * 2 < (cA.height + cB.height));

                if (overlapX && overlapY) {
                    eventBus.publish(CollisionEvent(a, b, std::abs(tA.x - tB.x), std::abs(tA.y - tB.y)));
                }
            }
        }
    }

private:
    EventBus& eventBus;
};

// ----------------------------------------------------------------------------
// CombatSystem: Responds to collisions (Damage & Destruction)
// ----------------------------------------------------------------------------
class CombatSystem {
public:
    CombatSystem(Coordinator& coord, EventBus& bus) 
        : coordinator(coord), eventBus(bus) {
        
        // Subscribe to CollisionEvent
        eventBus.subscribe<CollisionEvent>([this](const CollisionEvent& event) {
            this->onCollision(event);
        });
    }

    void onCollision(const CollisionEvent& event) {
        Entity a = event.entityA;
        Entity b = event.entityB;

        if (!coordinator.hasComponent<TagComponent>(a) || 
            !coordinator.hasComponent<TagComponent>(b)) return;

        auto& tagA = coordinator.getComponent<TagComponent>(a);
        auto& tagB = coordinator.getComponent<TagComponent>(b);

        // Player vs Enemy Collision
        if ((tagA.type == EntityType::Player && tagB.type == EntityType::Enemy) ||
            (tagA.type == EntityType::Enemy && tagB.type == EntityType::Player)) {
            
            Entity player = (tagA.type == EntityType::Player) ? a : b;
            Entity enemy  = (tagA.type == EntityType::Player) ? b : a;

            if (coordinator.hasComponent<HealthComponent>(player)) {
                auto& playerHp = coordinator.getComponent<HealthComponent>(player);
                int oldHp = playerHp.currentHealth;
                playerHp.takeDamage(20);
                std::cout << " [COMBAT] Player collided with Enemy! Took 20 damage. Health: " 
                          << playerHp.currentHealth << "/" << playerHp.maxHealth << "\n";

                eventBus.publish(HealthChangedEvent(player, oldHp, playerHp.currentHealth));

                if (!playerHp.isAlive) {
                    eventBus.publish(EntityDestroyedEvent(player, "Defeated by Enemy in combat"));
                }
            }
        }
        // Player vs Collectible Collision
        else if ((tagA.type == EntityType::Player && tagB.type == EntityType::Collectible) ||
                 (tagA.type == EntityType::Collectible && tagB.type == EntityType::Player)) {
            
            Entity item = (tagA.type == EntityType::Collectible) ? a : b;
            std::cout << " [PICKUP] Player collected item (Entity " << item << ")!\n";
            eventBus.publish(EntityDestroyedEvent(item, "Collected by Player"));
        }
    }

private:
    Coordinator& coordinator;
    EventBus& eventBus;
};

// ----------------------------------------------------------------------------
// RenderSystem: ASCII Viewport Grid Frame Buffer (20 x 10)
// ----------------------------------------------------------------------------
class RenderSystem : public ISystem {
public:
    RenderSystem(int width = 20, int height = 10) 
        : screenWidth(width), screenHeight(height) {}

    void update(Coordinator& coordinator, float dt) override {
        (void)dt;
        // Allocate empty buffer
        std::vector<std::string> buffer(screenHeight, std::string(screenWidth, '.'));

        // Draw entities onto buffer
        for (auto entity : coordinator.getAllEntities()) {
            if (coordinator.hasComponent<TransformComponent>(entity) &&
                coordinator.hasComponent<RenderComponent>(entity)) {
                
                const auto& transform = coordinator.getComponent<TransformComponent>(entity);
                const auto& render = coordinator.getComponent<RenderComponent>(entity);

                int gridX = static_cast<int>(std::round(transform.x));
                int gridY = static_cast<int>(std::round(transform.y));

                if (gridX >= 0 && gridX < screenWidth && gridY >= 0 && gridY < screenHeight) {
                    buffer[gridY][gridX] = render.glyph;
                }
            }
        }

        // Output rendered frame
        std::cout << "+" << std::string(screenWidth * 2, '-') << "+\n";
        for (int y = 0; y < screenHeight; ++y) {
            std::cout << "| ";
            for (int x = 0; x < screenWidth; ++x) {
                std::cout << buffer[y][x] << " ";
            }
            std::cout << "|\n";
        }
        std::cout << "+" << std::string(screenWidth * 2, '-') << "+\n";

        // Display legend and entity list
        printEntityStats(coordinator);
    }

    void printEntityStats(Coordinator& coordinator) {
        std::cout << " Entities active: " << coordinator.getEntityCount() << " | Status:\n";
        for (auto entity : coordinator.getAllEntities()) {
            if (coordinator.hasComponent<TagComponent>(entity) &&
                coordinator.hasComponent<TransformComponent>(entity)) {
                
                const auto& tag = coordinator.getComponent<TagComponent>(entity);
                const auto& t = coordinator.getComponent<TransformComponent>(entity);
                char glyph = coordinator.hasComponent<RenderComponent>(entity) 
                             ? coordinator.getComponent<RenderComponent>(entity).glyph : '?';

                std::cout << "   - [" << glyph << "] Entity " << entity << " (" << tag.tag << ") at ("
                          << std::fixed << std::setprecision(1) << t.x << ", " << t.y << ")";

                if (coordinator.hasComponent<HealthComponent>(entity)) {
                    const auto& hp = coordinator.getComponent<HealthComponent>(entity);
                    std::cout << " | HP: " << hp.currentHealth << "/" << hp.maxHealth;
                }
                std::cout << "\n";
            }
        }
    }

private:
    int screenWidth;
    int screenHeight;
};

#endif // SYSTEMS_HPP
