#include <windows.h>
#include <map>
#include <cassert>
#include <time.h>
#include "VERTEX.h"
#include "Coordinate.h"
#include "ASTAR.h"
#include "Tile.h"
#pragma comment(lib,"winmm.lib")

extern Tile g_Tile;
extern MSG msg;
void ConvertMillisecondsToTime(DWORD* milliseconds, DWORD* days, DWORD* hours, DWORD* minutes, DWORD* seconds);

bool ASTAR::pathFind(HWND hWnd)
{
    _open_list_backup.insert(std::pair<Coordinate, VERTEX*>{_start_Coordinate, new VERTEX{ _start_Coordinate,_destination_Coordinate }});
    while (!_open_list_backup.empty())
    {
       std::map<Coordinate,VERTEX*>::iterator current_vertex = std::min_element(_open_list_backup.begin(), _open_list_backup.end(), [](const auto& pair1, const auto& pair2) {
            return pair1.second->_F < pair2.second->_F;
            });

        VERTEX* current = current_vertex->second;
        if(current->_pos == _destination_Coordinate)
        {
            _open_list_backup.erase(current->_pos);
            _close_list.insert(std::pair<Coordinate, VERTEX*>{current->_pos, current});
            _pCurrent_vertex = current;//��θ� ���� ���� ���� ����ǵ���
            g_Tile[current->_pos] = DESTINATION;
            return true;//��ã�� ����
        }
        //�湮ó��(������� �ߺ���ĥ�� �������� ǥ�þ���)
        do
        {
            if (current->_pos == _start_Coordinate)
                break;
            if(current->_pos == _destination_Coordinate)
                break;

            g_Tile[current->_pos] = VISITED;
        } while (false);

		//���� ��ġ�� _close_list�� �ְ� �̵�
        _close_list.insert(std::pair<Coordinate, VERTEX*>{current->_pos, current});
        _open_list_backup.erase(current->_pos);

        // �̵���θ� ����ϱ����� ���� vertex�� ������ ��������� ����
        _pCurrent_vertex = current;

        // openList�� ���� 8���� ��� ����
        for (int i = Coordinate::NORTH; i < Coordinate::TOTAL; ++i)
        {
            // �̹� openList�� closeList�� �����ϴ� ���
            do
            {
                Coordinate next_pos = current->_pos + Coordinate::_direction[i];
                if (g_Tile.index_outOf_range(next_pos))   break;

                STATE next_state = (STATE)g_Tile[next_pos];
                if (next_state == CANDIDATE && next_state != START) // openList�� �����ϸ�
                {
					VERTEX* existing_vertex = _open_list_backup.find(next_pos)->second; // ���ϱ� ���ؼ� ����
					double compare_G = getGHF::get_G(Coordinate{ next_pos }, current);
					if (compare_G < existing_vertex->_G)
					{
						existing_vertex->_G = compare_G;
						existing_vertex->_F = existing_vertex->_H + compare_G;
						existing_vertex->_parent_vertex = current;
					}
					break;
				}
                else if (next_state == VISITED && next_state != START)
                    break;
                else if (next_state == OBSTACLE)
                    break;
                // �ߺ��� ���� �ʾѰ� �̹� �湮�� ������ �ƴϱ� ������ ����
                if (next_state == START)  
                    continue;
                VERTEX* temp = new VERTEX{next_pos,_destination_Coordinate,current };
                g_Tile[temp->_pos] = CANDIDATE;
                _open_list_backup.insert(std::pair<Coordinate, VERTEX*>(temp->_pos, temp));
            } while (false);
       }
    }
    return false;
}
void ASTAR::drawParentLine(HDC hdc)
{
    // ������̸� return
    // openList ��ȸ
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(139, 69, 19)); // ���� �� ����
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    for (auto iter = _open_list_backup.begin(); iter != _open_list_backup.end(); ++iter)
    {
        // ��ó���� ������� openList�� ������
        if (iter->second->_parent_vertex == nullptr)
            continue;

        Coordinate&& parent_midPoint = make_midPoint(iter->second->_parent_vertex);
        Coordinate&& current_midPoint = make_midPoint(iter->second);
        Coordinate&& vector_to_parent = (parent_midPoint - current_midPoint);
        MoveToEx(hdc, current_midPoint._x, current_midPoint._y, NULL);
        LineTo(hdc, current_midPoint._x + vector_to_parent._x, current_midPoint._y + vector_to_parent._y);
        //LineTo(hdc, current_midPoint._x + vector_to_parent._x, current_midPoint._y + vector_to_parent._y);
    }
    // close_list ��ȸ
    for (auto&& iter = _close_list.begin(); iter != _close_list.end(); ++iter)
    {
        // closeList�� �����ϴ� ������� �׸�������. (��ó���� �����ϸ� �׻� closeList�� ������� �����Ұ���)
        if (iter->second->_parent_vertex == nullptr)
            continue;

        Coordinate&& current_midPoint = make_midPoint(iter->second);
        Coordinate&& parent_midPoint = make_midPoint(iter->second->_parent_vertex);
        Coordinate&& vector_to_parent = (parent_midPoint - current_midPoint) / 2;
        MoveToEx(hdc, current_midPoint._x, current_midPoint._y, NULL);
        LineTo(hdc, current_midPoint._x + vector_to_parent._x, current_midPoint._y + vector_to_parent._y);
    }
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

ASTAR::ASTAR()
    :PATHFINDER()// �⺻���ڵ� ���� ������
{
}

