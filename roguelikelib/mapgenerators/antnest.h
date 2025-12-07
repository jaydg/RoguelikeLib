#pragma once
#ifndef RL_ANTNEST_H
#define RL_ANTNEST_H

#include "../map.h"
#include "../randomness.h"

namespace RL {
inline
void CreateAntNest(CMap &level, bool with_rooms = false)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear();

    level.SetCell(level.GetWidth() / 2, level.GetHeight() / 2, LevelElementCorridor);

    double dx, dy;
    int px, py;

    for(size_t object = 0; object < level.GetWidth() * level.GetHeight() / 3; ++object) {
        // degree
        double k = Random(360) * 3.1419532 / 180;
        // position on ellipse by degree
        double x1 = (double) level.GetWidth() / 2 + ((double) level.GetWidth() / 2) * sin(k);
        double y1 = (double) level.GetHeight() / 2 + ((double) level.GetHeight() / 2) * cos(k);

        // object will move not too horizontal and not too vertical
        do {
            dx = Random(100);
            dy = Random(100);
        } while((abs((int) dx) < 10 && abs((int) dy) < 10));

        dx -= 50;
        dy -= 50;
        dx /= 100;
        dy /= 100;

        int counter = 0;

        while(true) {
            // didn't catch anything after 1000 steps (just to avoid infinite loops)
            if(counter++ > 1000) {
                object--;
                break;
            }

            // move object by small step
            x1 += dx;
            y1 += dy;

            // change float to int
            px = (int) x1;
            py = (int) y1;

            // go through the border to the other side
            if(px < 0) {
                px = (int) level.GetWidth() - 1;
                x1 = px;
            }

            if(px > (int) level.GetWidth() - 1) {
                px = 0;
                x1 = px;
            }

            if(py < 0) {
                py = (int) level.GetHeight() - 1;
                y1 = py;
            }

            if(py > (int) level.GetHeight() - 1) {
                py = 0;
                y1 = py;
            }

            // if object has something to catch, then catch it
            if((px > 0 && level.GetCell(px - 1, py) == LevelElementCorridor) ||
                    (py > 0 && level.GetCell(px, py - 1) == LevelElementCorridor) ||
                    (px < static_cast<int>(level.GetWidth()) - 1 && level.GetCell(px + 1, py) == LevelElementCorridor) ||
                    (py < static_cast<int>(level.GetHeight()) - 1 && level.GetCell(px, py + 1) == LevelElementCorridor)) {
                level.SetCell(px, py, LevelElementCorridor);
                break;
            }
        }
    }

    if(with_rooms) {
        // add halls at the end of corridors
        for(size_t y = 1; y < level.GetHeight() - 1; y++) {
            for(size_t x = 1; x < level.GetWidth() - 1; x++) {
                if((x > level.GetWidth() / 2 - 10 && x < level.GetWidth() / 2 + 10 && y > level.GetHeight() / 2 - 5 && y < level.GetHeight() / 2 + 5) || level.GetCell(x, y) == LevelElementWall) {
                    continue;
                }

                int neighbours = CountNeighboursOfType(level, LevelElementCorridor, Position(x, y));

                if(neighbours == 1) {
                    for(px = -1; px <= 1; px++)
                        for(py = -1; py <= 1; py++) {
                            level.SetCell(x + px, y + py, LevelElementRoom);
                        }
                }
            }
        }
    } // end of if (with_rooms)
}

} // end of namespace RL

#endif
