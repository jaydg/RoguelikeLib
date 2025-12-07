//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_MAP_H
#define RL_MAP_H

#include <limits>
#include <vector>
#include <iostream>

#include "position.h"

namespace RL {

constexpr int LevelElementMax = std::numeric_limits<int>::max();

enum ELevelElement {
    LevelElementWall = '#',
    LevelElementCorridor = '.',
    LevelElementGrass = '"',
    LevelElementPlant = '&',
    LevelElementRoom = ',',
    LevelElementDoorClose = '+',
    LevelElementDoorOpen = '/',
    LevelElementWater = '~',
    LevelElementCorridor_value = LevelElementMax - 2, // Some algorithms (like pathfinding) needs values instead of tiles
    LevelElementRoom_value = LevelElementMax - 1,
    LevelElementWall_value = LevelElementMax
};

class CMap {
private:
    std::vector <int> m_map;
    Size size;
public:
    void Resize(size_t width, size_t height)
    {
        if(width == 0 || height == 0) {
            return;
        }

        size.x = width;
        size.y = height;

        m_map.resize(size.x * size.y);
    }

    void Resize(Size new_size)
    {
        return Resize(new_size.x, new_size.y);
    }

    [[nodiscard]] size_t GetWidth() const
    {
        return size.x;
    }

    [[nodiscard]] size_t GetHeight() const
    {
        return size.y;
    }

    void Clear(int filler)
    {
        if(m_map.empty()) {
            return;
        }

        for(size_t x = 0; x < size.x; ++x)
            for(size_t y = 0; y < size.y; ++y) {
                m_map[x * size.y + y] = filler;
            }
    }

    void Clear(ELevelElement filler = LevelElementWall)
    {
        if(m_map.empty()) {
            return;
        }

        for(size_t x = 0; x < size.x; ++x)
            for(size_t y = 0; y < size.y; ++y) {
                m_map[x * size.y + y] = filler;
            }
    }

    [[nodiscard]] bool OnMap(const size_t& x, const size_t& y) const
    {
        return (x < size.x && y < size.y);
    }

    [[nodiscard]] bool OnMap(const Position &pos) const
    {
        return OnMap(pos.x, pos.y);
    }

    void SetCell(const size_t& x, const size_t& y, ELevelElement element)
    {
        if(OnMap(x, y)) {
            m_map[x * size.y + y] = element;
        }
    }

    void SetCell(const size_t& x, const size_t& y, int element)
    {
        if(OnMap(x, y)) {
            m_map[x * size.y + y] = element;
        }
    }

    void SetCell(const Position &pos, int element)
    {
        return SetCell(pos.x, pos.y, element);
    }

    [[nodiscard]] int GetCell(const size_t& x, const size_t& y) const {
        if(OnMap(x, y)) {
            return m_map[x * size.y + y];
        } else {
            return -1;
        }
    }

    [[nodiscard]] int GetCell(const Position &pos) const
    {
        return GetCell(pos.x, pos.y);
    }

    void PrintMap() const
    {
        for(size_t y = 0; y < size.y; ++y) {
            for(size_t x = 0; x < size.x; ++x) {
                std::cout << static_cast <char> ((int) GetCell(x, y));
            }

            std::cout << std::endl;
        }
    }
};

struct SRoom {
    Position corner1, corner2;
    int type{};

    [[nodiscard]] bool IsInRoom(const Position &pos) const
    {
        return (pos.x >= corner1.x && pos.x <= corner2.x && pos.y >= corner1.y && pos.y <= corner2.y);
    }

    [[nodiscard]] bool IsInRoom(const size_t x, const size_t y) const
    {
        return (x >= corner1.x && x <= corner2.x && y >= corner1.y && y <= corner2.y);
    }
};

} // end of namespace RL

#endif
