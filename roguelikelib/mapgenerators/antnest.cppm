module;

export module rl.mapgenerators.antnest;

import rl.map;
import rl.maputils;
import rl.position;
import rl.randomness;
import rl.tile;
import std;

export namespace RL {

void CreateAntNest(CMap &level, bool with_rooms = false)
{
    if (level.getWidth() == 0 || level.getHeight() == 0) {
        return;
    }

    level.Clear("wall");

    level.SetCell(level.getWidth() / 2, level.getHeight() / 2, "corridor");

    double dx, dy;
    int px, py;

    const std::size_t max_objects = level.getWidth() * level.getHeight() / 3;

    for (std::size_t object = 0; object < max_objects; ++object) {
        // degree
        double k = static_cast<double>(Random(360)) * std::numbers::pi / 180.0;

        // position on ellipse by degree
        double x1 = static_cast<double>(level.getWidth()) / 2.0 + (static_cast<double>(level.getWidth()) / 2.0) * std::sin(k);
        double y1 = static_cast<double>(level.getHeight()) / 2.0 + (static_cast<double>(level.getHeight()) / 2.0) * std::cos(k);

        // object will move not too horizontal and not too vertical
        do {
            dx = static_cast<double>(Random(100));
            dy = static_cast<double>(Random(100));
        } while (dx < 10.0 && dy < 10.0);

        dx -= 50.0;
        dy -= 50.0;
        dx /= 100.0;
        dy /= 100.0;

        int counter = 0;

        while (true) {
            // didn't catch anything after 1000 steps (just to avoid infinite loops)
            if (counter++ > 1000) {
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
            if (px < 0) {
                px = static_cast<int>(level.getWidth()) - 1;
                x1 = static_cast<double>(px);
            }

            if (px > static_cast<int>(level.getWidth()) - 1) {
                px = 0;
                x1 = static_cast<double>(px);
            }

            if (py < 0) {
                py = static_cast<int>(level.getHeight()) - 1;
                y1 = static_cast<double>(py);
            }

            if(py > static_cast<int>(level.getHeight()) - 1) {
                py = 0;
                y1 = static_cast<double>(py);
            }

            // if object has something to catch, then catch it
            if ((px > 0 && level.get(px - 1, py).getType() == "corridor") ||
               (py > 0 && level.get(px, py - 1).getType() == "corridor") ||
               (px < static_cast<int>(level.getWidth()) - 1 && level.get(px + 1, py).getType() == "corridor") ||
               (py < static_cast<int>(level.getHeight()) - 1 && level.get(px, py + 1).getType() == "corridor")) {

                level.SetCell(px, py, "corridor");
                break;
            }
        }
    }

    if (with_rooms) {
        // add halls at the end of corridors
        for (std::size_t y = 1; y < level.getHeight() - 1; y++) {
            for (std::size_t x = 1; x < level.getWidth() - 1; x++) {

                if ((x > level.getWidth() / 2 - 10 && x < level.getWidth() / 2 + 10 &&
                    y > level.getHeight() / 2 - 5 && y < level.getHeight() / 2 + 5) ||
                    level.get(x, y).getType() == "wall") {
                    continue;
                }

                int neighbours = level.CountNeighbors(Position(x, y), CTile::ByType("corridor"));

                if (neighbours == 1) {
                    for (px = -1; px <= 1; px++) {
                        for (py = -1; py <= 1; py++) {
                            level.SetCell(x + px, y + py, "room");
                        }
                    }
                }
            }
        }
    } // end of if (with_rooms)
}

} // end of namespace RL
