#include "RoguelikeLib.h"
#include "permissive-fov-cpp.h"
#include "mt19937int.h"
#include <time.h>

namespace RL {
	//////////////////////////////////////////////////////////////////////////
	// To speed up some operations
	vector <int> square_root;

	//////////////////////////////////////////////////////////////////////////

	void CreateStandardDunegon(CMap &level, int max_number_of_rooms, bool with_doors)
	{
		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		level.Clear();

		SPosition p;
		SPosition room;
		int room_size_x, room_size_y;

		// place rooms
		for (int room_number=0;room_number<max_number_of_rooms;++room_number)
		{
			// size of room
			room_size_x = Random(5)+8;
			room_size_y = Random(5)+5;		
			if (FindOnMapRandomRectangleOfType(level, LevelElementWall, p, room_size_x, room_size_y))
			{
				for (int x=1;x<room_size_x-1;x++)
					for (int y=1;y<room_size_y-1;y++)
						level.SetCell(p.x+x,p.y+y, LevelElementRoom);
			}
		}

		ConnectClosestRooms(level,true,true); // changes tiles to values
		ConvertValuesToTiles(level);
		if (with_doors)
			AddDoors(level,1,0.5);
	}
	
	void ConnectClosestRooms(CMap &level, bool with_doors, bool straight_connections)
	{
		FillDisconnectedRoomsWithDifferentValues(level);

		vector < list < SPosition > > rooms;

		for (size_t y=0;y<level.GetHeight();++y)
		{
			for (size_t x=0;x<level.GetWidth();++x)
			{
				if (level.GetCell(x,y)!=LevelElementWall_value)
				{
					if (level.GetCell(x,y)>=(int) rooms.size())
						rooms.resize(level.GetCell(x,y)+1);

					if (CountNeighboursOfType(level,LevelElementWall_value,SPosition(x,y),false)>0) // only border cells without diagonals
						rooms[ level.GetCell(x,y) ].push_back(SPosition(x,y));
				}
			}
		}

		random_shuffle(rooms.begin(),rooms.end());

		if (rooms.size()<2)
			return;

		// for warshall algorithm
		// set the connection matrix

		vector < vector < bool > > room_connections;
		vector < vector < bool > > transitive_closure;
		vector < vector < int > > distance_matrix;
		vector < vector < pair < SPosition, SPosition >  > > closest_cells_matrix;

		room_connections.resize(rooms.size());
		transitive_closure.resize(rooms.size());
		distance_matrix.resize(rooms.size());
		closest_cells_matrix.resize(rooms.size());

		for (size_t a=0;a<rooms.size();++a)
		{	
			room_connections[a].resize(rooms.size());
			transitive_closure[a].resize(rooms.size());
			distance_matrix[a].resize(rooms.size());
			closest_cells_matrix[a].resize(rooms.size());
			for (size_t b=0;b<rooms.size();++b)
			{
				room_connections[a][b]=false;
				distance_matrix[a][b]=INT_MAX;
			}
		}

		// find the closest cells for each room - Random closest cell
		list < SPosition >::iterator m,_m,k,_k;

		for (int room_a=0;room_a<(int) rooms.size();++room_a)
		{
			for (int room_b=0;room_b<(int) rooms.size();++room_b)
			{
				if (room_a==room_b)
					continue;
				pair < SPosition, SPosition > closest_cells;
				for (m=rooms[room_a].begin(),_m=rooms[room_a].end();m!=_m;++m)
				{
					// for each boder cell in room_a try each border cell of room_b
					int x1 = (*m).x;
					int y1 = (*m).y;

					for (k=rooms[room_b].begin(),_k=rooms[room_b].end();k!=_k;++k)
					{
						int x2 = (*k).x;
						int y2 = (*k).y;

						int dist_ab = Distance(x1,y1,x2,y2);
						if (dist_ab<distance_matrix[room_a][room_b] || (dist_ab==distance_matrix[room_a][room_b] && CoinToss()))
						{
							closest_cells = make_pair( SPosition(x1,y1), SPosition(x2,y2) );
							distance_matrix[room_a][room_b] = dist_ab;
						}
					}
				}
				closest_cells_matrix[room_a][room_b] = closest_cells;
			}
		}

		// Now connect the rooms to the closest ones

		for (int room_a=0;room_a<(int) rooms.size();++room_a)
		{
			int min_distance=INT_MAX;
			int closest_room;
			for (int room_b=0;room_b<(int) rooms.size();++room_b)
			{
				if (room_a==room_b)
					continue;
				int distance = distance_matrix[room_a][room_b];
				if (distance<min_distance)
				{
					min_distance = distance;
					closest_room=room_b;
				}
			}

			// connect room_a to closest one
			pair < SPosition, SPosition > closest_cells;
			closest_cells = closest_cells_matrix[room_a][closest_room];

			int x1=closest_cells.first.x;
			int y1=closest_cells.first.y;
			int x2=closest_cells.second.x;
			int y2=closest_cells.second.y;

			if (room_connections[room_a][closest_room]==false && AddCorridor(level,x1,y1,x2,y2,straight_connections))
			{
				room_connections[room_a][closest_room]=true;
				room_connections[closest_room][room_a]=true;
			}
		}

		// The closest rooms connected. Connect the rest until all areas are connected


		for(int to_connect_a=0;to_connect_a!=-1;)
		{
			size_t a,b,c;
			int to_connect_b;


			for (a=0;a<rooms.size();a++)
				for (b=0;b<rooms.size();b++)
					transitive_closure[a][b] = room_connections[a][b];

			for (a=0;a<rooms.size();a++)
			{
				for (b=0;b<rooms.size();b++)
				{
					if (transitive_closure[a][b]==true && a!=b)
					{
						for (c=0;c<rooms.size();c++)
						{
							if (transitive_closure[b][c]==true)
							{
								transitive_closure[a][c]=true;
								transitive_closure[c][a]=true;
							}
						}
					}
				}
			}

			// Check if all rooms are connected
			to_connect_a=-1;
			for (a=0;a<rooms.size() && to_connect_a==-1;++a)
			{
				for (b=0;b<rooms.size();b++)
				{
					if (a!=b && transitive_closure[a][b]==false)
					{
						to_connect_a=(int) a;
						break;
					}
				}
			}

			if (to_connect_a!=-1)
			{
				// connect rooms a & b
				do {
					to_connect_b = Random((int) rooms.size());
				} while(to_connect_b==to_connect_a);
				pair < SPosition, SPosition > closest_cells;
				closest_cells = closest_cells_matrix[to_connect_a][to_connect_b];

				int x1=closest_cells.first.x;
				int y1=closest_cells.first.y;
				int x2=closest_cells.second.x;
				int y2=closest_cells.second.y;

				AddCorridor(level,x1,y1,x2,y2,straight_connections);

				room_connections[to_connect_a][to_connect_b]=true;
				room_connections[to_connect_b][to_connect_a]=true;
			}
		}
	}

	void ConvertValuesToTiles(CMap &level)
	{
		for (size_t y=0;y<level.GetHeight();++y)
		{
			for (size_t x=0;x<level.GetWidth();++x)
			{
				if (level.GetCell(x,y)==LevelElementCorridor_value)
					level.SetCell(x,y,LevelElementCorridor);
				else if (level.GetCell(x,y)==LevelElementWall_value)
					level.SetCell(x,y,LevelElementWall);
				else 
					level.SetCell(x,y,LevelElementRoom);
			}
		}
	}

	void InitSquareRoot(int size)
	{
		size_t old_size = square_root.size();
		if (size<=0 || static_cast <size_t>(size)<old_size)
			return;
		size++;
		square_root.resize(size);
		// count square root
		for (size_t a=old_size;a<static_cast <size_t>(size);++a)
			square_root[a]=(int )ceil(sqrt((double) a));
	}

	int Distance(const int& x1,const int& y1,const int& x2,const int& y2)
	{

		const int diff_x = x2-x1;
		const int diff_y = y2-y1;
		const int dist = diff_x*diff_x + diff_y*diff_y;

		if (dist>=static_cast <int> (square_root.size()))
			InitSquareRoot(dist);
		return square_root[dist];
	}

	void DrawRectangleOnMap(CMap &level, const SPosition& p1, const SPosition& p2, int value)
	{
		for (int y=p1.y;y<p2.y;++y)
			for (int x=p1.x;x<p2.x;++x)
				level.SetCell(x,y,value);
	}

	void AddRecursiveRooms(CMap &level, const ELevelElement& type,int min_size_x,int min_size_y, SRoom room, bool with_doors)
	{
		int size_x = room.corner2.x - room.corner1.x;
		if (size_x%2!=0)
			size_x-=CoinToss();

		int size_y = room.corner2.y - room.corner1.y;
		if (size_y%2!=0)
			size_y-=CoinToss();

		bool split_horizontal;

		if (size_y*4>size_x)
			split_horizontal=true;
		else if (size_x*4>size_y)
			split_horizontal=false;
		else
		{
			split_horizontal = CoinToss();
		}

		if (split_horizontal) // split horizontal
		{
			if (size_y/2<min_size_y)
				return;
			int split = size_y/2+Random(size_y/2-min_size_y);
			for (int x=room.corner1.x;x<room.corner2.x;x++)
				level.SetCell(x,room.corner1.y+split,type);

			if (with_doors)
			{
				level.SetCell(room.corner1.x+Random(size_x-1)+1,room.corner1.y+split,LevelElementDoorClose);
			}

			SRoom new_room = room;
			new_room.corner2.y = room.corner1.y+split;
			AddRecursiveRooms(level,type,min_size_x,min_size_y,new_room,with_doors);

			new_room = room;
			new_room.corner1.y = room.corner1.y+split;
			AddRecursiveRooms(level,type,min_size_x,min_size_y,new_room,with_doors);
		}
		else
		{
			if (size_x/2<min_size_x)
				return;
			int split = size_x/2+Random(size_x/2-min_size_x);		
			for (int y=room.corner1.y;y<room.corner2.y;y++)
				level.SetCell(room.corner1.x+split,y,type);

			if (with_doors)
				level.SetCell(room.corner1.x+split,room.corner1.y+Random(size_y-1)+1,LevelElementDoorClose);

			SRoom new_room = room;
			new_room.corner2.x = room.corner1.x+split;
			AddRecursiveRooms(level,type,min_size_x,min_size_y,new_room,with_doors);

			new_room = room;
			new_room.corner1.x = room.corner1.x+split;
			AddRecursiveRooms(level,type,min_size_x,min_size_y,new_room,with_doors);
		}
	}

	EDirection operator++( EDirection & e, int )
	{
		e = (( e < EDirectionMax ) ? (EDirection)( e + 1 ) : EDirectionMin) ;
		return e;
	} 

	EDirection operator--( EDirection & e, int )
	{
		e = (( e > EDirectionMin ) ? (EDirection)( e - 1 ) : EDirectionMax ) ;
		return e;
	} 

	inline EDirection CDirection::Incrase() 
	{ 
		direction++;
		return direction; 
	};
	inline EDirection CDirection::Decrase() 
	{ 
		direction--;
		return direction; 
	};

	void CreateSimpleCity(CMap &level,const int& a_number_of_buildings)
	{
		const int min_building_width=5;
		const int max_building_width=10;
		const int min_building_height=5;
		const int max_building_height=10;

		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		for (;;) // until created with proper # of buildings
		{
			level.Clear(LevelElementGrass);

			SRoom main;
			main.corner1.x=0;
			main.corner1.y=0;
			main.corner2.x=(int) level.GetWidth();
			main.corner2.y=(int) level.GetHeight();

			AddRecursiveRooms(level, LevelElementCorridor,max_building_width,max_building_height, main, false);

			int build_count=0;

			int tries=0;
			while(build_count!=a_number_of_buildings && tries<100)
			{
				int size_x=max_building_width*2;
				int size_y=max_building_height*2;

				while(1)
				{
					SPosition pos;
					if (FindOnMapRandomRectangleOfType(level,LevelElementGrass,pos,size_x+2,size_y+2))
					{
						SRoom building, smaller;
						pos.x++;
						pos.y++;
						building.corner1=pos;
						building.corner2=pos;
						building.corner2.x+=size_x;
						building.corner2.y+=size_y;
						smaller = building;
						smaller.corner1.x++;
						smaller.corner1.y++;
						smaller.corner2.x--;
						smaller.corner2.y--;
						DrawRectangleOnMap(level, building.corner1,building.corner2, LevelElementWall);
						DrawRectangleOnMap(level, smaller.corner1,smaller.corner2, LevelElementRoom);
						AddRecursiveRooms(level, LevelElementWall,3,3,smaller);

						// add a doors leading out (improve to lead to nearest road)
						if (CoinToss())
						{
							if (CoinToss())
								level.SetCell(building.corner1.x+Random(size_x-2)+1,building.corner1.y,LevelElementDoorClose);
							else
								level.SetCell(building.corner1.x+Random(size_x-2)+1,building.corner2.y-1,LevelElementDoorClose);
						}
						else
						{
							if (CoinToss())
								level.SetCell(building.corner1.x,building.corner1.y+Random(size_y-2)+1,LevelElementDoorClose);
							else
								level.SetCell(building.corner2.x-1,building.corner1.y+Random(size_y-2)+1,LevelElementDoorClose);
						}


						build_count++;
						if (build_count==a_number_of_buildings)
							break;
					}
					else
					{
						if (CoinToss())
							size_x--;
						else
							size_y--;
						if (size_x<=min_building_width || size_y<=min_building_height)
						{
							tries++;
							break;
						}
					}
				}			
			}
			if (tries<100)
			{
				// plant some trees
				for (int index=0;index<level.GetWidth()*level.GetHeight()*0.3;index++)
				{
					int x=Random(static_cast <int> (level.GetWidth()));
					int y=Random(static_cast <int> (level.GetHeight()));
					if (level.GetCell(x,y)==LevelElementGrass && CountNeighboursOfType(level,LevelElementWall,SPosition(x,y),true)==0)
						level.SetCell(x,y,LevelElementPlant);
				}

				return;
			}
		}
		return;
	}

	void CreateMaze(CMap &level,bool allow_loops)
	{
		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		level.Clear();

		list < SPosition > drillers;
		drillers.push_back(SPosition(level.GetWidth()/2, level.GetHeight()/2));
		while(drillers.size()>0)
		{
			list < SPosition >::iterator m,_m,temp;
			m=drillers.begin();
			_m=drillers.end();
			while (m!=_m)
			{
				bool remove_driller=false;
				switch(Random(4))
				{
				case 0:
					(*m).y-=2;
					if ((*m).y<0 || (level.GetCell((*m).x,(*m).y)==LevelElementCorridor))
					{
						if (!allow_loops || (allow_loops && Random(5)))
						{
							remove_driller=true;
							break;
						}
					}
					level.SetCell((*m).x,(*m).y+1,LevelElementCorridor);
					break;
				case 1:
					(*m).y+=2;
					if ((*m).y>=static_cast <int> (level.GetHeight()) || level.GetCell((*m).x,(*m).y)==LevelElementCorridor)
					{
						remove_driller=true;
						break;
					}
					level.SetCell((*m).x,(*m).y-1,LevelElementCorridor);
					break;
				case 2:
					(*m).x-=2;
					if ((*m).x<0 || level.GetCell((*m).x,(*m).y)==LevelElementCorridor)
					{
						remove_driller=true;
						break;
					}
					level.SetCell((*m).x+1,(*m).y,LevelElementCorridor);
					break;
				case 3:
					(*m).x+=2;
					if ((*m).x>=static_cast <int> (level.GetWidth()) || level.GetCell((*m).x,(*m).y)==LevelElementCorridor)
					{
						remove_driller=true;
						break;
					}
					level.SetCell((*m).x-1,(*m).y,LevelElementCorridor);
					break;
				}
				if (remove_driller)
					m = drillers.erase(m);
				else
				{
					drillers.push_back(SPosition((*m).x,(*m).y));
					drillers.push_back(SPosition((*m).x,(*m).y));

					level.SetCell((*m).x,(*m).y,LevelElementCorridor);
					++m;
				}
			}
		}
	};

	void CreateAntNest(CMap &level,bool with_rooms)
	{
		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		level.Clear();

		int x,y;

		level.SetCell(level.GetWidth()/2,level.GetHeight()/2,LevelElementCorridor);

		double x1,y1;
		double k;
		double dx, dy;
		int px, py;

		for (int object=0;object<(int) level.GetWidth()*(int) level.GetHeight()/3;++object)
		{
			// degree
			k = Random(360)*3.1419532/180;
			// position on ellipse by degree
			x1 = (double) level.GetWidth()/2+((double)level.GetWidth()/2)*sin(k);	
			y1 = (double) level.GetHeight()/2+((double)level.GetHeight()/2)*cos(k);

			// object will move not too horizontal and not too vertival
			do {
				dx=Random(100);
				dy=Random(100);
			} while ((abs((int) dx)<10 && abs((int) dy)<10));
			dx-=50;
			dy-=50;
			dx/=100;
			dy/=100;

			int counter=0;
			while (1)
			{
				// didn't catch anything after 1000 steps (just to avoid infinite loops)
				if (counter++>1000)
				{
					object--;
					break;
				}
				// move object by small step
				x1+=dx;
				y1+=dy;

				// change float to int

				px=(int) x1;
				py=(int) y1;

				// go through the border to the other side

				if (px<0)
				{
					px=(int) level.GetWidth()-1;
					x1=px;
				}
				if (px>(int) level.GetWidth()-1)
				{
					px=0;
					x1=px;
				}
				if (py<0)
				{
					py=(int) level.GetHeight()-1;
					y1=py;
				}
				if (py>(int) level.GetHeight()-1)
				{
					py=0;
					y1=py;
				}

				// if object has something to catch, then catch it

				if ((px>0 && level.GetCell(px-1,py)==LevelElementCorridor) ||
					(py>0 && level.GetCell(px,py-1)==LevelElementCorridor) ||
					(px<(int) level.GetWidth()-1 && level.GetCell(px+1,py)==LevelElementCorridor) ||
					(py<(int) level.GetHeight()-1 && level.GetCell(px,py+1)==LevelElementCorridor))
				{
					level.SetCell(px,py,LevelElementCorridor);
					break;
				}
			}

		}

		if (with_rooms)
		{
			// add halls at the end of corridors
			for (y=1;y<(int) level.GetHeight()-1;y++)
			{
				for (x=1;x<(int) level.GetWidth()-1;x++)
				{
					if ((x>(int) level.GetWidth()/2-10 && x<(int) level.GetWidth()/2+10 && y>(int) level.GetHeight()/2-5 && y<(int) level.GetHeight()/2+5) || level.GetCell(x,y)==LevelElementWall)
						continue;

					int neighbours=CountNeighboursOfType(level,LevelElementCorridor,SPosition(x,y));

					if (neighbours==1)
					{
						for (px=-1;px<=1;px++)
							for (py=-1;py<=1;py++)
							{
								level.SetCell(x+px,y+py,LevelElementRoom);
							}
					}
				}		
			}
		}
	}

	void FindOnMapAllRectanglesOfType(CMap &level, const RL::ELevelElement& type, const int& size_x, const int& size_y, vector < RL::SPosition >& positions )
	{
		CMap good_points;
		good_points = level;

		for (size_t y=0;y<level.GetHeight();++y)
			for (size_t x=0;x<level.GetWidth();++x)
				good_points.SetCell(x,y,0);

		for (size_t y=0;y<level.GetHeight();++y)
		{
			int horizontal_count=0;
			for (size_t x=0;x<level.GetWidth();++x)
			{
				if (level.GetCell(x,y)==type)
					horizontal_count++;
				else
					horizontal_count=0;

				if (horizontal_count==size_x)
				{
					good_points.SetCell(x-size_x+1,y,1);
					horizontal_count--;
				}
			}
		}

		// count verticals

		for (size_t x=0;x<level.GetWidth();++x)
		{
			int vertical_count=0;
			for (size_t y=0;y<level.GetHeight();++y)
			{
				if (good_points.GetCell(x,y)==1)
					vertical_count++;
				else
					vertical_count=0;

				if (vertical_count==size_y)
				{
					positions.push_back(SPosition(x,y-size_y+1));
					vertical_count--;
				}
			}
		}
	}

	bool FindOnMapRandomRectangleOfType(CMap &level, const ELevelElement& type, SPosition& pos, const int& size_x, const int& size_y)
	{
		vector < SPosition > positions;
		FindOnMapAllRectanglesOfType(level,type,size_x,size_y,positions);
		if (positions.size()==0)
			return false;

		// get position of Random rectangle 

		pos = positions[ Random((int) positions.size()) ];	
		return true;
	}

	void CreateMines(CMap &level, int max_number_of_rooms)
	{
		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		level.Clear();

		int x,y,sx,sy;

		typedef list < SRoom > list_of_rooms;

		list_of_rooms :: iterator m;
		list_of_rooms rooms;
		SRoom room;

		int random_number;
		int diff_x, diff_y;

		SPosition p,p1,p2;

		// Place rooms

		for (int room_number=0;room_number<max_number_of_rooms;++room_number)
		{
			// size of room
			sx = Random(5)+6;
			sy = Random(5)+6;		
			if (FindOnMapRandomRectangleOfType(level,LevelElementWall, p, sx+4, sy+4))
			{
				p.x+=2;
				p.y+=2;
				// Connect the room to existing one

				if (rooms.size()>0)
				{

					random_number = Random(static_cast<int>(rooms.size()));
					for (m = rooms.begin();--random_number>0;++m);

					// center of this room
					p1.x= p.x+sx/2;
					p1.y= p.y+sy/2;
					// center of second room
					p2.x= (*m).corner1.x + ((*m).corner2.x - (*m).corner1.x)/2;
					p2.y= (*m).corner1.y + ((*m).corner2.y - (*m).corner1.y)/2;
					// found the way to connect rooms

					diff_x = p2.x-p1.x;
					diff_y = p2.y-p1.y;

					if (diff_x<0)
						diff_x=-diff_x;
					if (diff_y<0)
						diff_y=-diff_y;

					x=p1.x; y=p1.y;

					while (!(diff_x==0 && diff_y==0))
					{
						if (RandomLowerThatLimit(diff_x,diff_x+diff_y)) // move horizontally
						{
							diff_x--;
							if (x>p2.x)
								x--;
							else
								x++;
						}
						else
						{
							diff_y--;
							if (y>p2.y)
								y--;
							else
								y++;
						}
						// Check what is on that position
						if (level.GetCell(x,y)==LevelElementRoom)
							break;
						else if (level.GetCell(x,y)==LevelElementCorridor)
							if (CoinToss())
								break;

						level.SetCell(x,y,LevelElementCorridor);
					}
				}
				// add to list of rooms

				room.corner1.x=p.x;
				room.corner1.y=p.y;
				room.corner2.x=p.x+sx;
				room.corner2.y=p.y+sy;
				room.type = room_number;
				rooms.push_back(room);

				// draw_room

				int room_type = Random(4);
				if (sx==sy)
					room_type=3;

				if (room_type!=2)
				{
					for (y=0;y<sy;y++)
						for (x=0;x<sx;x++)
						{
							switch (room_type)
							{
							case 0: // rectangle room
							case 1: 
								level.SetCell(p.x+x,p.y+y,LevelElementRoom);
								break;
							case 3: // round room
								if (Distance(sx/2,sx/2,x,y)<sx/2)
									level.SetCell(p.x+x,p.y+y,LevelElementRoom);
								break;
							}
						}
				} // end if
				else // typ==2 - Diamond
				{
					for (y=0;y<=sy/2;y++)
						for (x=0;x<=sx/2;x++)
						{
							if (y>=x)
							{
								level.SetCell(p.x+x+sx/2,p.y+y,LevelElementRoom);
								level.SetCell(p.x+x+sx/2,p.y+sy-y,LevelElementRoom);
								level.SetCell(p.x+sx/2-x,p.y+y,LevelElementRoom);
								level.SetCell(p.x+sx/2-x,p.y+sy-y,LevelElementRoom);
							} 
						} 
				} 
			} // end of room addition
		}
	}

	void CreateCaves(CMap &level, int iterations, float density)
	{
		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		level.Clear(LevelElementRoom);

		// create a game of life cave

		int x,y;

		for(int fill=0; fill<(level.GetWidth()*level.GetHeight()*density); fill++)
			level.SetCell(Random((int) level.GetWidth()),Random((int) level.GetHeight()),LevelElementWall);

		for(int iteration=0; iteration<iterations; iteration++)
		{
			for(x=0; x<(int) level.GetWidth(); x++)
			{
				for(y=0; y<(int) level.GetHeight(); y++)
				{
					int neighbours=CountNeighboursOfType(level,LevelElementWall,SPosition(x,y));

					if (level.GetCell(x,y)==LevelElementWall)
					{
						if (neighbours<4)
							level.SetCell(x,y,LevelElementRoom);
					}
					else
					{
						if (neighbours>4)
							level.SetCell(x,y,LevelElementWall);
					}

					if (x==0 || x==(int) level.GetWidth()-1 || y==0 || y==(int) level.GetHeight()-1)
						level.SetCell(x,y,LevelElementWall);
				}
			}
		}

		ConnectClosestRooms(level,true);
		ConvertValuesToTiles(level);
	}

	int CountNeighboursOfType(CMap &level, ELevelElement type, const SPosition& pos, bool diagonal)
	{
		int neighbours=0;
		if (pos.y>0)
			if (level.GetCell(pos.x,pos.y-1)==type) // N
				neighbours++;     

		if (pos.x<(int) level.GetWidth()-1)
			if (level.GetCell(pos.x+1,pos.y)==type) // E
				neighbours++;     

		if (pos.x>0 && pos.y<(int) level.GetHeight()-1)
			if (level.GetCell(pos.x,pos.y+1)==type) // S
				neighbours++;     

		if (pos.x>0 && pos.y>0)
			if (level.GetCell(pos.x-1,pos.y)==type) // W
				neighbours++;     

		if (diagonal)
		{
			if (pos.x>0 && pos.y>0)
				if (level.GetCell(pos.x-1,pos.y-1)==type)  // NW
					neighbours++;  

			if (pos.x<(int) level.GetWidth()-1 && pos.y>0)
				if (level.GetCell(pos.x+1,pos.y-1)==type) // NE
					neighbours++;   

			if (pos.x<(int) level.GetWidth()-1 && pos.y<(int) level.GetHeight()-1) // SE
				if (level.GetCell(pos.x+1,pos.y+1)==type) 
					neighbours++;   


			if (pos.x>0 && pos.y<(int) level.GetHeight()-1)
				if (level.GetCell(pos.x-1,pos.y+1)==type) // SW
					neighbours++;   
		}

		return neighbours;
	}

	int FillDisconnectedRoomsWithDifferentValues(CMap &level)
	{
		for (size_t y=0;y<level.GetHeight();++y)
			for (size_t x=0;x<level.GetWidth();++x)
			{
				if (level.GetCell(x,y)==LevelElementRoom)
					level.SetCell(x,y,LevelElementRoom_value);
				else if (level.GetCell(x,y)==LevelElementWall)
					level.SetCell(x,y,LevelElementWall_value);
			}

			int room_number=0;

			for (size_t y=0;y<level.GetHeight();++y)
			{
				for (size_t x=0;x<level.GetWidth();++x)
				{
					if (level.GetCell(x,y)==LevelElementRoom_value)
					{
						FloodFill(level,SPosition(x,y),room_number++);
					}
				}
			}
			return room_number;
	}

	void CreateSpaceShuttle(CMap &level, const int& max_number_of_rooms, bool mirror_vertical, bool rooms_the_same)
	{
		const int room_min_size=3;
		const int room_max_size=15;

		int number_of_rooms;
		int free_cells;

		list < SRoom > list_of_rooms;
		list < SRoom >::iterator m,_m;
		int x,y;

		int x1,y1,x2,y2, rx, ry;

		while(1) // create until shuttle looks good
		{
			list_of_rooms.clear();

			// fill with walls
			for (x=0;x<(int) level.GetWidth();++x)
				for (y=0;y<(int) level.GetHeight();++y)
					level.SetCell(x,y,LevelElementWall_value);


			for (number_of_rooms=0;number_of_rooms<max_number_of_rooms;)
			{
				if (number_of_rooms==0)
				{
					x1 = (int) level.GetWidth()/2 - Random(room_max_size);
					y1 = (int) level.GetHeight()/2 - Random(room_max_size) - room_min_size;
					rx = Random(room_max_size) + room_min_size;
					ry = Random(room_max_size - room_min_size) + room_min_size;
					x2 = x1+rx;
					y2 = (int) level.GetHeight()/2;
					if (x2 >= (int) level.GetWidth())
						continue;
				}
				else
				{
					x1 = Random((int) level.GetWidth() - room_min_size) + 1;
					y1 = Random((int) level.GetHeight() - room_min_size)/2 + 1;
					rx = Random(room_max_size - room_min_size) + room_min_size;
					ry = Random(room_max_size - room_min_size) + room_min_size;
					x2 = x1+rx;
					y2 = y1+ry;

					if (x2 >= (int) level.GetWidth()-1 || y2 >= (int) level.GetHeight()/2 + 3)
						continue;
				}

				// is in existing room?
				bool rand_again= false;
				for (m = list_of_rooms.begin(), _m = list_of_rooms.end();m!=_m;m++)
				{
					SRoom &room = *m;
					rand_again = true;
					if (room.IsInRoom(x1,y1))
					{
						if (!room.IsInRoom(x2,y2))
						{
							rand_again = false;
							room.type++;
						}
						break;
					}
					if (room.IsInRoom(x2,y2))
					{
						if (!room.IsInRoom(x1,y1))
						{
							rand_again = false;
							room.type++;
						}
						break;
					}
				}
				if (rand_again)
					continue;

				// Create room
				SRoom new_room;
				new_room.corner1.x = x1;
				new_room.corner1.y = y1;
				new_room.corner2.x = x2;
				new_room.corner2.y = y2;
				if (number_of_rooms == 0)
					new_room.type = 0;

				list_of_rooms.push_back(new_room);
				number_of_rooms++;

			} // end of for

			// create mirror
			m = list_of_rooms.begin();
			int index;
			for (index = 0;index<number_of_rooms;index++,m++)
			{
				SRoom room = *m;
				if (mirror_vertical)
				{
					room.corner1.x = (int) level.GetWidth() - room.corner1.x -1;
					room.corner2.x = (int) level.GetWidth() - room.corner2.x -1;		
					x1 = room.corner1.x;
					room.corner1.x = room.corner2.x;
					room.corner2.x = x1;
				}
				else
				{
					room.corner1.y = (int) level.GetHeight() - room.corner1.y -1;
					room.corner2.y = (int) level.GetHeight() - room.corner2.y -1;		
					y1 = room.corner1.y;
					room.corner1.y = room.corner2.y;
					room.corner2.y = y1;
				}
				list_of_rooms.insert(m, room);
			};
			for (m = list_of_rooms.begin(), _m = list_of_rooms.end();m!=_m;m++)
			{
				SRoom &room = *m;
				for (x=room.corner1.x;x<=room.corner2.x;x++)
					for (y=room.corner1.y;y<=room.corner2.y;y++)
					{
						if (level.GetCell(x,y) == LevelElementWall_value)
							level.SetCell(x,y,room.type);
					}			
			}

			// Create walls on connections
			free_cells = 0;
			for(x=0; x<(int) level.GetWidth()-1; x++)
			{
				for(y=0; y<(int) level.GetHeight()/2; y++)
				{
					if (level.GetCell(x,y)!= level.GetCell(x+1,y) && level.GetCell(x+1,y) != LevelElementWall_value)
						level.SetCell(x,y,LevelElementWall_value);
					else if (level.GetCell(x,y) != level.GetCell(x,y+1) && level.GetCell(x,y+1) != LevelElementWall_value)
						level.SetCell(x,y,LevelElementWall_value);
					else if (level.GetCell(x,y) != level.GetCell(x+1,y+1) && level.GetCell(x+1,y+1) != LevelElementWall_value)
						level.SetCell(x,y,LevelElementWall_value);

					if (level.GetCell(x,y)!= LevelElementWall_value)
						free_cells+=2; // +2 for mirror

					level.SetCell(x,level.GetHeight()-y-1, level.GetCell(x,y)); // i odbicie lustrzane
				}
			}

			//	Size of ship

			if (free_cells<(int) level.GetHeight()*(int) level.GetWidth()/4)
				continue;

			ConvertValuesToTiles(level);		
			ConnectClosestRooms(level,true);
			break;
		} 
		ConvertValuesToTiles(level);
		AddDoors(level, 1,0);
	}

	void AddDoors(CMap &level, float door_probability, float open_probability)
	{
		for (size_t x=0;x<level.GetWidth();++x)
			for (size_t y=0;y<level.GetHeight();++y)
			{
				SPosition pos(x,y);
				int room_cells = CountNeighboursOfType(level,LevelElementRoom,pos);
				int corridor_cells = CountNeighboursOfType(level,LevelElementCorridor,pos);
				int open_door_cells = CountNeighboursOfType(level,LevelElementDoorOpen,pos);
				int close_door_cells = CountNeighboursOfType(level,LevelElementDoorClose,pos);
				int door_cells = open_door_cells + close_door_cells;

				if (level.GetCell(x,y)==LevelElementCorridor)
				{
					if ((corridor_cells==1 && door_cells==0 && room_cells>0 && room_cells<4) ||
						(corridor_cells==0 && door_cells==0))
					{
						float exist = ((float) Random(1000))/1000;
						if (exist<door_probability)
						{
							float is_open = ((float) Random(1000))/1000;
							if (is_open<open_probability)
								level.SetCell(x,y,LevelElementDoorOpen);
							else
								level.SetCell(x,y,LevelElementDoorClose);
						}
					}
				} 
			}
	}

	bool FloodFill(CMap &level, SPosition position, int value, bool diagonal, int gradient, SPosition end)
	{
		// flood fill room
		int area_value = level.GetCell(position.x,position.y);
		level.SetCell(position.x,position.y,value);

		list < SPosition > positions;
		positions.push_back(position);

		list < SPosition > ::iterator m,_m;
		m=positions.begin();
		while (m!=positions.end())
		{

			// Fill only to the end?
			if (end.x!=-1 && end==(*m))
				break;

			int pos_x=(*m).x;
			int pos_y=(*m).y;

			int this_value = level.GetCell(pos_x,pos_y);

			if (pos_x>0)
				if (level.GetCell(pos_x-1,pos_y)==area_value)
				{
					level.SetCell(pos_x-1,pos_y,this_value+gradient);
					positions.push_back(SPosition(pos_x-1,pos_y));
				}

				if (pos_x<(int) level.GetWidth()-1)
					if (level.GetCell(pos_x+1,pos_y)==area_value)
					{
						level.SetCell(pos_x+1,pos_y,this_value+gradient);
						positions.push_back(SPosition(pos_x+1,pos_y));
					}

					if (pos_y>0)
						if (level.GetCell(pos_x,pos_y-1)==area_value)
						{
							level.SetCell(pos_x,pos_y-1,this_value+gradient);
							positions.push_back(SPosition(pos_x,pos_y-1));
						}

						if (pos_y<(int) level.GetHeight()-1)
							if (level.GetCell(pos_x,pos_y+1)==area_value)
							{
								level.SetCell(pos_x,pos_y+1,this_value+gradient);
								positions.push_back(SPosition(pos_x,pos_y+1));
							}

							if (diagonal)
							{
								if (pos_x>0 && pos_y>0)
									if (level.GetCell(pos_x-1,pos_y-1)==area_value)
									{
										level.SetCell(pos_x-1,pos_y-1,this_value+gradient);
										positions.push_back(SPosition(pos_x-1,pos_y-1));
									}

									if (pos_x<(int) level.GetWidth()-1 && pos_y<(int) level.GetHeight()-1)
										if (level.GetCell(pos_x+1,pos_y+1)==area_value)
										{
											level.SetCell(pos_x+1,pos_y+1,this_value+gradient);
											positions.push_back(SPosition(pos_x+1,pos_y+1));
										}

										if (pos_x<(int) level.GetWidth()-1 && pos_y>0)
											if (level.GetCell(pos_x+1,pos_y-1)==area_value)
											{
												level.SetCell(pos_x+1,pos_y-1,this_value+gradient);
												positions.push_back(SPosition(pos_x+1,pos_y-1));
											}

											if (pos_x>0 && pos_y<(int) level.GetHeight()-1)
												if (level.GetCell(pos_x-1,pos_y+1)==area_value)
												{
													level.SetCell(pos_x-1,pos_y+1,this_value+gradient);
													positions.push_back(SPosition(pos_x-1,pos_y+1));
												}
							}		
							m=positions.erase(m);
		}
		// filling to the end cell without success
		if (m==positions.end())
			return false;
		if (end.x!=-1 && end!=(*m))
			return false;

		return true;
	}

	bool AddCorridor(CMap &level, const int& start_x1, const int& start_y1, const int& start_x2, const int& start_y2, bool straight)
	{	
		if (!level.OnMap(start_x1,start_y1) || !level.OnMap(start_x2,start_y2))
			return false;
		// we start from both sides 
		int x1,y1,x2,y2;

		x1=start_x1;
		y1=start_y1;
		x2=start_x2;
		y2=start_y2;

		int dir_x;
		int dir_y;

		if (start_x2>start_x1)
			dir_x=1;
		else
			dir_x=-1;

		if (start_y2>start_y1)
			dir_y=1;
		else
			dir_y=-1;


		// move into direction of the other end
		bool first_horizontal=CoinToss();
		bool second_horizontal=CoinToss();

		while(1)
		{
			if (!straight)
			{
				first_horizontal=CoinToss();
				second_horizontal=CoinToss();
			}

			if (x1!=x2 && y1!=y2)
			{
				if (first_horizontal)
					x1+=dir_x;
				else
					y1+=dir_y;
			}
			// connect rooms
			if (x1!=x2 && y1!=y2)
			{
				if (second_horizontal)
					x2-=dir_x;
				else
					y2-=dir_y;
			}

			if (level.GetCell(x1,y1)==LevelElementWall_value)
				level.SetCell(x1,y1,LevelElementCorridor_value);
			if (level.GetCell(x2,y2)==LevelElementWall_value)
				level.SetCell(x2,y2,LevelElementCorridor_value);

			// connect corridors if on the same level
			if (x1==x2)
			{
				while(y1!=y2)
				{
					y1+=dir_y;
					if (level.GetCell(x1,y1)==LevelElementWall_value)
						level.SetCell(x1,y1,LevelElementCorridor_value);
				}
				if (level.GetCell(x1,y1)==LevelElementWall_value)
					level.SetCell(x1,y1,LevelElementCorridor_value);
				return true;
			}
			if (y1==y2)
			{
				while(x1!=x2)
				{
					x1+=dir_x;
					if (level.GetCell(x1,y1)==LevelElementWall_value)
						level.SetCell(x1,y1,LevelElementCorridor_value);
				}
				if (level.GetCell(x1,y1)==LevelElementWall_value)
					level.SetCell(x1,y1,LevelElementCorridor_value);
				return true;
			}
		}
		return true;
	}

	bool FindPath(CMap &level, const SPosition& start, const SPosition& end, vector < SPosition >& path, const bool& diagonals)
	{
		// fill from end to start
		if (!FloodFill(level, end,0,false,1,start))
			return false;

		// walk from start to end
		SPosition pos=start;
		SPosition new_pos=start;

		while(1)
		{
			if (pos==end)
				return true;
			pos = new_pos;

			if (pos!=start)
				path.push_back(pos);

			int current_value = level.GetCell(pos.x,pos.y);

			if (diagonals)
			{
				if (pos.x>0 && pos.y>0)
					if (level.GetCell(pos.x-1,pos.y-1)<current_value)  // NW
					{
						new_pos.x--;     
						new_pos.y--;     
						continue;
					}

					if (pos.x<(int) level.GetWidth()-1 && pos.y>0)
						if (level.GetCell(pos.x+1,pos.y-1)<current_value) // NE
						{
							new_pos.x++;     
							new_pos.y--;     
							continue;
						}

						if (pos.x<(int) level.GetWidth()-1 && pos.y<(int) level.GetHeight()-1) // SE
							if (level.GetCell(pos.x+1,pos.y+1)<current_value) 
							{
								new_pos.x++;     
								new_pos.y++;     
								continue;
							}

							if (pos.x>0 && pos.y<(int) level.GetHeight()-1)
								if (level.GetCell(pos.x-1,pos.y+1)<current_value) // SW
								{
									new_pos.x--;     
									new_pos.y++;     
									continue;
								}
			}

			if (pos.y>0)
				if (level.GetCell(pos.x,pos.y-1)<current_value) // N
				{
					new_pos.y--;     
					continue;
				}

				if (pos.x<(int) level.GetWidth()-1)
					if (level.GetCell(pos.x+1,pos.y)<current_value) // E
					{
						new_pos.x++;     
						continue;
					}

					if (pos.x>0 && pos.y<(int) level.GetHeight()-1)
						if (level.GetCell(pos.x,pos.y+1)<current_value) // S
						{
							new_pos.y++;     
							continue;
						}

						if (pos.x>0 && pos.y>0)
							if (level.GetCell(pos.x-1,pos.y)<current_value) // W
							{
								new_pos.x--;     
								continue;
							}
							if (pos==new_pos && new_pos!=end)
							{
								path.clear();
								return false;
							}
		}
	}

	void InitRandomness(void)
	{
		init_genrand((unsigned int) time(0));
	}
	int Random(int value)
	{ 
		int random_value;
		if (value==0)
			return 0;
		random_value= (int) (((float)genrand_int32() / (float)0xFFFFFFFF)*(value));
		return random_value;
	};
	EDirection RandomDirection() 
	{ 
		return (EDirection) Random(RL::EDirectionMax);
	};

	bool RandomLowerThatLimit(int limit, int value)
	{
		if (value==0)
			return false;
		if (Random(value)<limit)
			return true;
		return false;
	}

	bool CoinToss()
	{ 
		return Random(2)!=0; 
	};

	class FOVContext
	{
		CMap &level;
		CMap blocked;
	 public:
		FOVContext(CMap &input_level)
		:level(input_level)
		{
			blocked=input_level;
			level.Clear(0);
		}
		bool isBlocked(short destX, short destY)
		{
			return blocked.GetCell(destX,destY)!=false;
		}
		void visit(short destX, short destY)
		{
			level.SetCell(destX,destY,true);
		}
	};

	void CalculateFOV(CMap &level,SPosition start_position,int radius)
	{
		FOVContext context(level);
		permissive::squareFov(start_position.x,start_position.y,radius,context);
	}


} // end of namespace RL
