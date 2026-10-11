// Terminal rendering of a roguelike map.
module;

export module rtui.map;

export import rl.map;
import rtui.utf8;
import stc;
import std;

export namespace RL
{

void PrintMap(const CMap& map)
{
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            auto tile = map.get(x, y);
            std::cout << stc::rgb_fg(tile.getColor()) << EncodeUTF8(tile.getGlyph());
        }

        std::cout << stc::reset << std::endl;
    }
}

} // namespace RL
