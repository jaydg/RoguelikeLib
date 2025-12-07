//////////////////////////////////////////////////////////////////////////
// FOV calculation
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_FOV_H
#define RL_FOV_H

#include "map.h"
#include "position.h"
#include "extern/permissive-fov-cpp.h"

namespace RL {
class FOVContext {
    CMap &level;
    CMap blocked;
public:
    explicit FOVContext(CMap &input_level)
        : level(input_level)
    {
        blocked = input_level;
        level.Clear(0);
    }

    bool isBlocked(short destX, short destY)
    {
        return blocked.GetCell(destX, destY) != false;
    }

    void visit(short destX, short destY) const
    {
        level.SetCell(destX, destY, true);
    }
};

class FOVRoundContext {
    CMap &level;
    CMap blocked;
    Position start;
    int radius;
public:
    FOVRoundContext(CMap &input_level, const Position &a_start, const int &a_radius)
        : level(input_level), start(a_start), radius(a_radius)
    {
        blocked = input_level;
        level.Clear(0);
    }

    bool isBlocked(short destX, short destY)
    {
        if(start.Distance(Position(destX, destY)) >= radius) {
            return true;
        }

        return blocked.GetCell(destX, destY) != false;
    }

    void visit(short destX, short destY) const
    {
        level.SetCell(destX, destY, true);
    }
};

inline
void CalculateFOV(CMap &level, Position start_position, int radius, bool round = true)
{
    if(round) {
        FOVRoundContext context(level, start_position, radius);
        permissive::squareFov(start_position.x, start_position.y, radius, context);
    } else {
        FOVContext context(level);
        permissive::squareFov(start_position.x, start_position.y, radius, context);
    }
}

} // end of namespace RL

#endif
