#ifndef ROGUELIKE_LIB_H
#define ROGUELIKE_LIB_H

#include <math.h>
#include <limits>
#include <algorithm>
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <iostream>

#pragma warning (disable : 4267) 
using namespace std;

namespace RL {
	struct SPosition {
		int x;
		int y;
		SPosition() { x=-1; y=-1; };
		SPosition(int a_x, int a_y):x(a_x),y(a_y) {};
		SPosition(size_t a_x, size_t a_y)
			:x(static_cast<int>(a_x)),y(static_cast<int>(a_y)) {};
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

	enum ELevelElement {
		LevelElementWall='#',
		LevelElementCorridor='.',
		LevelElementGrass='"',
		LevelElementPlant='&',
		LevelElementRoom=',',
		LevelElementDoorClose='+',
		LevelElementDoorOpen='/',
		LevelElementWater='~',
		LevelElementCorridor_value=INT_MAX-2, // Some algorithms (like pathfinding) needs values instead of tiles
		LevelElementRoom_value=INT_MAX-1,
		LevelElementWall_value=INT_MAX
	};

	class CMap {
	private:
		vector <int> m_map;
		size_t m_width;
		size_t m_height;
	public:
		void Resize(size_t width,size_t height)
		{
			if (width==0 || height==0)
				return;
			m_width  = width;
			m_height = height;

			m_map.resize(m_width*m_height);			
		};

		size_t GetWidth()  { return m_width; };
		size_t GetHeight() { return m_height; };

		void Clear(int filler)
		{
			if (m_map.size()==0)
				return;

			for (size_t x=0;x<m_width;++x)
				for (size_t y=0;y<m_height;++y)
					m_map[x*m_height+y]=filler;
		}
		void Clear(ELevelElement filler=LevelElementWall)
		{
				if (m_map.size()==0)
					return;

				for (size_t x=0;x<m_width;++x)
					for (size_t y=0;y<m_height;++y)
						m_map[x*m_height+y]=filler;
		}

		bool OnMap(const int& x, const int& y)
		{ 
			return (x>=0 && x<(int) m_width && y>=0 && y<(int) m_height); 
		};
		bool OnMap(const SPosition &pos)
		{ 
			return OnMap(pos.x,pos.y);
		};

		void SetCell(const int& x, const int& y, ELevelElement element)
		{ 
			if (OnMap(x,y))
				m_map[x*m_height+y]=element;
		};
		void SetCell(const int& x, const int& y, int element)
		{ 
			if (OnMap(x,y))
				m_map[x*m_height+y]=element;
		};
		void SetCell(const SPosition &pos, int element)
		{ 
			return SetCell(pos.x,pos.y,element);
		};

		int GetCell(const int& x, const int& y)
		{ 
			if (OnMap(x,y))
				return m_map[x*m_height+y];
			else
				return -1;
		};

		int GetCell(const SPosition &pos)
		{ 
			return GetCell(pos.x,pos.y);
		};

		void PrintMap()
		{
			for (size_t y=0;y<m_height;++y)
			{
				for (size_t x=0;x<m_width;++x)
					cout << static_cast <char> ((int) GetCell(x,y));
				cout << endl;
			}
		}
	};

	enum EDirection {
		EDirectionMin=0,
		North=EDirectionMin,
		NorthEast,
		East,
		SouthEast,
		South,
		SouthWest,
		West,
		NorthWest,
		EDirectionMax=NorthWest
	};

	class CDirection {
	private:
		EDirection direction;
	public:
		CDirection():direction(North) {};
		CDirection(EDirection start):direction(start) {};
		inline EDirection Get() { return direction; };
		inline void Set(EDirection to_set) { direction=to_set; };
		inline EDirection Incrase();
		inline EDirection Decrase();
	};

	struct SRoom {		
		SPosition corner1, corner2;
		int type;
		inline bool IsInRoom(const SPosition &pos) { return (pos.x>=corner1.x && pos.x<=corner2.x && pos.y>=corner1.y && pos.y<=corner2.y); };
		inline bool IsInRoom(const int x, const int y) { return (x>=corner1.x && x<=corner2.x && y>=corner1.y && y<=corner2.y); };
	};

//////////////////////////////////////////////////////////////////////////
// Library functions
//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Randomness
	//////////////////////////////////////////////////////////////////////////
	void InitRandomness();
	bool CoinToss();
	bool RandomLowerThatLimit(int limit, int value);
	int Random(int value);

	//////////////////////////////////////////////////////////////////////////
	// Distance calculation
	//////////////////////////////////////////////////////////////////////////
	void InitSquareRoot(int size);
	int Distance(const int& x1,const int& y1,const int& x2,const int& y2);
	EDirection RandomDirection();

	//////////////////////////////////////////////////////////////////////////
	// Map Drawing functions	
	//////////////////////////////////////////////////////////////////////////
	void DrawRectangleOnMap(CMap &level, const SPosition& p1, const SPosition& p2, int value);
	bool FloodFill(CMap &level, SPosition position, int value, bool diagonal=true, int gradient=0, RL::SPosition end=RL::SPosition(-1,-1));
	int FillDisconnectedRoomsWithDifferentValues(CMap &level);

	//////////////////////////////////////////////////////////////////////////
	// Path finding
	//////////////////////////////////////////////////////////////////////////

	// CMap &level - in/out
	// input level must in form of values (higher than floodfill distance)
	bool FindPath(CMap &level, const RL::SPosition& start, const RL::SPosition& end, vector < RL::SPosition >& path, const bool& diagonals=true);

	//////////////////////////////////////////////////////////////////////////
	// FOV calculation
	//////////////////////////////////////////////////////////////////////////
	void CalculateFOV(CMap &level,SPosition start_position,int radius);

	//////////////////////////////////////////////////////////////////////////
	// Map generators
	//////////////////////////////////////////////////////////////////////////
	void CreateStandardDunegon(CMap &level,int max_number_of_rooms=10,bool with_doors=true);
	void CreateMaze(CMap &level,bool allow_loops=false);
	void CreateAntNest(CMap &level,bool with_rooms=false);
	void CreateMines(CMap &level,int max_number_of_rooms=10);
	void CreateCaves(CMap &level,int iterations=1, float density=0.65);
	void CreateSimpleCity(CMap &level,const int& a_number_of_buildings);
	void CreateSpaceShuttle(CMap &level,const int& max_number_of_rooms=15, bool mirror_vertical=false, bool rooms_the_same=true);

	//////////////////////////////////////////////////////////////////////////
	// Map generation helpers
	//////////////////////////////////////////////////////////////////////////
	void FindOnMapAllRectanglesOfType(CMap &level, const RL::ELevelElement& type, const int& size_x, const int& size_y, vector < RL::SPosition >& positions );
	bool FindOnMapRandomRectangleOfType(CMap &level, const RL::ELevelElement& type, RL::SPosition& pos, const int& size_x, const int& size_y);
	int CountNeighboursOfType(CMap &level, ELevelElement type, const SPosition& pos, bool diagonal=true);
	void AddDoors(CMap &level, float door_probability, float open_probability);
	bool AddCorridor(CMap &level, const int& start_x1, const int& start_y1, const int& start_x2, const int& start_y2, bool straight=false);	
	void ConnectClosestRooms(CMap &level, bool with_doors, bool straight_connections=false);
	void AddRecursiveRooms(CMap &level, const RL::ELevelElement& type,int min_size_x,int min_size_y, RL::SRoom room, bool with_doors=true);
	void ConvertValuesToTiles(CMap &level);

} // end of namespace RL

#endif	ROGUELIKE_LIB_H
