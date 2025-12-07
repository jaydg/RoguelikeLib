#ifndef DEMO_GAME_SIMPLE_GAME_H
#define DEMO_GAME_SIMPLE_GAME_H

#include <set>
#include <list>
#include "player.h"

constexpr int LEVEL_SIZE_X = 80;
constexpr int LEVEL_SIZE_Y = 24;

class CSimpleGame {
private:

    void PlacePlayer();
    void AddMonster();
    void MoveAllMonsters();

public:
    RL::CMap level;
    CPlayer player;
    std::list < CMonster * > monsters;
    std::set < CMonster * > monsters_to_remove;
    static CMonster *GetMonsterFromCell(const RL::Position &cell);

    void CreateLevel();
    [[noreturn]] void MainLoop();
};

#endif
