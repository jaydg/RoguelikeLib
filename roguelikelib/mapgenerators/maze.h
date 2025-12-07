#pragma once
#ifndef RL_MAZE_H
#define RL_MAZE_H

#include "../map.h"
#include "../randomness.h"
#include <list>

namespace RL {
inline
void CreateMaze(CMap &level, bool allow_loops = false)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear();

    std::list < Position > drillers;
    drillers.emplace_back(level.GetWidth() / 2, level.GetHeight() / 2);

    while(!drillers.empty()) {
        std::list < Position >::iterator m, _m;
        m = drillers.begin();
        _m = drillers.end();

        while(m != _m) {
            bool remove_driller = false;

            switch(Random(4)) {
            case 0:
                if (m->y < 2) {
                    remove_driller = true;
                    break;
                }

                m->y -= 2;

                if(level.GetCell(m->x, m->y) == LevelElementCorridor) {
                    if(!allow_loops || (allow_loops && Random(5))) {
                        remove_driller = true;
                        break;
                    }
                }

                level.SetCell(m->x, m->y + 1, LevelElementCorridor);
                break;

            case 1:
                m->y += 2;

                if(m->y >= level.GetHeight() || level.GetCell(m->x, m->y) == LevelElementCorridor) {
                    remove_driller = true;
                    break;
                }

                level.SetCell(m->x, m->y - 1, LevelElementCorridor);
                break;

            case 2:
                if (m->x < 2) {
                    remove_driller = true;
                    break;
                }
                m->x -= 2;

                if(level.GetCell(m->x, m->y) == LevelElementCorridor) {
                    remove_driller = true;
                    break;
                }

                level.SetCell(m->x + 1, m->y, LevelElementCorridor);
                break;

            case 3:
            default: // make linter happy
                m->x += 2;

                if(m->x >= level.GetWidth() || level.GetCell(m->x, m->y) == LevelElementCorridor) {
                    remove_driller = true;
                    break;
                }

                level.SetCell(m->x - 1, m->y, LevelElementCorridor);
                break;
            }

            if(remove_driller) {
                m = drillers.erase(m);
            } else {
                drillers.emplace_back(m->x, m->y);
                drillers.emplace_back(m->x, m->y);

                level.SetCell(m->x, m->y, LevelElementCorridor);
                ++m;
            }
        }
    }
}

} // end of namespace RL

#endif
