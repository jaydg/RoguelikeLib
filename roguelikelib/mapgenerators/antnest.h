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

    int x, y;

    level.SetCell(level.GetWidth() / 2, level.GetHeight() / 2, LevelElementCorridor);

    double x1, y1;
    double k;
    double dx, dy;
    int px, py;

    for(int object = 0; object < (int) level.GetWidth() * (int) level.GetHeight() / 3; ++object) {
        // degree
        k = Random(360) * 3.1419532 / 180;
        // position on ellipse by degree
        x1 = (double) level.GetWidth() / 2 + ((double)level.GetWidth() / 2) * sin(k);
        y1 = (double) level.GetHeight() / 2 + ((double)level.GetHeight() / 2) * cos(k);

        // object will move not too horizontal and not too vertival
        do {
            dx = Random(100);
            dy = Random(100);
        } while((abs((int) dx) < 10 && abs((int) dy) < 10));

        dx -= 50;
        dy -= 50;
        dx /= 100;
        dy /= 100;

        int counter = 0;

        while(1) {
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
                    (px < (int) level.GetWidth() - 1 && level.GetCell(px + 1, py) == LevelElementCorridor) ||
                    (py < (int) level.GetHeight() - 1 && level.GetCell(px, py + 1) == LevelElementCorridor)) {
                level.SetCell(px, py, LevelElementCorridor);
                break;
            }
        }
    }

    if(with_rooms) {
        // add halls at the end of corridors
        for(y = 1; y < (int) level.GetHeight() - 1; y++) {
            for(x = 1; x < (int) level.GetWidth() - 1; x++) {
                if((x > (int) level.GetWidth() / 2 - 10 && x < (int) level.GetWidth() / 2 + 10 && y > (int) level.GetHeight() / 2 - 5 && y < (int) level.GetHeight() / 2 + 5) || level.GetCell(x, y) == LevelElementWall) {
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
