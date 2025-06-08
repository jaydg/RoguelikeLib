#include <math.h>
#include <limits>
#include <algorithm>
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <iostream>

using namespace std;

// every cell is int, for f.e. height map generator
// also without casting ELevelElement can be assigned.
typedef vector < vector <int> > level_t;

namespace RL {
	struct SPosition {
		int x;
		int y;
		SPosition() { x=-1; y=-1; };
		SPosition(int a_x, int a_y):x(a_x),y(a_y) {};
		SPosition(size_t a_x, size_t a_y):x((int) a_x),y((int) a_y) {};
		const SPosition& operator+=(const SPosition& r)
		{
			x+=r.x;
			y+=r.y;
			return *this;
		}
		const SPosition& operator-=(const SPosition& r)
		{
			x-=r.x;
			y-=r.y;
			return *this;
		}	
		bool operator==(const SPosition& r)
		{
			if (x==r.x && y==r.y)
				return true;
			else
				return false;
		}
		bool operator!=(const SPosition& r) { return !operator==(r); }

	};

	enum ELevelElement 
	{
		Wall='#',
		Corridor='.',
		Grass='"',
		Plant='&',
		Room=',',
		DoorClose='+',
		DoorOpen='/',
		Water='~',
		Corridor_value=INT_MAX-2,
		Room_value=INT_MAX-1,
		Wall_value=INT_MAX
	};

	struct SRoom {		
		SPosition corner1, corner2;
		int type;
		inline bool IsInRoom(int x,int y) { return (x>=corner1.x && x<=corner2.x && y>=corner1.y && y<=corner2.y); };
	};
}

class CRoguelikeLibrary {
public:
	CRoguelikeLibrary():m_level_width(0),m_level_height(0) { init_square_root(10000); };
	//////////////////////////////////////////////////////////////////////////
	level_t& CreateStandardDunegon(int max_number_of_rooms=10);
	level_t& CreateMaze(bool allow_loops=false);
	level_t& CreateAntNest(bool with_rooms=false);
	level_t& CreateMines(int max_number_of_rooms=10);
	level_t& CreateCaves(int iterations=1, float density=0.65);
	level_t& CreateSpaceShuttle(const int& max_number_of_rooms=15, bool mirror_vertical=false, bool rooms_the_same=true);
	//////////////////////////////////////////////////////////////////////////

	bool FindPath(const RL::SPosition& start, const RL::SPosition& end, vector < RL::SPosition >& path, const bool& diagonals=true);
	void ClearLevel(RL::ELevelElement filler=RL::Wall);
	void ResizeLevel(size_t width, size_t height);

	void SetLevel(const level_t& a_level) { m_level = a_level; };
	level_t& Level() { return m_level; };

	void PrintLevel();
	inline bool onMap(const int& x, const int& y) { return (x>=0 && x<(int) m_level_width && y>=0 && y<(int) m_level_height); };

private:
	level_t m_level;
	size_t m_level_width;
	size_t m_level_height;

	vector <int> square_root;

	void convert_values_to_tiles();
	bool flood_fill(RL::SPosition position, int value, bool diagonal=true, int gradient=0, RL::SPosition end=RL::SPosition(-1,-1));
	void add_doors(float door_probability, float open_probability);
	bool add_corridor(const int& start_x1, const int& start_y1, const int& start_x2, const int& start_y2, bool straight=false);
	inline int random(int value) { return rand()%value; };
	inline int coin_toss() { return random(2); };
	inline bool random_lower_than_limit(int limit, int value)
	{
		if (value==0)
			return false;
		if (random(value)<limit)
			return true;
		return false;
	}
	inline int distance(const int& x1,const int& y1,const int& x2,const int& y2)
	{

		const int diff_x = x2-x1;
		const int diff_y = y2-y1;
		const int dist = diff_x*diff_x + diff_y*diff_y;

		if (dist>static_cast <int> (square_root.size()))
			init_square_root(dist);
		return square_root[dist];
	}
	void init_square_root(int size);
	bool find_on_map_rectangle_of_type(RL::ELevelElement type, int &px, int &py, const int& size_x, const int& size_y, const int& grid=1);
	int count_neighbours_of_type(RL::ELevelElement type, const RL::SPosition& pos, bool diagonal=true);

	int fill_disconnected_rooms_with_different_values();
	bool connect_closest_rooms(bool with_doors, bool straight_connections=false);
};