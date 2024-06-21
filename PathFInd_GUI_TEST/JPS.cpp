#include <windows.h> 
#include <map>
#include <cassert>
#include <cmath>
#include <utility>
#include "VERTEX.h"
#include "Coordinate.h"
#include "PATHFINDER.h"
#include "Tile.h"
#include "JPS.h"
//#define SHOW
constexpr BYTE is_not_dir = 0b11111111;
constexpr BYTE dest = 0b11111110;
extern Tile g_Tile;
using pair = std::pair<Coordinate, VERTEX*>;
extern MSG msg;
Tile temp_tile;

JPS::JPS()
	: PATHFINDER() {}

bool JPS::pathFind(HWND hWnd)
{
	int i = 0;
	temp_tile = g_Tile;
	bool _is_first_search = true;
	_open_list_backup.insert(pair{ _start_Coordinate, new VERTEX{ _start_Coordinate,_destination_Coordinate,nullptr,is_not_dir,is_not_dir } });
	while (!_open_list_backup.empty())
	{
		//_F���� ���� ���� ��带 ���¸���Ʈ���� �̱�
		VERTEX* pCurrent = std::min_element(_open_list_backup.begin(), _open_list_backup.end(), [](const auto& pair1, const auto& pair2) {
			return pair1.second->_F < pair2.second->_F;
			})->second;

		Coordinate& temp = pCurrent->_pos;
		_pCurrent_vertex = pCurrent;
		_close_list.insert(pair{ pCurrent->_pos, pCurrent });
		_open_list_backup.erase(pCurrent->_pos);
		if (_is_first_search || pCurrent->_pos == _destination_Coordinate)
		{
			if (!_is_first_search)
			{
				_pCurrent_vertex = pCurrent;//��θ� ���� ���� ���� ����ǵ���
				g_Tile[pCurrent->_pos] = DESTINATION;
				{
					try
					{
						//int obs_after = g_Tile.count_obs();
						//if (obs_prev != obs_after)
							//throw 1;
						for (auto& pair : _open_list_backup)
						{
							for (auto& pair2 : _close_list)
							{
								if (pair.second->_pos == pair2.second->_pos)
									//++i;
									throw 2;
								if (g_Tile[pair.second->_pos] != CANDIDATE || g_Tile[pair2.second->_pos] != VISITED)
								{
									if (pair.second->_pos == _start_Coordinate || pair2.second->_pos == _start_Coordinate) continue;
									if (pair.second->_pos == _destination_Coordinate || pair2.second->_pos == _destination_Coordinate) continue;
									throw 3;
								}
							}
						}
					}
					catch (int exptNum)
					{
						FILE* fp;
						fopen_s(&fp, "error_map.txt", "wb");
						if (!fwrite(fp, sizeof(temp_tile._tile), 1, fp))
							throw;
						fclose(fp);
						fopen_s(&fp, "error_map_setting.txt", "wb");
						char temp[256];
						sprintf_s(temp, _countof(temp), "%d,%d,%d,%d", PATHFINDER::_start_Coordinate._x, PATHFINDER::_start_Coordinate._y, PATHFINDER::_destination_Coordinate._x, PATHFINDER::_destination_Coordinate._y);
						fwrite(temp, sizeof(temp), 1, fp);
						fclose(fp);
						WCHAR TEMP[100];
						if(exptNum == 1)
							swprintf_s(TEMP, L"��ֹ� �ջ�");
						else if(exptNum == 2)
							swprintf_s(TEMP, L"���� ����Ʈ Ŭ���� ����Ʈ ��ġ�� �׸� ����");
						else if(exptNum == 3)
							swprintf_s(TEMP, L"����, Ŭ���� ����� ȭ�鿡 ������ ���� �ʴ� �� ����");
						MessageBox(hWnd, L"error", L"error_code", MB_OK);
					}
					return true;
				}
			}
		}
		else
			g_Tile[pCurrent->_pos] = VISITED;

		BYTE forced_dir;
		if (_is_first_search)
		{
			_is_first_search = false;
			Coordinate next_vertex;
			for (BYTE i = Direction::NORTH; i < Direction::TOTAL; ++i)
			{
				Coordinate&& temp = pCurrent->_pos + Coordinate::_direction[i];
				if(is_dir_diagonal(i))
					search_diagonal(pCurrent, temp, (Direction)i, hWnd);
				else
				{
					if (search_linear(pCurrent,temp, &next_vertex, (Direction)i, &forced_dir, hWnd))
					{
						if (g_Tile[next_vertex] != VISITED && g_Tile[next_vertex] != CANDIDATE)
						{
							g_Tile[next_vertex] = (char)CANDIDATE;
							_open_list_backup.insert(pair{ next_vertex, new VERTEX{ next_vertex, _destination_Coordinate, pCurrent,i,forced_dir} });
						}
					}
				}
			}
			continue;
		}
		// ���� ���� ��尡 �ڳʶ� openList���� ���°�� �ڳ� �������� Ž�� �߰�
		// �ڳʷ� �����Ǵ� ������ �ִ� 2���̱� ������ BYTE�� 2�� �����Ѱ�
		if (pCurrent->_forced_neighbour_info != is_not_dir)
		{
			BYTE dir_first = (Direction)get_lower_4bit(pCurrent->_forced_neighbour_info);
			BYTE dir_second = (Direction)get_upper_4bit(pCurrent->_forced_neighbour_info);
			if (is_dir_diagonal(dir_first))
				search_diagonal(pCurrent, pCurrent->_pos + Coordinate::_direction[dir_first], (Direction)dir_first, hWnd);
			if (is_dir_diagonal(dir_second))
				search_diagonal(pCurrent, pCurrent->_pos + Coordinate::_direction[dir_second], (Direction)dir_second, hWnd);
		}

		// ���� ���������� Ž��
		Coordinate next_vertex;
		if (is_linear((Direction)pCurrent->_direction) && search_linear(pCurrent,pCurrent->_pos + Coordinate::_direction[pCurrent->_direction], &next_vertex, (Direction)pCurrent->_direction, &forced_dir, hWnd))
		{
			g_Tile[next_vertex] = (char)CANDIDATE;
			_open_list_backup.insert(pair{ next_vertex, new VERTEX{ next_vertex, _destination_Coordinate, pCurrent,pCurrent->_direction,forced_dir} });
		}
		if (is_diagonal((Direction)pCurrent->_direction))
			search_diagonal(pCurrent, pCurrent->_pos, (Direction)pCurrent->_direction, hWnd);

#ifdef SHOW
		InvalidateRect(hWnd, NULL, FALSE);
		if (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif 
	}
	return false;
} 

// ������ *pForced_dir �� �������, ���н� is_not_dir ����
bool JPS::isJumpPoint(Coordinate current ,const Direction search_direction,BYTE* pForced_dir)
{
	if (current == _destination_Coordinate)
	{
		*pForced_dir = is_not_dir;
		return true;
	}
	*pForced_dir = 0;
	bool is_success = false;
	std::pair<Direction, Direction> blocked_dir;
	std::pair<Direction, Direction> open_dir;
	determine_blocked_direction(search_direction, &blocked_dir);
	determine_open_direction(search_direction, &open_dir);

	Coordinate blocked = current + Coordinate::_direction[blocked_dir.first];
	Coordinate open = current + Coordinate::_direction[open_dir.first];
	bool is_blocked = !g_Tile.index_outOf_range(blocked) && g_Tile[blocked] == OBSTACLE;
	bool is_open = !g_Tile.index_outOf_range(open) && g_Tile[open] != OBSTACLE;
	if (is_blocked && is_open)
	{
		*pForced_dir |= open_dir.first;
		is_success = true;
	}
	blocked = current + Coordinate::_direction[blocked_dir.second];
	open = current + Coordinate::_direction[open_dir.second];
	is_blocked = !g_Tile.index_outOf_range(blocked) && g_Tile[blocked] == OBSTACLE;
	is_open =  !g_Tile.index_outOf_range(open) && g_Tile[open] != OBSTACLE;
	if(is_blocked && is_open)
	{
		*pForced_dir |= open_dir.second << 4;
		is_success = true;
	}
	if (!is_success)
		*pForced_dir = is_not_dir;
	return is_success;
}
// Ž�� ����� searched �Ȱ��� ���� ��ĥ��, ���� ��带 ������ ��ġ�� ã�Ѵٸ� �׺κ��� ȣ���ڰ� ���� ��ĥ�ؾ���
// current_pos�� ���� ������� 
bool JPS::search_linear(VERTEX* parent,Coordinate current, Coordinate* pNext_vertex_coordinate, Direction direction, BYTE* pForced_dir, HWND hWnd)
{
	while (g_Tile.valid_pos(current))
	{
		// ���� ��ġ�� �ϳ� �̻��� forced_neighbor�� ������ jump point
		bool is_start = g_Tile[current] == START;
		bool is_search_start_point_visited = g_Tile[parent->_pos] == VISITED;
		bool is_visited = g_Tile[current] == VISITED;
		bool is_candidate = g_Tile[current] == CANDIDATE;
		// �밢���� ��� Ž���� �� ó�� ������ �θ���� ����Ž���� ������ ��ġ�� �ٸ��� ���ϼ��� �����ϴ� ��峢���� �θ� �����ϱ� ���ؼ� ���� ����
		bool is_horizontal_or_vertical = current._x == parent->_pos._x || current._y == parent->_pos._y;

		// �̹� closelist�� �����ϴ� ���� ������ �������� �ʴ´� (���� �ϸ� �� �ִܰ�ο� ������� ���ɼ��� ������ ���ɻ� JPS�� �ǹ̰� ����ɰ��̴�)
		if (!is_start && !is_visited && isJumpPoint(current, direction, pForced_dir))
		{
			// ���� �̹� �ڳʷμ� ���¸���Ʈ�� �����ϴ� ������� ���� ���ؿ����� �ڳ��ΰ��
			// ���� �θ��� ����� ���� ���� Ȥ�� ���򼱻� �����ϴµ� ��尡 �̹� �ٸ������ �ڽ��ΰ���� 
			if ((is_candidate) && is_horizontal_or_vertical && is_search_start_point_visited)
			{
				// G���� ���ؼ� �� �۴ٸ�
				VERTEX* pExist = _open_list_backup.find(current)->second;
				double compare_G = getGHF::get_G(current, parent);
				if (compare_G < pExist->_G)
				{
					pExist->_parent_vertex = parent;
					pExist->_direction = (BYTE)direction;
					pExist->_forced_neighbour_info = *pForced_dir;
					pExist->_G = compare_G;
					pExist->_F = compare_G + pExist->_H;
					return false; // ���� ���� �����ϸ� �ȵ�
				}
			}
			//�ڳ����� ���¸���Ʈ���� ���� ���
			else if(!is_visited)
			{
				*pNext_vertex_coordinate = current;
				return true;
			}
		}
		// ���򼱻� �����ϴ� closelist�� �����ϴ� ��� ���ؿ����� �ڳʰ� �ƴ����� �θ� �ٲ�� G���� �������
		else if (is_candidate && is_horizontal_or_vertical)
		{
			VERTEX* pExist = _open_list_backup.find(current)->second;
			double compare_G = getGHF::get_G(current, parent);
			if (compare_G < pExist->_G)
			{
				pExist->_parent_vertex = parent;
				//pExist->_direction = (BYTE)direction;
				//pExist->_forced_neighbour_info = *pForced_dir;
				pExist->_G = compare_G;
				pExist->_F = compare_G + pExist->_H;
			}
		}
		if (g_Tile[current] != START && g_Tile[current] != CANDIDATE && g_Tile[current] != VISITED) g_Tile[current] = SEARCHED;
		current+= Coordinate::_direction[direction];
	}
#ifdef PRINT
	InvalidateRect(hWnd, NULL, FALSE);
	if (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
	// Ž���� ��� jumppoint�� Ž������ �ʰ� ��ֹ� Ȥ�� ���� ���� �������
	pNext_vertex_coordinate = nullptr; 
	return false;
}

void JPS::search_diagonal(VERTEX* parent, Coordinate current, const Direction direction, HWND hWnd)
{
	// search_linear�ʹ� �ٸ��� while������ �Ұ��ձ⋚���� �̸� üũ
	if(!g_Tile.valid_pos(current))
		return; 
	Coordinate next_horizontal;
	Coordinate next_vertical;
	BYTE corner_dir;
	BYTE corner_dir_horizontal;
	BYTE corner_dir_vertical;
	std::pair<Direction, Direction> auxiliary_direction;
	dertermine_auxiliary_direction(direction, &auxiliary_direction);
	bool search_horizontal;
	bool search_vertical;
	bool is_candidate = g_Tile[current] == CANDIDATE;
	bool is_visited = g_Tile[current] == VISITED;
	//����Ž������ ���� ����� ���ڸ��� ��尡 �����ȵ� �밢��Ž���Լ��� �ش���ġ���� �ٽ� ȣ�������� ����Ž������ �߰ߵ� �ڳ� ��ġ�� vertex�� �߰��� �̵� 
	if (parent->_pos == current)
	{
			search_vertical = search_linear(parent, current + Coordinate::_direction[auxiliary_direction.second], &next_vertical, auxiliary_direction.second, &corner_dir_vertical, hWnd);
			search_horizontal = search_linear(parent, current + Coordinate::_direction[auxiliary_direction.first], &next_horizontal, auxiliary_direction.first, &corner_dir_horizontal, hWnd);
			if (search_horizontal && g_Tile[next_horizontal] != VISITED && g_Tile[next_horizontal] != CANDIDATE)
			{
				_open_list_backup.insert(pair{ next_horizontal, new VERTEX{ next_horizontal, _destination_Coordinate, parent, (BYTE)auxiliary_direction.first, corner_dir_horizontal} });
				g_Tile[next_horizontal] = CANDIDATE;
			}
			if (search_vertical && g_Tile[next_vertical] != VISITED && g_Tile[next_vertical] != CANDIDATE)
			{
				_open_list_backup.insert(pair{ next_vertical, new VERTEX{ next_vertical, _destination_Coordinate, parent, (BYTE)auxiliary_direction.second, corner_dir_vertical} });
				g_Tile[next_vertical] = CANDIDATE;
			}
		current += Coordinate::_direction[direction];
	}

	while (g_Tile.valid_pos(current))
	{
		is_candidate = g_Tile[current] == CANDIDATE;
		is_visited = g_Tile[current] == VISITED;
		// �켱 ���ڸ��� �ڳ����� üũ�ϰ� �ڳʶ�� vertex�� �߰��ϰ� ����
		if (!is_visited && isJumpPoint(current, direction, &corner_dir))
		{
			// ���� �������ε� �ڳ��ε� �̹� ���� vertex�� ����������� ���� G���� �� ������ vertex��ü�� ��ü��
			if (is_candidate)
				//if (is_candidate || is_visited)
			{
				VERTEX* pExist = _open_list_backup.find(current)->second;
				double compare_G = getGHF::get_G(current, parent);
				if (compare_G < pExist->_G)
				{
					pExist->_G = compare_G;
					pExist->_parent_vertex = parent;
					pExist->_direction = (BYTE)direction;
					pExist->_forced_neighbour_info = corner_dir;
					pExist->_F = compare_G + pExist->_H;
				}
			}
			else if (!is_visited)
			{
				_open_list_backup.insert(pair{ current, new VERTEX{ current, _destination_Coordinate, parent, (BYTE)direction,corner_dir} });
				g_Tile[current] = CANDIDATE;
			}
			return;
		}
		// �밢�� Ž���̸鼭 �� ��ġ�� �ڳʰ� �ƴ϶�� ����Ž�� ����� ���ؼ� �ڳʰ� ������̴� 
		// is_candidate�� is_visited�� ������ ���� �ϳ��� �����̴� �Ѵ� ���̵Ǵ°� �Ұ���
		// ���� ��ġ�� ���¸���Ʈ�� ���µ� ����Ž���� ���� �ڳʸ� ã���� ���� ��ġ�� ��带 �߰��ϰ� ����

		if (!is_visited)
		{
			search_horizontal = search_linear(parent, current + Coordinate::_direction[auxiliary_direction.first], &next_horizontal, auxiliary_direction.first, &corner_dir_horizontal, hWnd);
			search_vertical = search_linear(parent, current + Coordinate::_direction[auxiliary_direction.second], &next_vertical, auxiliary_direction.second, &corner_dir_vertical, hWnd);
			if ((search_horizontal || search_vertical))
			{
				_open_list_backup.insert(pair{ current, new VERTEX{ current, _destination_Coordinate, parent, (BYTE)direction,is_not_dir} });
				g_Tile[current] = CANDIDATE;
				return;
			}
		}
			// ��ĥ
		if (g_Tile[current] != START && g_Tile[current] != CANDIDATE && g_Tile[current] != VISITED) g_Tile[current] = SEARCHED;

		current += Coordinate::_direction[direction];
	}


}
