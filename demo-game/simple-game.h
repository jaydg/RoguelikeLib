#include <set>
#include <list>
#include "player.h"

const int LEVEL_SIZE_X = 80;
const int LEVEL_SIZE_Y = 24;

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
    CMonster *GetMonsterFromCell(const RL::Position &cell);

    void CreateLevel();
    void MainLoop();
};

