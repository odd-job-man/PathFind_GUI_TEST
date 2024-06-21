#include <cassert>
#include "Coordinate.h"
#include "Tile.h"
#include <vcruntime_string.h>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <Windows.h>
#include <map>
#include "PATHFINDER.h"
#define PRINT
#pragma warning (disable :  4244)

constexpr int OBSTACLE_PROBABILITY = 45;
extern Tile g_Tile;
extern PATHFINDER* pathfinder;

Tile::Tile()
{
	initialize_tile();
}
char& Tile::operator[](const Coordinate& index)
{
	return _tile[index._y][index._x];
}

const char Tile::operator[](const Coordinate& index) const
{
	return _tile[index._y][index._x];
}

bool Tile::index_outOf_range(const Coordinate& index)
{
	return index._x < 0 || index._x >= GRID_WIDTH || index._y < 0 || index._y >= GRID_HEIGHT;
}

bool Tile::valid_pos(const Coordinate index)
{
	return _tile[index._y][index._x] != OBSTACLE && !index_outOf_range(index);
}

void Tile::initialize_tile()
{
	memset(_tile, (int)NOSTATE, sizeof(_tile));
}
void Tile::make_map()
{
	bool is_success = false;
	do
	{
		for (int y = 0; y < GRID_HEIGHT; y++)
		{
			for (int x = 0; x < GRID_WIDTH; x++)
			{
				int pos = rand() % 100 + 1;
				if (pos <= OBSTACLE_PROBABILITY)
					_tile[y][x] = OBSTACLE;
				else
					_tile[y][x] = NOSTATE;
			}
		}
		// ���귯 ���丶Ÿ�� �� ����
		for (int k = 0; k < 5; ++k)
		{
			for (int y = 0; y < GRID_HEIGHT; ++y)
			{
				for (int x = 0; x < GRID_WIDTH; ++x)
				{
					Coordinate current{ x,y };
					int count = 0;
					if (operator[](current) == NOSTATE)
					{
						for (int i = 0; i < Coordinate::TOTAL; ++i)
						{
							Coordinate next = current + Coordinate::_direction[i];
							if (!index_outOf_range(next) && operator[](next) == NOSTATE)
								++count;
						}
						if (!(count >= 4))
							operator[](current) = OBSTACLE;
					}
					else if (operator[](current) == OBSTACLE)
					{
						for (int i = 0; i < Coordinate::TOTAL; ++i)
						{
							Coordinate next = current + Coordinate::_direction[i];
							if (!index_outOf_range(next) && operator[](next) == NOSTATE)
								++count;
						}
						if (count >= 5)
							operator[](current) = NOSTATE;
					}
				}
			}		
		}
		is_success = remove_Dead_end(*this);
	} while (!is_success);//���������� ����
}
void Tile::make_map_render(HWND hWnd)
{
	bool is_success = false;
	do
	{
		for (int y = 0; y < GRID_HEIGHT; y++)
		{
			for (int x = 0; x < GRID_WIDTH; x++)
			{
				int pos = rand() % 100 + 1;
				if (pos <= OBSTACLE_PROBABILITY)
					_tile[y][x] = OBSTACLE;
				else
					_tile[y][x] = NOSTATE;
			}
		}
		InvalidateRect(hWnd, NULL, false);
		UpdateWindow(hWnd);
		Sleep(100);
		// ���귯 ���丶Ÿ�� �� ����
		for (int k = 0; k < 5; ++k)
		{
			for (int y = 0; y < GRID_HEIGHT; ++y)
			{
				for (int x = 0; x < GRID_WIDTH; ++x)
				{
					Coordinate current{ x,y };
					int count = 0;
					if (operator[](current) == NOSTATE)
					{
						for (int i = 0; i < Coordinate::TOTAL; ++i)
						{
							Coordinate next = current + Coordinate::_direction[i];
							if (!index_outOf_range(next) && operator[](next) == NOSTATE)
								++count;
						}
						if (!(count >= 4))
							operator[](current) = OBSTACLE;
					}
					else if (operator[](current) == OBSTACLE)
					{
						for (int i = 0; i < Coordinate::TOTAL; ++i)
						{
							Coordinate next = current + Coordinate::_direction[i];
							if (!index_outOf_range(next) && operator[](next) == NOSTATE)
								++count;
						}
						if (count >= 5)
							operator[](current) = NOSTATE;
					}
				}
			}
			InvalidateRect(hWnd, NULL, false);
			UpdateWindow(hWnd);
			Sleep(250);
		}
		is_success = remove_Dead_end_render(*this, hWnd);
	} while (!is_success);//���������� ����
}
void Tile::polling(HWND hWnd)
{
	for (int i = 0; i < 100000000; ++i)
	{
		make_map();
		InvalidateRect(hWnd, NULL, false);
		UpdateWindow(hWnd);
	}
}
void Tile::reappearnce_all_1s_tile(HWND hWnd)
{
	FILE* fp;
	fopen_s(&fp, "error_map.txt", "rb");
	if (!fread(_tile, sizeof(_tile), 1, fp))
		throw;


	InvalidateRect(hWnd, NULL, false);
	UpdateWindow(hWnd);
	fclose(fp);
	Tile* pTemp_tile = new Tile;
	Tile& temp_tile = *pTemp_tile;
	memcpy_s(temp_tile._tile, sizeof(temp_tile._tile), _tile, sizeof(_tile));

	remove_Dead_end(temp_tile);
	memcpy_s(_tile, sizeof(_tile), temp_tile._tile, sizeof(temp_tile._tile));
	delete pTemp_tile;

}
void Tile::clear_tile(bool isClear_start_dest, bool isClear_open_list, bool isClear_close_list, bool isClear_obstacle,bool isClear_search)
{
	for (int y = 0; y < GRID_HEIGHT; ++y)
	{
		for (int x = 0; x < GRID_WIDTH; ++x)
		{
			switch ((STATE)_tile[y][x])
			{
			case OBSTACLE:
				if(isClear_obstacle)
					_tile[y][x] = NOSTATE;
				break;
			case START:
			case DESTINATION:
				if(isClear_start_dest)
					_tile[y][x] = NOSTATE;
				break;
			case CANDIDATE:
				if (isClear_open_list)
					_tile[y][x] = NOSTATE;
				break;
			case VISITED:
				if (isClear_close_list)
					_tile[y][x] = NOSTATE;
				break;
			case SEARCHED:
				if (isClear_search)
					_tile[y][x] = NOSTATE;
			default:
				break;
			}	
		}
	}
}
void Tile::backup_map_and_setting()
{
	FILE* fp;
	fopen_s(&fp, "error_map.txt", "wb");
	if (!fwrite(_tile, sizeof(_tile), 1, fp))
		throw;
	fclose(fp);

	fopen_s(&fp,"error_map_setting.txt", "wb");
	char temp[256];
	sprintf_s(temp, _countof(temp), "%d,%d,%d,%d", PATHFINDER::_start_Coordinate._x, PATHFINDER::_start_Coordinate._y, PATHFINDER::_destination_Coordinate._x, PATHFINDER::_destination_Coordinate._y);
	fwrite(temp, sizeof(temp), 1, fp);
	fclose(fp);

}
void Tile::set_map_and_setting(HWND hWnd)
{
	pathfinder->reset();
	FILE* fp;
	fopen_s(&fp, "error_map.txt", "rb");
	if (!fread(_tile, sizeof(_tile), 1, fp))
		throw;
	fclose(fp);


	fopen_s(&fp, "error_map_setting.txt", "rb");
	char temp[256];
	if (fread(temp, sizeof(temp), 1, fp) != 1)
		throw;
	fclose(fp);

	char* pNext_token = nullptr;
	PATHFINDER::_start_Coordinate._x = atoi(strtok_s(temp, ",", &pNext_token));
	PATHFINDER::_start_Coordinate._y = atoi(strtok_s(pNext_token, ",", &pNext_token));
	PATHFINDER::_destination_Coordinate._x = atoi(strtok_s(pNext_token, ",", &pNext_token));
	PATHFINDER::_destination_Coordinate._y = atoi(strtok_s(pNext_token, ",", &pNext_token));

	g_Tile[PATHFINDER::_start_Coordinate] = START;
	g_Tile[PATHFINDER::_destination_Coordinate] = DESTINATION;
	PATHFINDER::_isSet_start_point = true;
	PATHFINDER::_isSet_destination = true;
	InvalidateRect(hWnd, NULL, false);
	UpdateWindow(hWnd);
}
Coordinate* make_BFS_start_pos(const Tile& tile)
{
	int y; int x;
	for (y = 0; y < Tile::GRID_HEIGHT; y++)
	{
		for (x = 0; x < Tile::GRID_WIDTH; x++)
		{
			Coordinate current{ x,y };
			if (tile[current] == NOSTATE)
			{
				return new Coordinate{ x,y };
			}
		}
	}

	// ȭ���� �����ϰ� ������ �� ������ �迭�� ������ �����Ǵ� ���
	return nullptr;
}
bool remove_Dead_end(Tile& tile)
{
	Coordinate* start_pos = make_BFS_start_pos(tile);
	if (!start_pos) return false;

	std::queue<Coordinate*> q;
	q.push(start_pos);
	tile[*start_pos] = VISITED;
	int visited_count = 1;

	while (!q.empty())
	{
		Coordinate* current = q.front();
		q.pop();
		for (int i = Coordinate::NORTH; i < Coordinate::TOTAL; ++i)
		{
			Coordinate next = *current + Coordinate::_direction[i];
			if (!tile.index_outOf_range(next) && tile[next] == NOSTATE)
			{
				tile[next] = VISITED;
				++visited_count;
				q.push(new Coordinate{ next });
			}
		}
		delete current;
	}
	// ���� ������ ������ ���������� �ƴϾ��� ���
	if (visited_count >= Tile::GRID_HEIGHT * Tile::GRID_WIDTH / 2)
	{
		for (int y = 0; y < Tile::GRID_HEIGHT; ++y)
		{
			for (int x = 0; x < Tile::GRID_WIDTH; ++x)
			{
				Coordinate&& current = Coordinate{ x,y };
				switch (tile[current])
				{
				case VISITED:
					tile[current] = NOSTATE;
					break;
				case NOSTATE:
					tile[current] = OBSTACLE;
					break;
				}
			}
		}

	}
	else // ���������� ������ ���� �����̾��� ���
	{
		for (int y = 0; y < Tile::GRID_HEIGHT; ++y)
		{
			for (int x = 0; x < Tile::GRID_WIDTH; ++x)
			{
				Coordinate&& current = Coordinate{ x,y };
				if (tile[current] == VISITED)
					tile[current] = OBSTACLE;
			}
		}
		return remove_Dead_end(tile);
	}
	// ��� ���� �ֻ�ܿ��� ������ true ��ȯ
	return true;
}
bool remove_Dead_end_render(Tile& tile, HWND hWnd)
{
	Coordinate* start_pos = make_BFS_start_pos(tile);
	if (!start_pos) return false;

	std::queue<Coordinate*> q;
	q.push(start_pos);
	tile[*start_pos] = VISITED;
	int visited_count = 1;

	while (!q.empty())
	{
		Coordinate* current = q.front();
		q.pop();
		for (int i = Coordinate::NORTH; i < Coordinate::TOTAL; ++i)
		{
			Coordinate next = *current + Coordinate::_direction[i];
			if (!tile.index_outOf_range(next) && tile[next] == NOSTATE)
			{
				tile[next] = VISITED;
				++visited_count;
				q.push(new Coordinate{ next });
			}
		}
		delete current;
	}
	InvalidateRect(hWnd, NULL, false);
	UpdateWindow(hWnd);
	Sleep(500);
	// ���� ������ ������ ���������� �ƴϾ��� ���
	if (visited_count >= Tile::GRID_HEIGHT * Tile::GRID_WIDTH / 2)
	{
		for (int y = 0; y < Tile::GRID_HEIGHT; ++y)
		{
			for (int x = 0; x < Tile::GRID_WIDTH; ++x)
			{
				Coordinate&& current = Coordinate{ x,y };
				if (tile[current] == NOSTATE)
				{
					tile[current] = OBSTACLE;
					InvalidateRect(hWnd, NULL, false);
					UpdateWindow(hWnd);
				}
			}
		}

		for (int y = 0; y < Tile::GRID_HEIGHT; ++y)
		{
			for (int x = 0; x < Tile::GRID_WIDTH; ++x)
			{
				Coordinate&& current = Coordinate{ x,y };
				if (tile[current] == VISITED)
					tile[current] = NOSTATE;
			}
		}
		InvalidateRect(hWnd, NULL, false);
		UpdateWindow(hWnd);
		Sleep(500);
	}
	else // ���������� ������ ���� �����̾��� ���
	{
		for (int y = 0; y < Tile::GRID_HEIGHT; ++y)
		{
			for (int x = 0; x < Tile::GRID_WIDTH; ++x)
			{
				Coordinate&& current = Coordinate{ x,y };
				if (tile[current] == VISITED)
				{
					tile[current] = OBSTACLE;
					InvalidateRect(hWnd, NULL, false);
					UpdateWindow(hWnd);
				}
			}
		}
		return remove_Dead_end_render(tile, hWnd);
	}
	// ��� ���� �ֻ�ܿ��� ������ true ��ȯ
	return true;
}
