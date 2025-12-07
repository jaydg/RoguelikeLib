//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_MAP_H
#define RL_MAP_H

#include <climits>
#include <limits>
#include <vector>
#include <iostream>

namespace RL {
struct Position {
    int x;
    int y;
    Position()
    {
        x = -1;
        y = -1;
    }

    Position(int a_x, int a_y): x(a_x), y(a_y) {};

    Position(size_t a_x, size_t a_y)
        : x(static_cast<int>(a_x)), y(static_cast<int>(a_y)) {};

    const Position& operator+=(const Position& r)
    {
        x += r.x;
        y += r.y;
        return *this;
    }

    const Position& operator-=(const Position& r)
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }

    bool operator==(const Position& r)
    {
        if(x == r.x && y == r.y) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const Position& r)
    {
        return !operator==(r);
    }
};

typedef Position Size; // alias name

enum ELevelElement {
    LevelElementWall = '#',
    LevelElementCorridor = '.',
    LevelElementGrass = '"',
    LevelElementPlant = '&',
    LevelElementRoom = ',',
    LevelElementDoorClose = '+',
    LevelElementDoorOpen = '/',
    LevelElementWater = '~',
    LevelElementCorridor_value = INT_MAX - 2, // Some algorithms (like pathfinding) needs values instead of tiles
    LevelElementRoom_value = INT_MAX - 1,
    LevelElementWall_value = INT_MAX
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

    void Resize(Size size)
    {
        return Resize(size.x, size.y);
    }

    size_t GetWidth()
    {
        return size.x;
    }

    size_t GetHeight()
    {
        return size.y;
    }

    void Clear(int filler)
    {
        if(m_map.size() == 0) {
            return;
        }

        for(int x = 0; x < size.x; ++x)
            for(int y = 0; y < size.y; ++y) {
                m_map[x * size.y + y] = filler;
            }
    }

    void Clear(ELevelElement filler = LevelElementWall)
    {
        if(m_map.size() == 0) {
            return;
        }

        for(int x = 0; x < size.x; ++x)
            for(int y = 0; y < size.y; ++y) {
                m_map[x * size.y + y] = filler;
            }
    }

    bool OnMap(const int& x, const int& y)
    {
        return (x >= 0 && x < (int) size.x && y >= 0 && y < (int) size.y);
    }

    bool OnMap(const Position &pos)
    {
        return OnMap(pos.x, pos.y);
    }

    void SetCell(const int& x, const int& y, ELevelElement element)
    {
        if(OnMap(x, y)) {
            m_map[x * size.y + y] = element;
        }
    }

    void SetCell(const int& x, const int& y, int element)
    {
        if(OnMap(x, y)) {
            m_map[x * size.y + y] = element;
        }
    }

    void SetCell(const Position &pos, int element)
    {
        return SetCell(pos.x, pos.y, element);
    }

    int GetCell(const int& x, const int& y)
    {
        if(OnMap(x, y)) {
            return m_map[x * size.y + y];
        } else {
            return -1;
        }
    }

    int GetCell(const Position &pos)
    {
        return GetCell(pos.x, pos.y);
    }

    void PrintMap()
    {
        for(int y = 0; y < size.y; ++y) {
            for(int x = 0; x < size.x; ++x) {
                std::cout << static_cast <char> ((int) GetCell(x, y));
            }

            std::cout << std::endl;
        }
    }
};

struct SRoom {
    Position corner1, corner2;
    int type;

    inline bool IsInRoom(const Position &pos)
    {
        return (pos.x >= corner1.x && pos.x <= corner2.x && pos.y >= corner1.y && pos.y <= corner2.y);
    }

    inline bool IsInRoom(const int x, const int y)
    {
        return (x >= corner1.x && x <= corner2.x && y >= corner1.y && y <= corner2.y);
    }
};

} // end of namespace RL

#endif
