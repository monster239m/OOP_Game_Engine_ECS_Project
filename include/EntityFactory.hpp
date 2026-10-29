#ifndef ENTITY_FACTORY_HPP
#define ENTITY_FACTORY_HPP

#include <string>
#include "ECS.hpp"
#include "Components.hpp"

// ============================================================================
// DESIGN PATTERN: FACTORY & BUILDER PATTERN
// Encapsulates entity assembly with specific component combinations.
// Demonstrates composition over inheritance: different entities are just
// different combinations of components attached to a lightweight entity ID!
// ============================================================================

class EntityFactory {
public:
    // Factory method: Player Archetype
    static Entity createPlayer(Coordinator& coordinator, float startX, float startY, int maxHp = 100) {
        Entity player = coordinator.createEntity();
        coordinator.addComponent(player, TransformComponent(startX, startY));
        coordinator.addComponent(player, VelocityComponent(0.0f, 0.0f, 1.0f));
        coordinator.addComponent(player, RenderComponent('@', "Player", "GREEN"));
        coordinator.addComponent(player, HealthComponent(maxHp));
        coordinator.addComponent(player, ColliderComponent(1.0f, 1.0f, false));
        coordinator.addComponent(player, TagComponent(EntityType::Player, "Hero"));
        return player;
    }

    // Factory method: Enemy Monster Archetype
    static Entity createEnemy(Coordinator& coordinator, float startX, float startY, 
                             float vx = 0.0f, float vy = 0.0f, int hp = 50) {
        Entity enemy = coordinator.createEntity();
        coordinator.addComponent(enemy, TransformComponent(startX, startY));
        coordinator.addComponent(enemy, VelocityComponent(vx, vy, 1.0f));
        coordinator.addComponent(enemy, RenderComponent('E', "Goblin", "RED"));
        coordinator.addComponent(enemy, HealthComponent(hp));
        coordinator.addComponent(enemy, ColliderComponent(1.0f, 1.0f, false));
        coordinator.addComponent(enemy, TagComponent(EntityType::Enemy, "Aggressive Goblin"));
        return enemy;
    }

    // Factory method: Static Wall / Obstacle
    static Entity createObstacle(Coordinator& coordinator, float x, float y) {
        Entity wall = coordinator.createEntity();
        coordinator.addComponent(wall, TransformComponent(x, y));
        // Obstacle has NO Velocity and NO Health: pure composition!
        coordinator.addComponent(wall, RenderComponent('#', "Stone Wall", "GRAY"));
        coordinator.addComponent(wall, ColliderComponent(1.0f, 1.0f, false));
        coordinator.addComponent(wall, TagComponent(EntityType::Obstacle, "Wall"));
        return wall;
    }

    // Factory method: Collectible Coin / Gem
    static Entity createCollectible(Coordinator& coordinator, float x, float y) {
        Entity coin = coordinator.createEntity();
        coordinator.addComponent(coin, TransformComponent(x, y));
        coordinator.addComponent(coin, RenderComponent('$', "Gold Coin", "YELLOW"));
        coordinator.addComponent(coin, ColliderComponent(1.0f, 1.0f, true)); // isTrigger = true
        coordinator.addComponent(coin, TagComponent(EntityType::Collectible, "Treasure"));
        return coin;
    }
};

// ----------------------------------------------------------------------------
// Builder Pattern: Fluent Entity Builder for Custom Dynamic Entities
// ----------------------------------------------------------------------------
class EntityBuilder {
public:
    EntityBuilder(Coordinator& coord) : coordinator(coord), entity(coord.createEntity()) {}

    EntityBuilder& withTransform(float x, float y) {
        coordinator.addComponent(entity, TransformComponent(x, y));
        return *this;
    }

    EntityBuilder& withVelocity(float vx, float vy, float speed = 1.0f) {
        coordinator.addComponent(entity, VelocityComponent(vx, vy, speed));
        return *this;
    }

    EntityBuilder& withRender(char glyph, const std::string& name, const std::string& color = "WHITE") {
        coordinator.addComponent(entity, RenderComponent(glyph, name, color));
        return *this;
    }

    EntityBuilder& withHealth(int maxHp) {
        coordinator.addComponent(entity, HealthComponent(maxHp));
        return *this;
    }

    EntityBuilder& withCollider(float width, float height, bool isTrigger = false) {
        coordinator.addComponent(entity, ColliderComponent(width, height, isTrigger));
        return *this;
    }

    EntityBuilder& withTag(EntityType type, const std::string& tag) {
        coordinator.addComponent(entity, TagComponent(type, tag));
        return *this;
    }

    Entity build() {
        return entity;
    }

private:
    Coordinator& coordinator;
    Entity entity;
};

#endif // ENTITY_FACTORY_HPP
