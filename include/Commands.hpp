#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <iostream>
#include <vector>
#include <stack>
#include <memory>
#include "ECS.hpp"
#include "Components.hpp"

// ============================================================================
// DESIGN PATTERN: COMMAND PATTERN (Input & Actions with Undo/Redo)
// Encapsulates player/system actions as objects, enabling custom key rebinding,
// input recording, and full Undo/Redo capability.
// ============================================================================

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(Coordinator& coordinator) = 0;
    virtual void undo(Coordinator& coordinator) = 0;
    virtual std::string getDescription() const = 0;
};

// ----------------------------------------------------------------------------
// MoveCommand: Moves entity by (dx, dy), remembers previous position for undo
// ----------------------------------------------------------------------------
class MoveCommand : public ICommand {
public:
    MoveCommand(Entity entity, float deltaX, float deltaY)
        : targetEntity(entity), dx(deltaX), dy(deltaY), prevX(0.0f), prevY(0.0f), executed(false) {}

    void execute(Coordinator& coordinator) override {
        if (coordinator.hasComponent<TransformComponent>(targetEntity)) {
            auto& transform = coordinator.getComponent<TransformComponent>(targetEntity);
            prevX = transform.x;
            prevY = transform.y;
            transform.x += dx;
            transform.y += dy;
            executed = true;
        }
    }

    void undo(Coordinator& coordinator) override {
        if (executed && coordinator.hasComponent<TransformComponent>(targetEntity)) {
            auto& transform = coordinator.getComponent<TransformComponent>(targetEntity);
            transform.x = prevX;
            transform.y = prevY;
            executed = false;
        }
    }

    std::string getDescription() const override {
        return "MoveCommand: entity " + std::to_string(targetEntity) + 
               " shifted by (" + std::to_string(dx) + ", " + std::to_string(dy) + ")";
    }

private:
    Entity targetEntity;
    float dx;
    float dy;
    float prevX;
    float prevY;
    bool executed;
};

// ----------------------------------------------------------------------------
// AttackCommand: Deals damage to target entity, remembers damage for undo
// ----------------------------------------------------------------------------
class AttackCommand : public ICommand {
public:
    AttackCommand(Entity attacker, Entity target, int dmg)
        : attackerEntity(attacker), targetEntity(target), damage(dmg), executed(false) {}

    void execute(Coordinator& coordinator) override {
        if (coordinator.hasComponent<HealthComponent>(targetEntity)) {
            auto& health = coordinator.getComponent<HealthComponent>(targetEntity);
            health.takeDamage(damage);
            executed = true;
        }
    }

    void undo(Coordinator& coordinator) override {
        if (executed && coordinator.hasComponent<HealthComponent>(targetEntity)) {
            auto& health = coordinator.getComponent<HealthComponent>(targetEntity);
            health.currentHealth += damage;
            if (health.currentHealth > health.maxHealth) {
                health.currentHealth = health.maxHealth;
            }
            health.isAlive = (health.currentHealth > 0);
            executed = false;
        }
    }

    std::string getDescription() const override {
        return "AttackCommand: entity " + std::to_string(attackerEntity) + 
               " dealt " + std::to_string(damage) + " damage to entity " + std::to_string(targetEntity);
    }

private:
    Entity attackerEntity;
    Entity targetEntity;
    int damage;
    bool executed;
};

// ----------------------------------------------------------------------------
// CommandManager: Invoker for Command Pattern with Undo / Redo Stacks
// ----------------------------------------------------------------------------
class CommandManager {
public:
    void executeCommand(std::shared_ptr<ICommand> command, Coordinator& coordinator) {
        command->execute(coordinator);
        undoStack.push(command);
        // Clear redo stack on new action
        while (!redoStack.empty()) {
            redoStack.pop();
        }
    }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }

    void undo(Coordinator& coordinator) {
        if (!undoStack.empty()) {
            auto command = undoStack.top();
            undoStack.pop();
            command->undo(coordinator);
            redoStack.push(command);
        }
    }

    void redo(Coordinator& coordinator) {
        if (!redoStack.empty()) {
            auto command = redoStack.top();
            redoStack.pop();
            command->execute(coordinator);
            undoStack.push(command);
        }
    }

private:
    std::stack<std::shared_ptr<ICommand>> undoStack;
    std::stack<std::shared_ptr<ICommand>> redoStack;
};

#endif // COMMANDS_HPP
