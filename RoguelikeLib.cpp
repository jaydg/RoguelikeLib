#include "RoguelikeLib.h"


void CRoguelikeLibrary::init_square_root(int size)
{
	size_t old_size = square_root.size();
	if (size<=0 || static_cast <size_t>(size)<old_size)
		return;
	square_root.resize(size);
	// count square root
	for (size_t a=old_size;a<static_cast <size_t>(size);++a)
		square_root[a]=(int )ceil(sqrt((double) a));
}

void CRoguelikeLibrary::ResizeLevel(size_t width, size_t height)
{
	if (width==0 || height==0)
		return;
	m_level_width  = width;
	m_level_height = height;

	m_level.resize(m_level_width);
	for (size_t x=0;x<m_level_width;++x)
		m_level[x].resize(m_level_height);
}

void CRoguelikeLibrary::ClearLevel(RL::ELevelElement filler)
{
	if (m_level.size()==0)
		return;

	for (size_t x=0;x<m_level_width;++x)
		for (size_t y=0;y<m_level_height;++y)
			m_level[x][y]=filler;
}

level_t&  CRoguelikeLibrary::CreateMaze(bool allow_loops)
{
	if (m_level_width==0 || m_level_height==0)
		return m_level;

	ClearLevel();

	list < RL::SPosition > drillers;
	drillers.push_back(RL::SPosition(m_level_width/2, m_level_height/2));
	while(drillers.size()>0)
	{
		list < RL::SPosition >::iterator m,_m,temp;
		m=drillers.begin();
		_m=drillers.end();
		while (m!=_m)
		{
			bool remove_driller=false;
			switch(random(4))
			{
			case 0:
				(*m).y-=2;
				if ((*m).y<0 || (m_level[(*m).x][(*m).y]==RL::Corridor))
				{
					if (!allow_loops || (allow_loops && random(5)))
					{
						remove_driller=true;
						break;
					}
				}
				m_level[(*m).x][(*m).y+1]=RL::Corridor;
				break;
			case 1:
				(*m).y+=2;
				if ((*m).y>=static_cast <int> (m_level_height) || m_level[(*m).x][(*m).y]==RL::Corridor)
				{
					remove_driller=true;
					break;
				}
				m_level[(*m).x][(*m).y-1]=RL::Corridor;
				break;
			case 2:
				(*m).x-=2;
				if ((*m).x<0 || m_level[(*m).x][(*m).y]==RL::Corridor)
				{
					remove_driller=true;
					break;
				}
				m_level[(*m).x+1][(*m).y]=RL::Corridor;
				break;
			case 3:
				(*m).x+=2;
				if ((*m).x>=static_cast <int> (m_level_width) || m_level[(*m).x][(*m).y]==RL::Corridor)
				{
					remove_driller=true;
					break;
				}
				m_level[(*m).x-1][(*m).y]=RL::Corridor;
				break;
			}
			if (remove_driller)
				m = drillers.erase(m);
			else
			{
				drillers.push_back(RL::SPosition((*m).x,(*m).y));
				drillers.push_back(RL::SPosition((*m).x,(*m).y));

				m_level[(*m).x][(*m).y]=RL::Corridor;
				++m;
			}
		}
	}
	return m_level;
};

level_t& CRoguelikeLibrary::CreateAntNest(bool with_rooms)
{
	if (m_level_width==0 || m_level_height==0)
		return m_level;

	ClearLevel();

	int x,y;

	m_level[m_level_width/2][m_level_height/2]=RL::Corridor;

	double x1,y1;
	double k;
	double dx, dy;
	int px, py;

	for (int object=0;object<(int) m_level_width*(int) m_level_height/3;++object)
	{
		// degree
		k = random(360)*3.1419532/180;
		// position on ellipse by degree
		x1 = (double) m_level_width/2+((double)m_level_width/2)*sin(k);	
		y1 = (double) m_level_height/2+((double)m_level_height/2)*cos(k);

		// object will move not too horizontal and not too vertival
		do {
			dx=random(100);
			dy=random(100);
		} while ((abs(dx)<10 && abs(dy)<10));
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
				px=(int) m_level_width-1;
				x1=px;
			}
			if (px>(int) m_level_width-1)
			{
				px=0;
				x1=px;
			}
			if (py<0)
			{
				py=(int) m_level_height-1;
				y1=py;
			}
			if (py>(int) m_level_height-1)
			{
				py=0;
				y1=py;
			}

			// if object has something to catch, then catch it

			if ((px>0 && m_level[px-1][py]==RL::Corridor) ||
				(py>0 && m_level[px][py-1]==RL::Corridor) ||
				(px<(int) m_level_width-1 && m_level[px+1][py]==RL::Corridor) ||
				(py<(int) m_level_height-1 && m_level[px][py+1]==RL::Corridor))
			{
				m_level[px][py]=RL::Corridor;
				break;
			}
		}

	}

	if (with_rooms)
	{
		// add halls at the end of corridors
		for (y=1;y<(int) m_level_height-1;y++)
		{
			for (x=1;x<(int) m_level_width-1;x++)
			{
				if ((x>(int) m_level_width/2-10 && x<(int) m_level_width/2+10 && y>(int) m_level_height/2-5 && y<(int) m_level_height/2+5) || m_level[x][y]==RL::Wall)
					continue;

				int neighbours=count_neighbours_of_type(RL::Corridor,RL::SPosition(x,y));

				if (neighbours==1)
				{
					for (px=-1;px<=1;px++)
						for (py=-1;py<=1;py++)
						{
							m_level[x+px][y+py]=RL::Room;
						}
				}
			}		
		}
	}
	return m_level;
}

bool CRoguelikeLibrary::find_on_map_rectangle_of_type(RL::ELevelElement type, int &px, int &py, const int& size_x, const int& size_y, const int& grid)
{
	int x,y;
	bool not_blocked;

	for (int b=0;b<1000;b++)
	{
		not_blocked=true;
		px = random((static_cast <int> (m_level_width)-size_x)/grid)*grid;
		py = random((static_cast <int> (m_level_height)-size_y)/grid)*grid;

		for (y=0;y<=size_y && not_blocked;y++)
		{
			for (x=0;x<=size_x  && not_blocked;x++)
			{
				if (onMap(x+px,y+py))
				{
					if (m_level[x+px][y+py]!=type)
						not_blocked = false;
				}
			} // end of for
		} // end of for
		if (not_blocked)
			break;
	}
	return not_blocked;
}

level_t& CRoguelikeLibrary::CreateStandardDunegon(int max_number_of_rooms)
{
	if (m_level_width==0 || m_level_height==0)
		return m_level;

	ClearLevel();

	int px, py, sx, sy;

	// place rooms
	for (int room_number=0;room_number<max_number_of_rooms;++room_number)
	{
		// size of room
		sx = random(5)+8;
		sy = random(5)+5;		
		if (find_on_map_rectangle_of_type(RL::Wall, px, py, sx+4, sy+4, 1))
		{
			for (int x=1;x<sx-1;x++)
				for (int y=1;y<sy-1;y++)
					if (onMap(px+x,py+y))
						m_level[px+x][py+y]=RL::Room;
		}
	}

	connect_closest_rooms(true,true);
	convert_values_to_tiles();
	add_doors(1,0.5);
	return m_level;
}

level_t& CRoguelikeLibrary::CreateMines(int max_number_of_rooms)
{
	if (m_level_width==0 || m_level_height==0)
		return m_level;

	ClearLevel();

	int x,y,px,py,sx,sy;

	typedef list < RL::SRoom > list_of_rooms;

	list_of_rooms :: iterator m;
	list_of_rooms rooms;
	RL::SRoom room;

	int random_number;
	int diff_x, diff_y;
	int px1,py1,px2,py2;

	// Place rooms

	for (int room_number=0;room_number<max_number_of_rooms;++room_number)
	{
		// size of room
		sx = random(5)+6;
		sy = random(5)+6;		
		if (find_on_map_rectangle_of_type(RL::Wall, px, py, sx+4, sy+4, 1))
		{
			px+=2;
			py+=2;
			// Connect the room to existing one

			if (rooms.size()>0)
			{

				random_number = random(static_cast<int>(rooms.size()));
				for (m = rooms.begin();--random_number>0;++m);

				// center of this room
				px1= px+sx/2;
				py1= py+sy/2;
				// center of second room
				px2= (*m).corner1.x + ((*m).corner2.x - (*m).corner1.x)/2;
				py2= (*m).corner1.y + ((*m).corner2.y - (*m).corner1.y)/2;
				// found the way to connect rooms

				diff_x = px2-px1;
				diff_y = py2-py1;

				if (diff_x<0)
					diff_x=-diff_x;
				if (diff_y<0)
					diff_y=-diff_y;

				x=px1; y=py1;

				while (!(diff_x==0 && diff_y==0))
				{
					if (random_lower_than_limit(diff_x,diff_x+diff_y)) // move horizontally
					{
						diff_x--;
						if (x>px2)
							x--;
						else
							x++;
					}
					else
					{
						diff_y--;
						if (y>py2)
							y--;
						else
							y++;
					}
					// Check what is on that position
					if (m_level[x][y]==RL::Room)
						break;
					else if (m_level[x][y]==RL::Corridor)
						if (coin_toss())
							break;

					m_level[x][y]=RL::Corridor;
				}
			}
			// add to list of rooms

			room.corner1.x=px;
			room.corner1.y=py;
			room.corner2.x=px+sx;
			room.corner2.y=py+sy;
			room.type = room_number;
			rooms.push_back(room);

			// draw_room

			int room_type = random(4);
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
							m_level[px+x][py+y]=RL::Room;
							break;
						case 3: // round room
							if (distance(sx/2,sx/2,x,y)<sx/2)
								m_level[px+x][py+y]=RL::Room;
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
							m_level[px+x+sx/2][py+y]=RL::Room;
							m_level[px+x+sx/2][py+sy-y]=RL::Room;
							m_level[px+sx/2-x][py+y]=RL::Room;
							m_level[px+sx/2-x][py+sy-y]=RL::Room;
						} 
					} 
			} 
		} // end of room addition
	}

	//////////////////////////////////////////////////////////////////////////

	return m_level;
}

level_t& CRoguelikeLibrary::CreateCaves(int iterations, float density)
{
	if (m_level_width==0 || m_level_height==0)
		return m_level;

	ClearLevel(RL::Room);

	// create a game of life cave

	int x,y;

	for(int fill=0; fill<(m_level_width*m_level_height*density); fill++)
		m_level[random((int) m_level_width)][random((int) m_level_height)]=RL::Wall;

	for(int iteration=0; iteration<iterations; iteration++)
	{
		for(x=0; x<(int) m_level_width; x++)
		{
			for(y=0; y<(int) m_level_height; y++)
			{
				int neighbours=count_neighbours_of_type(RL::Wall,RL::SPosition(x,y));

				if (m_level[x][y]==RL::Wall)
				{
					if (neighbours<4)
						this->m_level[x][y]=RL::Room;
				}
				else
				{
					if (neighbours>4)
						this->m_level[x][y]=RL::Wall;
				}

				if (x==0 || x==(int) m_level_width-1 || y==0 || y==(int) m_level_height-1)
					this->m_level[x][y]=RL::Wall;
			}
		}
	}

	connect_closest_rooms(true);
	convert_values_to_tiles();
	return m_level;
}

int CRoguelikeLibrary::count_neighbours_of_type(RL::ELevelElement type, const RL::SPosition& pos, bool diagonal)
{
	int neighbours=0;
	if (pos.y>0)
		if (m_level[pos.x][pos.y-1]==type) // N
			neighbours++;     

	if (pos.x<(int) m_level_width-1)
		if (m_level[pos.x+1][pos.y]==type) // E
			neighbours++;     

	if (pos.x>0 && pos.y<(int) m_level_height-1)
		if (m_level[pos.x][pos.y+1]==type) // S
			neighbours++;     

	if (pos.x>0 && pos.y>0)
		if (m_level[pos.x-1][pos.y]==type) // W
			neighbours++;     

	if (diagonal)
	{
		if (pos.x>0 && pos.y>0)
			if (m_level[pos.x-1][pos.y-1]==type)  // NW
				neighbours++;  

		if (pos.x<(int) m_level_width-1 && pos.y>0)
			if (m_level[pos.x+1][pos.y-1]==type) // NE
				neighbours++;   

		if (pos.x<(int) m_level_width-1 && pos.y<(int) m_level_height-1) // SE
			if (m_level[pos.x+1][pos.y+1]==type) 
				neighbours++;   


		if (pos.x>0 && pos.y<(int) m_level_height-1)
			if (m_level[pos.x-1][pos.y+1]==type) // SW
				neighbours++;   
	}

	return neighbours;
}

level_t& CRoguelikeLibrary::CreateSpaceShuttle(const int& max_number_of_rooms, bool mirror_vertical, bool rooms_the_same)
{
	const int room_min_size=3;
	const int room_max_size=15;

	int number_of_rooms;
	int free_cells;

	list < RL::SRoom > list_of_rooms;
	list < RL::SRoom >::iterator m,_m;
	int x,y;

	int x1,y1,x2,y2, rx, ry;

	while(1) // create until shuttle looks good
	{
		list_of_rooms.clear();

		// fill with walls
		for (x=0;x<(int) m_level_width;++x)
			for (y=0;y<(int) m_level_height;++y)
				m_level[x][y]=RL::Wall_value;


		for (number_of_rooms=0;number_of_rooms<max_number_of_rooms;)
		{
			if (number_of_rooms==0)
			{
				x1 = (int) m_level_width/2 - random(room_max_size);
				y1 = (int) m_level_height/2 - random(room_max_size) - room_min_size;
				rx = random(room_max_size) + room_min_size;
				ry = random(room_max_size - room_min_size) + room_min_size;
				x2 = x1+rx;
				y2 = (int) m_level_height/2;
				if (x2 >= (int) m_level_width)
					continue;
			}
			else
			{
				x1 = random((int) m_level_width - room_min_size) + 1;
				y1 = random((int) m_level_height - room_min_size)/2 + 1;
				rx = random(room_max_size - room_min_size) + room_min_size;
				ry = random(room_max_size - room_min_size) + room_min_size;
				x2 = x1+rx;
				y2 = y1+ry;

				if (x2 >= (int) m_level_width-1 || y2 >= (int) m_level_height/2 + 3)
					continue;
			}

			// is in existing room?
			bool rand_again= false;
			for (m = list_of_rooms.begin(), _m = list_of_rooms.end();m!=_m;m++)
			{
				RL::SRoom &room = *m;
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
			RL::SRoom new_room;
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
			RL::SRoom room = *m;
			if (mirror_vertical)
			{
				room.corner1.x = (int) m_level_width - room.corner1.x -1;
				room.corner2.x = (int) m_level_width - room.corner2.x -1;		
				x1 = room.corner1.x;
				room.corner1.x = room.corner2.x;
				room.corner2.x = x1;
			}
			else
			{
				room.corner1.y = (int) m_level_height - room.corner1.y -1;
				room.corner2.y = (int) m_level_height - room.corner2.y -1;		
				y1 = room.corner1.y;
				room.corner1.y = room.corner2.y;
				room.corner2.y = y1;
			}
			list_of_rooms.insert(m, room);
		};
		for (m = list_of_rooms.begin(), _m = list_of_rooms.end();m!=_m;m++)
		{
			RL::SRoom &room = *m;
			for (x=room.corner1.x;x<=room.corner2.x;x++)
				for (y=room.corner1.y;y<=room.corner2.y;y++)
				{
					if (m_level[x][y] == RL::Wall_value)
						m_level[x][y] = room.type;
				}			
		}

		// Create walls on connections
		free_cells = 0;
		for(x=0; x<(int) m_level_width-1; x++)
		{
			for(y=0; y<(int) m_level_height/2; y++)
			{
				if (m_level[x][y]!= m_level[x+1][y] && m_level[x+1][y] != RL::Wall_value)
					m_level[x][y]= RL::Wall_value;
				else if (m_level[x][y] != m_level[x][y+1] && m_level[x][y+1] != RL::Wall_value)
					m_level[x][y]= RL::Wall_value;
				else if (m_level[x][y] != m_level[x+1][y+1] && m_level[x+1][y+1] != RL::Wall_value)
					m_level[x][y]= RL::Wall_value;

				if (m_level[x][y]!= RL::Wall_value)
					free_cells+=2; // +2 for mirror

				m_level[x][m_level_height-y-1] = m_level[x][y]; // i odbicie lustrzane
			}
		}

		//	Size of ship

		if (free_cells<(int) m_level_height*(int) m_level_width/4)
			continue;

		convert_values_to_tiles();		
		if (connect_closest_rooms(true))
			break;
	} 
	convert_values_to_tiles();
	add_doors(1,0);
	return m_level;
}

void CRoguelikeLibrary::add_doors(float door_probability, float open_probability)
{
	for (size_t x=0;x<m_level_width;++x)
		for (size_t y=0;y<m_level_height;++y)
		{
			RL::SPosition pos(x,y);
			int room_cells = count_neighbours_of_type(RL::Room,pos);
			int corridor_cells = count_neighbours_of_type(RL::Corridor,pos);
			int open_door_cells = count_neighbours_of_type(RL::DoorOpen,pos);
			int close_door_cells = count_neighbours_of_type(RL::DoorClose,pos);
			int door_cells = open_door_cells + close_door_cells;

			if (m_level[x][y]==RL::Corridor)
			{
				if ((corridor_cells==1 && door_cells==0 && room_cells>0 && room_cells<4) ||
					(corridor_cells==0 && door_cells==0))
				{
					float exist = ((float) random(1000))/1000;
					if (exist<door_probability)
					{
						float is_open = ((float) random(1000))/1000;
						if (is_open<open_probability)
							m_level[x][y]=RL::DoorOpen;
						else
							m_level[x][y]=RL::DoorClose;
					}
				}
			} // end if (m_level[x][y]==RL::Corridor)
		}
}

int CRoguelikeLibrary::fill_disconnected_rooms_with_different_values()
{
	for (size_t y=0;y<m_level_height;++y)
		for (size_t x=0;x<m_level_width;++x)
		{
			if (m_level[x][y]==RL::Room)
				m_level[x][y]=RL::Room_value;
			else if (m_level[x][y]==RL::Wall)
				m_level[x][y]=RL::Wall_value;
		}

		int room_number=0;

		for (size_t y=0;y<m_level_height;++y)
		{
			for (size_t x=0;x<m_level_width;++x)
			{
				if (m_level[x][y]==RL::Room_value)
				{
					flood_fill(RL::SPosition(x,y),room_number++);
				}
			}
		}
		return room_number;
}

bool CRoguelikeLibrary::flood_fill(RL::SPosition position, int value, bool diagonal, int gradient, RL::SPosition end)
{
	// flood fill room
	int area_value = m_level[position.x][position.y];
	m_level[position.x][position.y] = value;

	list < RL::SPosition > positions;
	positions.push_back(position);

	list < RL::SPosition > ::iterator m,_m;
	m=positions.begin();
	while (m!=positions.end())
	{

		// Fill only to the end?
		if (end.x!=-1 && end==(*m))
			break;

		int pos_x=(*m).x;
		int pos_y=(*m).y;

		int this_value = m_level[pos_x][pos_y];

		if (pos_x>0)
			if (m_level[pos_x-1][pos_y]==area_value)
			{
				m_level[pos_x-1][pos_y]=this_value+gradient;
				positions.push_back(RL::SPosition(pos_x-1,pos_y));
			}

		if (pos_x<(int) m_level_width-1)
			if (m_level[pos_x+1][pos_y]==area_value)
			{
				m_level[pos_x+1][pos_y]=this_value+gradient;
				positions.push_back(RL::SPosition(pos_x+1,pos_y));
			}

		if (pos_y>0)
			if (m_level[pos_x][pos_y-1]==area_value)
			{
				m_level[pos_x][pos_y-1]=this_value+gradient;
				positions.push_back(RL::SPosition(pos_x,pos_y-1));
			}

		if (pos_y<(int) m_level_height-1)
			if (m_level[pos_x][pos_y+1]==area_value)
			{
				m_level[pos_x][pos_y+1]=this_value+gradient;
				positions.push_back(RL::SPosition(pos_x,pos_y+1));
			}

		if (diagonal)
		{
			if (pos_x>0 && pos_y>0)
				if (m_level[pos_x-1][pos_y-1]==area_value)
				{
					m_level[pos_x-1][pos_y-1]=this_value+gradient;
					positions.push_back(RL::SPosition(pos_x-1,pos_y-1));
				}

			if (pos_x<(int) m_level_width-1 && pos_y<(int) m_level_height-1)
				if (m_level[pos_x+1][pos_y+1]==area_value)
				{
					m_level[pos_x+1][pos_y+1]=this_value+gradient;
					positions.push_back(RL::SPosition(pos_x+1,pos_y+1));
				}

			if (pos_x<(int) m_level_width-1 && pos_y>0)
				if (m_level[pos_x+1][pos_y-1]==area_value)
				{
					m_level[pos_x+1][pos_y-1]=this_value+gradient;
					positions.push_back(RL::SPosition(pos_x+1,pos_y-1));
				}

			if (pos_x>0 && pos_y<(int) m_level_height-1)
				if (m_level[pos_x-1][pos_y+1]==area_value)
				{
					m_level[pos_x-1][pos_y+1]=this_value+gradient;
					positions.push_back(RL::SPosition(pos_x-1,pos_y+1));
				}
		}		
		m=positions.erase(m);
	}
	// filling to the end cell without success
	if (end.x!=-1 && end!=(*m))
		return false;

	return true;
}

bool CRoguelikeLibrary::connect_closest_rooms(bool with_doors, bool straight_connections)
{
	fill_disconnected_rooms_with_different_values();

	vector < list < RL::SPosition > > rooms;

	for (size_t y=0;y<m_level_height;++y)
	{
		for (size_t x=0;x<m_level_width;++x)
		{
			if (m_level[x][y]!=RL::Wall_value)
			{
				if (m_level[x][y]>=(int) rooms.size())
					rooms.resize(m_level[x][y]+1);

				if (count_neighbours_of_type(RL::Wall_value,RL::SPosition(x,y),false)>0) // only border cells without diagonals
					rooms[ m_level[x][y] ].push_back(RL::SPosition(x,y));
			}
		}
	}

	random_shuffle(rooms.begin(),rooms.end());

	if (rooms.size()<2)
		return true;

	// for warshall algorithm
	// set the connection matrix

	vector < vector < bool > > room_connections;
	vector < vector < bool > > transitive_closure;
	vector < vector < int > > distance_matrix;
	vector < vector < pair < RL::SPosition, RL::SPosition >  > > closest_cells_matrix;

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

	// find the closest cells for each room - random closest cell
	list < RL::SPosition >::iterator m,_m,k,_k;

	for (int room_a=0;room_a<(int) rooms.size();++room_a)
	{
		for (int room_b=0;room_b<(int) rooms.size();++room_b)
		{
			if (room_a==room_b)
				continue;
			pair < RL::SPosition, RL::SPosition > closest_cells;
			for (m=rooms[room_a].begin(),_m=rooms[room_a].end();m!=_m;++m)
			{
				// for each boder cell in room_a try each border cell of room_b
				int x1 = (*m).x;
				int y1 = (*m).y;

				for (k=rooms[room_b].begin(),_k=rooms[room_b].end();k!=_k;++k)
				{
					int x2 = (*k).x;
					int y2 = (*k).y;

					int dist_ab = distance(x1,y1,x2,y2);
					if (dist_ab<distance_matrix[room_a][room_b] || (dist_ab==distance_matrix[room_a][room_b] && coin_toss()))
					{
						closest_cells = make_pair( RL::SPosition(x1,y1), RL::SPosition(x2,y2) );
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
		pair < RL::SPosition, RL::SPosition > closest_cells;
		closest_cells = closest_cells_matrix[room_a][closest_room];

		int x1=closest_cells.first.x;
		int y1=closest_cells.first.y;
		int x2=closest_cells.second.x;
		int y2=closest_cells.second.y;

		if (room_connections[room_a][closest_room]==false && add_corridor(x1,y1,x2,y2,straight_connections))
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
				to_connect_b = random((int) rooms.size());
			} while(to_connect_b==to_connect_a);
			pair < RL::SPosition, RL::SPosition > closest_cells;
			closest_cells = closest_cells_matrix[to_connect_a][to_connect_b];

			int x1=closest_cells.first.x;
			int y1=closest_cells.first.y;
			int x2=closest_cells.second.x;
			int y2=closest_cells.second.y;

			add_corridor(x1,y1,x2,y2,straight_connections);

			room_connections[to_connect_a][to_connect_b]=true;
			room_connections[to_connect_b][to_connect_a]=true;
		}
	}

	return true;
}

bool CRoguelikeLibrary::add_corridor(const int& start_x1, const int& start_y1, const int& start_x2, const int& start_y2, bool straight)
{	
	if (!onMap(start_x1,start_y1) || !onMap(start_x2,start_y2))
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
	bool first_horizontal=coin_toss();
	bool second_horizontal=coin_toss();

	while(1)
	{
		if (!straight)
		{
			first_horizontal=coin_toss();
			second_horizontal=coin_toss();
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

		if (m_level[x1][y1]==RL::Wall_value)
			m_level[x1][y1]=RL::Corridor_value;
		if (m_level[x2][y2]==RL::Wall_value)
			m_level[x2][y2]=RL::Corridor_value;

		// connect corridors if on the same level
		if (x1==x2)
		{
			while(y1!=y2)
			{
				y1+=dir_y;
				if (m_level[x1][y1]==RL::Wall_value)
					m_level[x1][y1]=RL::Corridor_value;
			}
			if (m_level[x1][y1]==RL::Wall_value)
				m_level[x1][y1]=RL::Corridor_value;
			return true;
		}
		if (y1==y2)
		{
			while(x1!=x2)
			{
				x1+=dir_x;
				if (m_level[x1][y1]==RL::Wall_value)
					m_level[x1][y1]=RL::Corridor_value;
			}
			if (m_level[x1][y1]==RL::Wall_value)
				m_level[x1][y1]=RL::Corridor_value;
			return true;
		}
	}
	return true;
}


void CRoguelikeLibrary::convert_values_to_tiles()
{
	for (size_t y=0;y<m_level_height;++y)
	{
		for (size_t x=0;x<m_level_width;++x)
		{
			if (m_level[x][y]==RL::Corridor_value)
				m_level[x][y]=RL::Corridor;
			else if (m_level[x][y]==RL::Wall_value)
				m_level[x][y]=RL::Wall;
			else 
				m_level[x][y]=RL::Room;
		}
	}
}


void CRoguelikeLibrary::PrintLevel()
{
	for (size_t y=0;y<m_level_height;++y)
	{
		for (size_t x=0;x<m_level_width;++x)
			cout << static_cast <char> (m_level[x][y]);
		cout << endl;
	}
}

bool CRoguelikeLibrary::FindPath(const RL::SPosition& start, const RL::SPosition& end, vector < RL::SPosition >& path, const bool& diagonals)
{
	// fill from end to start
	if (!flood_fill(end,0,false,1,start))
		return false;

	// walk from start to end
	RL::SPosition pos=start;
	RL::SPosition new_pos=start;

	while(1)
	{
		if (pos==end)
			return true;
		pos = new_pos;

		if (pos!=start)
			path.push_back(pos);

		int current_value = m_level[pos.x][pos.y];

		if (diagonals)
		{
			if (pos.x>0 && pos.y>0)
				if (m_level[pos.x-1][pos.y-1]<current_value)  // NW
				{
					new_pos.x--;     
					new_pos.y--;     
					continue;
				}

			if (pos.x<(int) m_level_width-1 && pos.y>0)
				if (m_level[pos.x+1][pos.y-1]<current_value) // NE
				{
					new_pos.x++;     
					new_pos.y--;     
					continue;
				}

			if (pos.x<(int) m_level_width-1 && pos.y<(int) m_level_height-1) // SE
				if (m_level[pos.x+1][pos.y+1]<current_value) 
				{
					new_pos.x++;     
					new_pos.y++;     
					continue;
				}

			if (pos.x>0 && pos.y<(int) m_level_height-1)
				if (m_level[pos.x-1][pos.y+1]<current_value) // SW
				{
					new_pos.x--;     
					new_pos.y++;     
					continue;
				}
		}

		if (pos.y>0)
			if (m_level[pos.x][pos.y-1]<current_value) // N
			{
				new_pos.y--;     
				continue;
			}
		
		if (pos.x<(int) m_level_width-1)
			if (m_level[pos.x+1][pos.y]<current_value) // E
			{
				new_pos.x++;     
				continue;
			}

		if (pos.x>0 && pos.y<(int) m_level_height-1)
			if (m_level[pos.x][pos.y+1]<current_value) // S
			{
				new_pos.y++;     
				continue;
			}

		if (pos.x>0 && pos.y>0)
			if (m_level[pos.x-1][pos.y]<current_value) // W
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