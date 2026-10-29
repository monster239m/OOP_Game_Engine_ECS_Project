#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include "ECS.hpp"

// ============================================================================
// DESIGN PATTERN: OBSERVER PATTERN (Decoupled Event Bus)
// Allows systems to broadcast events (e.g. Collision, Damage, Death) without
// direct coupling between systems.
// ============================================================================

struct IEvent {
    virtual ~IEvent() = default;
};

// Event: Fired when two entities collide
struct CollisionEvent : public IEvent {
    Entity entityA;
    Entity entityB;
    float overlapX;
    float overlapY;

    CollisionEvent(Entity a, Entity b, float ox = 0.0f, float oy = 0.0f)
        : entityA(a), entityB(b), overlapX(ox), overlapY(oy) {}
};

// Event: Fired when an entity takes damage or dies
struct EntityDestroyedEvent : public IEvent {
    Entity entity;
    std::string reason;

    EntityDestroyedEvent(Entity e, const std::string& r)
        : entity(e), reason(r) {}
};

// Event: Fired when entity gains or loses health
struct HealthChangedEvent : public IEvent {
    Entity entity;
    int oldHealth;
    int newHealth;

    HealthChangedEvent(Entity e, int oldH, int newH)
        : entity(e), oldHealth(oldH), newHealth(newH) {}
};

// ----------------------------------------------------------------------------
// EventBus: Central Dispatcher for Observer Pattern
// ----------------------------------------------------------------------------
class EventBus {
public:
    using HandlerFunction = std::function<void(const IEvent&)>;

    template <typename EventType>
    void subscribe(std::function<void(const EventType&)> callback) {
        std::type_index typeIndex(typeid(EventType));
        handlers[typeIndex].push_back([callback](const IEvent& event) {
            callback(static_cast<const EventType&>(event));
        });
    }

    template <typename EventType>
    void publish(const EventType& event) {
        std::type_index typeIndex(typeid(EventType));
        if (handlers.find(typeIndex) != handlers.end()) {
            for (auto& handler : handlers[typeIndex]) {
                handler(event);
            }
        }
    }

    void clear() {
        handlers.clear();
    }

private:
    std::unordered_map<std::type_index, std::vector<HandlerFunction>> handlers;
};

#endif // EVENTS_HPP
