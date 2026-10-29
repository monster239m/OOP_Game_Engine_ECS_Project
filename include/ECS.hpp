#ifndef ECS_HPP
#define ECS_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <queue>
#include <cassert>
#include <algorithm>

// ============================================================================
// ENTITY COMPONENT SYSTEM (ECS) CORE ARCHITECTURE
// Demonstrates:
// 1. Composition over Inheritance
// 2. Cache-Friendly Memory Layout (Contiguous Component Pools via std::vector)
// 3. RAII & Smart Pointers (std::unique_ptr, std::shared_ptr)
// 4. Type Erasure & Run-time Polymorphism for Component Pools
// ============================================================================

using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 1000;
const std::size_t MAX_COMPONENTS = 32;

using ComponentType = std::uint8_t;
using Signature = std::bitset<MAX_COMPONENTS>;

// Helper for type-safe component ID generation at runtime
inline ComponentType getUniqueComponentTypeID() {
    static ComponentType lastID = 0;
    return lastID++;
}

template <typename T>
inline ComponentType getComponentTypeID() noexcept {
    static ComponentType typeID = getUniqueComponentTypeID();
    return typeID;
}

// ----------------------------------------------------------------------------
// EntityManager: Manages recycling of lightweight entity IDs
// ----------------------------------------------------------------------------
class EntityManager {
public:
    EntityManager() : livingEntityCount(0) {
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            availableEntities.push(entity);
        }
    }

    Entity createEntity() {
        assert(livingEntityCount < MAX_ENTITIES && "Too many entities in existence.");
        Entity id = availableEntities.front();
        availableEntities.pop();
        ++livingEntityCount;
        signatures[id].reset();
        return id;
    }

    void destroyEntity(Entity entity) {
        assert(entity < MAX_ENTITIES && "Entity out of range.");
        signatures[entity].reset();
        availableEntities.push(entity);
        --livingEntityCount;
    }

    void setSignature(Entity entity, Signature signature) {
        assert(entity < MAX_ENTITIES && "Entity out of range.");
        signatures[entity] = signature;
    }

    Signature getSignature(Entity entity) const {
        assert(entity < MAX_ENTITIES && "Entity out of range.");
        return signatures[entity];
    }

    std::uint32_t getLivingCount() const { return livingEntityCount; }

private:
    std::queue<Entity> availableEntities;
    std::array<Signature, MAX_ENTITIES> signatures;
    std::uint32_t livingEntityCount;
};

// ----------------------------------------------------------------------------
// Component Pool Interface & Implementation
// Cache-friendly: Stores data tightly packed in std::vector
// ----------------------------------------------------------------------------
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void entityDestroyed(Entity entity) = 0;
};

template <typename T>
class ComponentPool : public IComponentPool {
public:
    void insertData(Entity entity, T component) {
        assert(entityToIndexMap.find(entity) == entityToIndexMap.end() && "Component already added.");

        // Put new entry at end and record the index
        std::size_t newIndex = size;
        entityToIndexMap[entity] = newIndex;
        indexToEntityMap[newIndex] = entity;
        
        if (newIndex >= componentArray.size()) {
            componentArray.resize(newIndex + 16);
        }
        componentArray[newIndex] = component;
        ++size;
    }

    void removeData(Entity entity) {
        assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Removing non-existent component.");

        // Move last element into deleted element's spot to maintain packing
        std::size_t indexOfRemoved = entityToIndexMap[entity];
        std::size_t indexOfLast = size - 1;
        componentArray[indexOfRemoved] = componentArray[indexOfLast];

        Entity entityOfLast = indexToEntityMap[indexOfLast];
        entityToIndexMap[entityOfLast] = indexOfRemoved;
        indexToEntityMap[indexOfRemoved] = entityOfLast;

        entityToIndexMap.erase(entity);
        indexToEntityMap.erase(indexOfLast);
        --size;
    }

    T& getData(Entity entity) {
        assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Retrieving non-existent component.");
        return componentArray[entityToIndexMap[entity]];
    }

    bool hasData(Entity entity) const {
        return entityToIndexMap.find(entity) != entityToIndexMap.end();
    }

    void entityDestroyed(Entity entity) override {
        if (entityToIndexMap.find(entity) != entityToIndexMap.end()) {
            removeData(entity);
        }
    }

    std::size_t getSize() const { return size; }

private:
    std::vector<T> componentArray;
    std::unordered_map<Entity, std::size_t> entityToIndexMap;
    std::unordered_map<std::size_t, Entity> indexToEntityMap;
    std::size_t size{0};
};

// ----------------------------------------------------------------------------
// ComponentManager: Coordinates all component pools
// ----------------------------------------------------------------------------
class ComponentManager {
public:
    template <typename T>
    void registerComponent() {
        ComponentType type = getComponentTypeID<T>();
        if (componentPools.find(type) == componentPools.end()) {
            componentPools[type] = std::make_shared<ComponentPool<T>>();
        }
    }

    template <typename T>
    ComponentType getComponentType() {
        return getComponentTypeID<T>();
    }

    template <typename T>
    void addComponent(Entity entity, T component) {
        getComponentPool<T>()->insertData(entity, component);
    }

    template <typename T>
    void removeComponent(Entity entity) {
        getComponentPool<T>()->removeData(entity);
    }

    template <typename T>
    T& getComponent(Entity entity) {
        return getComponentPool<T>()->getData(entity);
    }

    template <typename T>
    bool hasComponent(Entity entity) {
        return getComponentPool<T>()->hasData(entity);
    }

    void entityDestroyed(Entity entity) {
        for (auto const& pair : componentPools) {
            pair.second->entityDestroyed(entity);
        }
    }

private:
    std::unordered_map<ComponentType, std::shared_ptr<IComponentPool>> componentPools;

    template <typename T>
    std::shared_ptr<ComponentPool<T>> getComponentPool() {
        ComponentType type = getComponentTypeID<T>();
        if (componentPools.find(type) == componentPools.end()) {
            registerComponent<T>();
        }
        return std::static_pointer_cast<ComponentPool<T>>(componentPools[type]);
    }
};

// ----------------------------------------------------------------------------
// Coordinator: Central facade combining EntityManager & ComponentManager
// ----------------------------------------------------------------------------
class Coordinator {
public:
    Coordinator() {
        entityManager = std::make_unique<EntityManager>();
        componentManager = std::make_unique<ComponentManager>();
    }

    Entity createEntity() {
        Entity e = entityManager->createEntity();
        allEntities.push_back(e);
        return e;
    }

    void destroyEntity(Entity entity) {
        entityManager->destroyEntity(entity);
        componentManager->entityDestroyed(entity);
        allEntities.erase(std::remove(allEntities.begin(), allEntities.end(), entity), allEntities.end());
    }

    template <typename T>
    void registerComponent() {
        componentManager->registerComponent<T>();
    }

    template <typename T>
    void addComponent(Entity entity, T component) {
        componentManager->addComponent<T>(entity, component);
        auto signature = entityManager->getSignature(entity);
        signature.set(componentManager->getComponentType<T>(), true);
        entityManager->setSignature(entity, signature);
    }

    template <typename T>
    void removeComponent(Entity entity) {
        componentManager->removeComponent<T>(entity);
        auto signature = entityManager->getSignature(entity);
        signature.set(componentManager->getComponentType<T>(), false);
        entityManager->setSignature(entity, signature);
    }

    template <typename T>
    T& getComponent(Entity entity) {
        return componentManager->getComponent<T>(entity);
    }

    template <typename T>
    bool hasComponent(Entity entity) {
        return componentManager->hasComponent<T>(entity);
    }

    Signature getSignature(Entity entity) {
        return entityManager->getSignature(entity);
    }

    const std::vector<Entity>& getAllEntities() const {
        return allEntities;
    }

    std::uint32_t getEntityCount() const {
        return entityManager->getLivingCount();
    }

private:
    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<ComponentManager> componentManager;
    std::vector<Entity> allEntities;
};

#endif // ECS_HPP
