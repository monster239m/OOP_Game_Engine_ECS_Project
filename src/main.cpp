#include <iostream>
#include <chrono>
#include <thread>
#include "ECS.hpp"
#include "Components.hpp"
#include "Events.hpp"
#include "Commands.hpp"
#include "Systems.hpp"
#include "EntityFactory.hpp"

// ============================================================================
// OOP C++ SEMESTER MINI PROJECT:
// 2D GAME ENGINE ARCHITECTURE WITH ENTITY COMPONENT SYSTEM (ECS)
// Features Demonstrated:
// - Modern C++ (RAII, Smart Pointers, Type Erasure, Contiguous Component Pools)
// - Polymorphism vs Composition (ECS Architecture)
// - Factory & Builder Patterns (Entity Archetype Creation)
// - Observer Pattern (Decoupled EventBus)
// - Command Pattern (Input Actions with Undo / Redo capability)
// ============================================================================

int main() {
    std::cout << "=================================================================\n";
    std::cout << "    EXTENSIBLE 2D GAME ENGINE (ENTITY COMPONENT SYSTEM - ECS)    \n";
    std::cout << "        Subject: Object-Oriented Programming with C++            \n";
    std::cout << "=================================================================\n\n";

    // 1. Initialise Coordinator (Facade for Entity and Component Management)
    Coordinator coordinator;

    // Register all component types into the contiguous component pools
    coordinator.registerComponent<TransformComponent>();
    coordinator.registerComponent<VelocityComponent>();
    coordinator.registerComponent<RenderComponent>();
    coordinator.registerComponent<HealthComponent>();
    coordinator.registerComponent<ColliderComponent>();
    coordinator.registerComponent<TagComponent>();

    std::cout << "[INIT] Coordinator initialised. Component pools registered.\n";

    // 2. Initialise Event Bus (Observer Pattern)
    EventBus eventBus;
    std::vector<Entity> entitiesToDestroy;

    // Subscribe to EntityDestroyedEvent
    eventBus.subscribe<EntityDestroyedEvent>([&entitiesToDestroy](const EntityDestroyedEvent& event) {
        std::cout << " >>> [EVENT BUS: EntityDestroyed] Entity " << event.entity 
                  << " was destroyed. Reason: " << event.reason << "\n";
        entitiesToDestroy.push_back(event.entity);
    });

    // Subscribe to HealthChangedEvent
    eventBus.subscribe<HealthChangedEvent>([](const HealthChangedEvent& event) {
        std::cout << " >>> [EVENT BUS: HealthChanged] Entity " << event.entity 
                  << " HP changed: " << event.oldHealth << " -> " << event.newHealth << "\n";
    });

    // 3. Initialise Systems
    MovementSystem movementSystem;
    CollisionSystem collisionSystem(eventBus);
    CombatSystem combatSystem(coordinator, eventBus);
    RenderSystem renderSystem(20, 10);
    CommandManager commandManager;

    std::cout << "[INIT] Systems & Event Bus ready.\n\n";

    // 4. Scene Creation via Factory Pattern and Builder Pattern
    std::cout << "-----------------------------------------------------------------\n";
    std::cout << " 1. SCENE CREATION (Factory & Builder Design Patterns)\n";
    std::cout << "-----------------------------------------------------------------\n";

    // Create Player using Factory
    Entity player = EntityFactory::createPlayer(coordinator, 2.0f, 2.0f, 100);
    std::cout << " -> Created Player (ID: " << player << ") at (2, 2) with 100 HP\n";

    // Create Enemy using Factory with moving velocity
    Entity enemy = EntityFactory::createEnemy(coordinator, 6.0f, 2.0f, -0.5f, 0.0f, 60);
    std::cout << " -> Created Goblin Enemy (ID: " << enemy << ") at (6, 2) moving Left\n";

    // Create Obstacle Walls
    Entity wall1 = EntityFactory::createObstacle(coordinator, 10.0f, 3.0f);
    Entity wall2 = EntityFactory::createObstacle(coordinator, 10.0f, 4.0f);
    Entity wall3 = EntityFactory::createObstacle(coordinator, 10.0f, 5.0f);
    std::cout << " -> Created Stone Walls (IDs: " << wall1 << ", " << wall2 << ", " << wall3 << ") at column 10\n";

    // Create Collectible Coin
    Entity coin = EntityFactory::createCollectible(coordinator, 4.0f, 2.0f);
    std::cout << " -> Created Gold Coin (ID: " << coin << ") at (4, 2)\n";

    // Create a Custom Boss using the Builder Pattern (Fluent API)
    Entity boss = EntityBuilder(coordinator)
                    .withTransform(16.0f, 7.0f)
                    .withVelocity(-0.2f, -0.2f, 1.0f)
                    .withRender('B', "Dragon Boss", "PURPLE")
                    .withHealth(300)
                    .withCollider(2.0f, 2.0f, false)
                    .withTag(EntityType::Enemy, "Boss Dragon")
                    .build();
    std::cout << " -> Created Dragon Boss (ID: " << boss << ") via Builder Pattern at (16, 7) with 300 HP\n\n";

    // Render Initial Scene
    std::cout << "[INITIAL SCENE FRAME]\n";
    renderSystem.update(coordinator, 0.0f);

    // 5. Demonstrate Command Pattern with Undo / Redo
    std::cout << "\n-----------------------------------------------------------------\n";
    std::cout << " 2. COMMAND PATTERN DEMONSTRATION (Input Handling with Undo/Redo)\n";
    std::cout << "-----------------------------------------------------------------\n";

    auto& pTrans = coordinator.getComponent<TransformComponent>(player);
    std::cout << " Player original position: (" << pTrans.x << ", " << pTrans.y << ")\n";

    // Execute Move Right Command
    auto moveRight = std::make_shared<MoveCommand>(player, 3.0f, 0.0f);
    std::cout << " Executing: " << moveRight->getDescription() << "\n";
    commandManager.executeCommand(moveRight, coordinator);
    std::cout << " Player position after Move Right: (" << pTrans.x << ", " << pTrans.y << ")\n";

    // Execute Move Down Command
    auto moveDown = std::make_shared<MoveCommand>(player, 0.0f, 2.0f);
    std::cout << " Executing: " << moveDown->getDescription() << "\n";
    commandManager.executeCommand(moveDown, coordinator);
    std::cout << " Player position after Move Down: (" << pTrans.x << ", " << pTrans.y << ")\n";

    // Demonstrate Undo
    std::cout << "\n --- Testing Undo Capability ---\n";
    std::cout << " Invoking UNDO...\n";
    commandManager.undo(coordinator);
    std::cout << " Player position after 1st Undo (reverted Move Down): (" << pTrans.x << ", " << pTrans.y << ")\n";

    std::cout << " Invoking UNDO...\n";
    commandManager.undo(coordinator);
    std::cout << " Player position after 2nd Undo (reverted Move Right): (" << pTrans.x << ", " << pTrans.y << ") [Restored to Origin!]\n";

    // Demonstrate Redo
    std::cout << "\n --- Testing Redo Capability ---\n";
    std::cout << " Invoking REDO...\n";
    commandManager.redo(coordinator);
    std::cout << " Player position after 1st Redo: (" << pTrans.x << ", " << pTrans.y << ")\n";

    // Revert back so player can interact with the coin and enemy in the simulation
    commandManager.undo(coordinator);
    std::cout << " Player reset for simulation at: (" << pTrans.x << ", " << pTrans.y << ")\n\n";

    // 6. Game Loop Simulation
    std::cout << "-----------------------------------------------------------------\n";
    std::cout << " 3. GAME TICK SIMULATION (ECS Systems Execution & Collisions)\n";
    std::cout << "-----------------------------------------------------------------\n";

    // Give player initial velocity towards the coin and enemy
    coordinator.getComponent<VelocityComponent>(player).vx = 1.0f;
    std::cout << " Setting Player velocity: vx = +1.0 (moving towards coin & goblin)\n\n";

    const int TOTAL_TICKS = 6;
    const float DELTA_TIME = 1.0f;

    for (int tick = 1; tick <= TOTAL_TICKS; ++tick) {
        std::cout << "\n>>> [TICK " << tick << " - DELTA TIME: " << DELTA_TIME << "s] <<<\n";

        // System 1: Movement System updates positions
        movementSystem.update(coordinator, DELTA_TIME);

        // System 2: Collision System checks intersections and emits events
        collisionSystem.update(coordinator, DELTA_TIME);

        // Process any entities marked for destruction by events
        for (auto e : entitiesToDestroy) {
            coordinator.destroyEntity(e);
        }
        entitiesToDestroy.clear();

        // System 3: Render current frame
        renderSystem.update(coordinator, DELTA_TIME);
    }

    // 7. Architectural & Memory Verification (RAII & Cache-Friendliness)
    std::cout << "\n-----------------------------------------------------------------\n";
    std::cout << " 4. ARCHITECTURAL VERIFICATION & PERFORMANCE SUMMARY\n";
    std::cout << "-----------------------------------------------------------------\n";
    std::cout << " [1] Composition vs Inheritance:\n";
    std::cout << "     - Entities are pure IDs (uint32_t).\n";
    std::cout << "     - Zero deep class hierarchies (no GameObject -> Actor -> Monster).\n";
    std::cout << "     - Functionality is dynamically added/removed via Component bitsets.\n";
    std::cout << " [2] Memory Management & RAII:\n";
    std::cout << "     - Managed completely by std::unique_ptr and std::shared_ptr.\n";
    std::cout << "     - Destructors automatically release entity pools.\n";
    std::cout << " [3] Cache-Friendly Layout:\n";
    std::cout << "     - Components stored contiguously in std::vector<T> pools.\n";
    std::cout << "     - Dense array packing with packed entity-to-index mapping.\n";
    std::cout << " [4] Design Patterns Implemented:\n";
    std::cout << "     - Observer Pattern (EventBus for CollisionEvent & EntityDestroyedEvent)\n";
    std::cout << "     - Command Pattern (MoveCommand & AttackCommand with Undo/Redo)\n";
    std::cout << "     - Factory & Builder Patterns (EntityFactory & EntityBuilder)\n";
    std::cout << "     - Strategy / Component Pattern (Decoupled systems)\n";
    std::cout << "=================================================================\n";
    std::cout << "       OOP C++ MINI PROJECT EXECUTION COMPLETED SUCCESSFULLY      \n";
    std::cout << "=================================================================\n";

    return 0;
}
