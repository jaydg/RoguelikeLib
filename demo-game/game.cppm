module;

export module demo_game.game;

import demo_game.monster;
import demo_game.player;
export import rl.map;
export import rl.position;
import std;

export class CSimpleGame {
private:

    void PlacePlayer();
    void AddMonster();
    void MoveAllMonsters();

public:
    static constexpr int LEVEL_SIZE_X = 80;
    static constexpr int LEVEL_SIZE_Y = 24;

    RL::CMap level;
    CPlayer player;
    std::list <CMonster*> monsters;
    std::set <CMonster*> monsters_to_remove;
    CMonster *GetMonsterFromCell(const RL::Position& cell);

    void CreateLevel();
    [[noreturn]] void MainLoop();
};

// game is a global singleton
export CSimpleGame game;
