#include "ActionGather.h"
#include "core/Entity.h"
#include "mechanics/Player.h"
#include "ActionMove.h"
#include "UnitManager.h"
#include <genie/dat/Unit.h>
#include <limits>

ActionGather::ActionGather(const Unit::Ptr &unit, const Unit::Ptr &target, const genie::Task *task, UnitManager *unitManager) : IAction(Type::Gather, unit),
    m_target(target),
    m_task(task),
    m_unitManager(unitManager)
{

}

bool ActionGather::update(Time time)
{
    Unit::Ptr unit = m_unit.lock();
    if (!unit) {
        WARN << "Unit gone";
        return true;
    }

    Unit::Ptr target = m_target.lock();
    if (!target) {
        WARN << "target gone";
        unit->removeAction(this);
        return true;
    }

    if (!m_prevTime) {
        m_prevTime = time;
        return false;
    }

    genie::ResourceType inputResource = genie::ResourceType(m_task->ResourceIn);
    genie::ResourceType resourceType = inputResource;
    if (m_task->ResourceOut >= 0) {
        resourceType = genie::ResourceType(m_task->ResourceOut);
    }

    if (unit->resources[resourceType] >= unit->data()->ResourceCapacity || target->resources[resourceType] == 0) {
        const MapPos &currentPos = unit->position();

        const Unit::Ptr dropSite = findDropSite(unit);

        if (dropSite) {
            DBG << "moving to" << dropSite->position() << "to drop off, then returning to" << currentPos << "to continue gathering";

            unit->queueAction(MoveOnMap::moveUnitTo(unit, dropSite->position(), m_unitManager->map(), m_unitManager));
            unit->queueAction(std::make_shared<ActionDropOff>(unit, dropSite, m_task));
            unit->queueAction(MoveOnMap::moveUnitTo(unit, currentPos, m_unitManager->map(), m_unitManager));

            if (target->resources[resourceType] > 0) {
                unit->queueAction(std::make_shared<ActionGather>(unit, target, m_task, m_unitManager));
            }
        } else {
            WARN << "failed to find a drop site";
        }

        unit->removeAction(this);

        return true;
    }


    float amount = unit->data()->Action.WorkRate * m_task->WorkValue1;
    if (m_task->ResourceMultiplier >= 0) {
        Player::Ptr player = unit->player.lock();
        if (!player) {
            WARN << "player gone";
            return true;
        }

        amount *= player->resources[genie::ResourceType(m_task->ResourceMultiplier)];
    }

    amount *= (time - m_prevTime) * 0.0015;
    m_prevTime = time;

    amount = std::min(amount, target->resources[resourceType]);

    target->resources[resourceType] -= amount;
    unit->resources[resourceType] += amount;

    return false;
}

Unit::Ptr ActionGather::findDropSite(const Unit::Ptr &unit)
{
    float closestDistance = std::numeric_limits<float>::max();
    MapPos closestPos = unit->position(); // fallback
    Unit::Ptr closestUnit;

    const int dropUnitId1 = unit->data()->Action.DropSite.first;
    const int dropUnitId2 = unit->data()->Action.DropSite.second;

    for (const Unit::Ptr &other : m_unitManager->units()) {
        if (other->data()->ID != dropUnitId1 && other->data()->ID != dropUnitId2) {
            continue;
        }

        const float distance = unit->position().distance(other->position());
        if (distance > closestDistance) {
            continue;
        }

        closestDistance = distance;
        closestPos = other->position();
        closestUnit = other;
    }

    return closestUnit;
}


ActionDropOff::ActionDropOff(const Unit::Ptr &unit, const Unit::Ptr &target, const genie::Task *task) : IAction(Type::DropOff, unit),
    m_target(target),
    m_task(task)
{
}

bool ActionDropOff::update(Time /*time*/)
{
    // TODO check if we need to move closer

    Unit::Ptr unit = m_unit.lock();
    if (!unit) {
        WARN << "Unit gone";
        return true;
    }

    Unit::Ptr target = m_target.lock();
    if (!target) {
        WARN << "target gone";
        unit->removeAction(this);
        return true;
    }

    genie::ResourceType inputResource = genie::ResourceType(m_task->ResourceIn);
    genie::ResourceType resourceType = inputResource;
    if (m_task->ResourceOut >= 0) {
        resourceType = genie::ResourceType(m_task->ResourceOut);
    }
    DBG << "dropping off" << unit->resources[resourceType] << "resources";

    target->resources[resourceType] += unit->resources[resourceType] ;
    unit->resources[resourceType] = 0;

    unit->removeAction(this);

    return false;
}
