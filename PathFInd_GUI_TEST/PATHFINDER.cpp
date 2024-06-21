#include <Windows.h>
#include <map>
#include <cassert>
#include <time.h>
#include "VERTEX.h"
#include "Coordinate.h"
#include "PATHFINDER.h"
#include "Tile.h"

extern Tile g_Tile;
extern MSG msg;
Coordinate PATHFINDER::_destination_Coordinate = Coordinate{ 0,0 };
Coordinate PATHFINDER::_start_Coordinate = Coordinate{ 0,0 };
std::map<Coordinate,VERTEX*, pointCompare> PATHFINDER::_open_list_backup;
std::map<Coordinate, VERTEX*, pointCompare> PATHFINDER::_close_list;
bool PATHFINDER::_isSet_destination = false;
bool PATHFINDER::_isSet_start_point = false;	
VERTEX* PATHFINDER::_pCurrent_vertex = nullptr;

void ConvertMillisecondsToTime(DWORD* milliseconds, DWORD* days, DWORD* hours, DWORD* minutes, DWORD* seconds) {
    // �и��ʸ� ��(day), ��(hour), ��(minute), ��(second)�� ��ȯ
	*seconds = *milliseconds / 1000;
	*minutes = *seconds / 60;
	*hours = *minutes / 60;
	*days = *hours / 24;

	*seconds %= 60;
	*minutes %= 60;
	*hours %= 24;
}

void PATHFINDER::clearList_by_selection(bool clear_open_list, bool clear_close_list)
{

	if (clear_open_list)
	{
		for (auto iter = _open_list_backup.begin(); iter != _open_list_backup.end(); )
		{
			if (iter != _open_list_backup.end())
			{
				delete iter->second;
				iter = _open_list_backup.erase(iter);
			}
		}
	}

	if (clear_close_list)
	{
		for (auto iter = _close_list.begin(); iter != _close_list.end(); )
		{
			if (iter != _close_list.end())
			{
				delete 	iter->second;
				iter = _close_list.erase(iter);
			}
		}
		_close_list.clear();
	}
	
}

PATHFINDER::PATHFINDER(bool is_find_path_success)
	: _is_find_path_success(is_find_path_success) {};


void PATHFINDER::test(HWND hWnd)
{
	unsigned long long success_num = 0;
	unsigned long long fail_num = 0;
	timeBeginPeriod(1);

	struct tm start_time_yyyymmdd;
	__time64_t start_time_64;
	_time64(&start_time_64);
	localtime_s(&start_time_yyyymmdd, &start_time_64);

	DWORD start_time = timeGetTime();
	while (1)
	{
		g_Tile.make_map();
		for (int i = 0; i < 100; ++i)
		{

			// start,destination,CANDIDATE, VISITED,SEARCHED ���¸� NOSTATE�� �ʱ�ȭ
			g_Tile.clear_tile(true, true, true, false,true);
			reset();

			_start_Coordinate = Coordinate{ rand() % Tile::GRID_WIDTH,rand() % Tile::GRID_HEIGHT };
			while (g_Tile[_start_Coordinate] != NOSTATE)
				_start_Coordinate = Coordinate{ rand() % Tile::GRID_WIDTH,rand() % Tile::GRID_HEIGHT };

			_destination_Coordinate = Coordinate{ rand() % Tile::GRID_WIDTH,rand() % Tile::GRID_HEIGHT };
			while (g_Tile[_destination_Coordinate] != NOSTATE || _destination_Coordinate == _start_Coordinate)
				_destination_Coordinate = Coordinate{ rand() % Tile::GRID_WIDTH,rand() % Tile::GRID_HEIGHT };

			g_Tile[_start_Coordinate] = START;
			g_Tile[_destination_Coordinate] = DESTINATION;

			_isSet_start_point = true;
			_isSet_destination = true;

			if (pathFind(hWnd))
				success_num++;
			else
				fail_num++;

			if (GetAsyncKeyState(VK_ESCAPE) & 0x8001)
			{
				FILE* fp;
				WCHAR buffer[1024];
				swprintf_s(buffer, _countof(buffer), L"ASTAR_result_%d_%d_%d_%d_%d_%d.txt",
					start_time_yyyymmdd.tm_year + 1900, start_time_yyyymmdd.tm_mon + 1, start_time_yyyymmdd.tm_mday, start_time_yyyymmdd.tm_hour, start_time_yyyymmdd.tm_min, start_time_yyyymmdd.tm_sec);
				_wfopen_s(&fp, buffer, L"w, ccs=UTF-16LE");
				DWORD milliseconds = timeGetTime() - start_time;
				DWORD days; DWORD hours; DWORD minutes; DWORD seconds;
				ConvertMillisecondsToTime(&milliseconds, &days, &hours, &minutes, &seconds);

				struct tm end_time_yyyymmdd;
				__time64_t end_time_64;
				_time64(&end_time_64);
				localtime_s(&end_time_yyyymmdd, &start_time_64);
				memset(buffer, 0, sizeof(buffer));
				swprintf_s(buffer, L"Termination Time : %d - %d - %d : %d : %d : %d\nsuccess_num : %llu\nfail_num : %llu\nElapsed time - %u days : %u hours : %u minutes : %u seconds\n",
					end_time_yyyymmdd.tm_year + 1900, end_time_yyyymmdd.tm_mon + 1, end_time_yyyymmdd.tm_mday, end_time_yyyymmdd.tm_hour, end_time_yyyymmdd.tm_min, end_time_yyyymmdd.tm_sec
					, success_num, fail_num, days, hours, minutes, seconds);
				fwrite(buffer, sizeof(buffer), 1, fp);
				fclose(fp);
				timeEndPeriod(1);
				exit(1);
			}
			InvalidateRect(hWnd, NULL, false);
			GetMessage(&msg, nullptr, 0, 0);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

bool PATHFINDER::_isStart(void)
{
	return (_isSet_start_point && _isSet_destination);
}

void PATHFINDER::reset()
{
    _start_Coordinate = Coordinate{ 0,0 };
    _destination_Coordinate = Coordinate{ 0,0 };

	clearList_by_selection(true, true);
    _isSet_start_point = false;
    _isSet_destination = false;

    _pCurrent_vertex = nullptr;
}

// ���� ��η� ������ vertex���� �̿��Ͽ� ��θ� �׸���.
void PATHFINDER::drawPolyLine(HDC hdc)
{
	// ���� ���� ���Ž���� ��������ʾ� current->vertex�� �������� �ʾѴٸ�
	if (_pCurrent_vertex == nullptr)
		return;

	//������̸� return
	if (_pCurrent_vertex->_parent_vertex == nullptr)
		return;

	HPEN hPen = CreatePen(PS_SOLID, 5, RGB(0, 0, 0)); // ������ �� ����
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	const VERTEX* current_vertex_temp = _pCurrent_vertex;

	do
	{
		Coordinate current_midPoint = make_midPoint(current_vertex_temp);
		Coordinate parent_midPoint = make_midPoint(current_vertex_temp->_parent_vertex);
		MoveToEx(hdc, current_midPoint._x, current_midPoint._y, NULL);
		LineTo(hdc, parent_midPoint._x, parent_midPoint._y);
		current_vertex_temp = current_vertex_temp->_parent_vertex;
	} while (current_vertex_temp->_parent_vertex != nullptr);

	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);
}


bool pointCompare::operator()(const Coordinate& lhs, const Coordinate& rhs) const
{
    if (lhs._x != rhs._x)
        return lhs._x < rhs._x;
    else
        return lhs._y < rhs._y;
}
