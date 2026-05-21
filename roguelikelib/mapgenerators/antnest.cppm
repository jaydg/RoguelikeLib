module;

export module rl.mapgenerators.antnest;

import rl.map;
import rl.maputils;
import rl.position;
import rl.randomness;
import std;

export namespace RL {

void CreateAntNest(CMap &level, bool with_rooms = false)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear(LevelElementWall);

    level.SetCell(level.GetWidth() / 2, level.GetHeight() / 2, LevelElementCorridor);

    double dx, dy;
    int px, py;

    const std::size_t max_objects = level.GetWidth() * level.GetHeight() / 3;

    for(std::size_t object = 0; object < max_objects; ++object) {
        // degree
        double k = static_cast<double>(Random(360)) * std::numbers::pi / 180.0;

        // position on ellipse by degree
        double x1 = static_cast<double>(level.GetWidth()) / 2.0 + (static_cast<double>(level.GetWidth()) / 2.0) * std::sin(k);
        double y1 = static_cast<double>(level.GetHeight()) / 2.0 + (static_cast<double>(level.GetHeight()) / 2.0) * std::cos(k);

        // object will move not too horizontal and not too vertical
        do {
            dx = static_cast<double>(Random(100));
            dy = static_cast<double>(Random(100));
        } while(dx < 10.0 && dy < 10.0);

        dx -= 50.0;
        dy -= 50.0;
        dx /= 100.0;
        dy /= 100.0;

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
            px = static_cast<int>(x1);
            py = static_cast<int>(y1);

            // go through the border to the other side
            if(px < 0) {
                px = static_cast<int>(level.GetWidth()) - 1;
                x1 = static_cast<double>(px);
            }

            if(px > static_cast<int>(level.GetWidth()) - 1) {
                px = 0;
                x1 = static_cast<double>(px);
            }

            if(py < 0) {
                py = static_cast<int>(level.GetHeight()) - 1;
                y1 = static_cast<double>(py);
            }

            if(py > static_cast<int>(level.GetHeight()) - 1) {
                py = 0;
                y1 = static_cast<double>(py);
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
        for(std::size_t y = 1; y < level.GetHeight() - 1; y++) {
            for(std::size_t x = 1; x < level.GetWidth() - 1; x++) {

                if((x > level.GetWidth() / 2 - 10 && x < level.GetWidth() / 2 + 10 &&
                    y > level.GetHeight() / 2 - 5 && y < level.GetHeight() / 2 + 5) ||
                    level.GetCell(x, y) == LevelElementWall) {
                    continue;
                }

                int neighbours = CountNeighboursOfType(level, LevelElementCorridor, Position(x, y));

                if(neighbours == 1) {
                    for(px = -1; px <= 1; px++) {
                        for(py = -1; py <= 1; py++) {
                            level.SetCell(x + px, y + py, LevelElementRoom);
                        }
                    }
                }
            }
        }
    } // end of if (with_rooms)
}

} // end of namespace RL
