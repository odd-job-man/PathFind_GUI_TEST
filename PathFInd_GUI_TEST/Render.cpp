#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <queue>
#include <algorithm>
#include "Render.h"
#include "Tile.h"
#include "PATHFINDER.h"
#include "ASTAR.h"
#include "JPS.h"
#include "VERTEX.h"

#pragma warning (disable : 4244)
// ���� ����:
HINSTANCE hInst;                                // ���� �ν��Ͻ��Դϴ�.
BOOL g_bClick;
HPEN g_hPen;
HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;
bool g_isPrintPos = false;
Coordinate cursor_tile_index;
HBRUSH g_hTileBrush;
HPEN g_hGridPen;
Tile g_Tile;
bool g_bErase = false;
bool g_bDrag = false;
bool g_drawUI = false;

// �湮�� �ĺ��� ��ĥ ���θ� �����ϴ� ������ 3�� Ű�ٿ ���� ����Ī�Ǹ� render obstacle���� ���ȴ�.
bool g_draw_CANDIDATE_VISITED = true;

bool is_Astar = true;
ASTAR g_ASTAR;
JPS g_JPS;
PATHFINDER* pathfinder = &g_JPS;

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void RenderGrid(HDC hdc);
void RenderUI(HDC hdc);

Coordinate g_render_start_point;
int g_scroll_up = 0;

void RegisterWindow(void)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, NULL);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TEST";
    wcex.hIconSm = NULL;
    RegisterClassExW(&wcex);
}
void makeWindow(HWND hWnd, int nCmdShow)
{
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int a;
    HDC hdc;
    PAINTSTRUCT ps;
    switch (message)
    {
    case WM_LBUTTONDOWN:
        g_bDrag = true;
        {
            Coordinate tile_index= Coordinate{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) } / (Tile::_GRID_SIZE + g_scroll_up) + g_render_start_point;
            if (g_Tile.index_outOf_range(tile_index))
                break;
            if(g_Tile[tile_index] == OBSTACLE)
                g_bErase = true;
            else
                g_bErase = false;
        }
        break;
    case WM_LBUTTONUP:
        g_bDrag = false;
        break;
    case WM_LBUTTONDBLCLK:
    {
        if (!g_ASTAR._isSet_start_point)
        {
            PATHFINDER::_start_Coordinate = Coordinate{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) } / (Tile::_GRID_SIZE + g_scroll_up) + g_render_start_point;
            g_Tile[PATHFINDER::_start_Coordinate] = START;
        }
        else
        {
            for (auto iter = PATHFINDER::_open_list_backup.begin(); iter != g_ASTAR._open_list_backup.end();)
            {
				g_Tile[iter->first] = NOSTATE;
                iter = g_ASTAR._open_list_backup.erase(iter);
            }

            for (auto iter = PATHFINDER::_close_list.begin(); iter != g_ASTAR._close_list.end();)
            {
                PATHFINDER::_pCurrent_vertex = nullptr;
                g_Tile[iter->first] = NOSTATE;
                iter = PATHFINDER::_close_list.erase(iter);
            }
            g_Tile[PATHFINDER::_start_Coordinate] = NOSTATE;
            PATHFINDER::_start_Coordinate = Coordinate{ 0,0 };
        }

        g_ASTAR._isSet_start_point = !g_ASTAR._isSet_start_point;
        InvalidateRect(hWnd, NULL, false);
    }
    break;

    case WM_RBUTTONDBLCLK:
    {
        int a;
        if (!g_ASTAR._isSet_destination)
        {
            g_ASTAR._destination_Coordinate = Coordinate{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) } / (Tile::_GRID_SIZE + g_scroll_up) + g_render_start_point;
            g_Tile[g_ASTAR._destination_Coordinate] = DESTINATION;
        }
        else
        {
            for (auto iter = g_ASTAR._open_list_backup.begin(); iter != g_ASTAR._open_list_backup.end();)
            {
				g_Tile[iter->first] = NOSTATE;
                iter = g_ASTAR._open_list_backup.erase(iter);
            }

            for (auto iter = g_ASTAR._close_list.begin(); iter != g_ASTAR._close_list.end();)
            {
                g_Tile[iter->first] = NOSTATE;
                iter = g_ASTAR._close_list.erase(iter);
            }
            g_Tile[g_ASTAR._destination_Coordinate] = NOSTATE;
            g_ASTAR._pCurrent_vertex = nullptr;
            g_ASTAR._destination_Coordinate = Coordinate{ 0,0 };
        }

        g_ASTAR._isSet_destination = !g_ASTAR._isSet_destination;
        InvalidateRect(hWnd, NULL, false);
    }
    break;

    case WM_MOUSEMOVE:
    {
        if (g_bDrag)
        {
            cursor_tile_index = Coordinate{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) } / (Tile::_GRID_SIZE + g_scroll_up) + g_render_start_point;
            if (g_Tile.index_outOf_range(cursor_tile_index))    break;
            g_Tile[cursor_tile_index] = !g_bErase;
            InvalidateRect(hWnd, NULL, false);
        }
    }
    break;
    case WM_MOUSEWHEEL:
	{
		// ���� ��ġ�� x, y ��ǥ ���ϱ�
        Coordinate original_pos = Coordinate{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
        Coordinate zoomPos = original_pos / (Tile::_GRID_SIZE + g_scroll_up) + g_render_start_point;
        int zoomDelta = GET_WHEEL_DELTA_WPARAM(wParam);

        if (zoomDelta < 0)
        {
			if (g_scroll_up > 0)
				g_scroll_up -= 20;
		}
        if (zoomDelta > 0)
        {
            if(g_scroll_up <= 60)
                g_scroll_up += 20;
        }
        
        g_render_start_point = zoomPos - original_pos / (Tile::_GRID_SIZE + g_scroll_up);
        if(g_render_start_point._x < 0)
			g_render_start_point._x = 0;

        if(g_render_start_point._y < 0)
            g_render_start_point._y = 0;
		InvalidateRect(hWnd, NULL, false);
	}
	break;
    case WM_CREATE:
    {
        HDC hdc = GetDC(hWnd);
        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc); // �޸� ����̽� ���ؽ�Ʈ ����
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap); // ���ʿ� ���� �޸� ��Ʈ�� ���
    }
    break;
    case WM_PAINT:
    {
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        hdc = BeginPaint(hWnd, &ps);
        RenderGrid(g_hMemDC);
        RenderObstacle(g_hMemDC);
        RenderUI(g_hMemDC);
        if (pathfinder->_isStart())
        {
			pathfinder->drawPolyLine(g_hMemDC);
			g_ASTAR.drawParentLine(g_hMemDC);
			if (is_Astar)
			{
                //if (g_draw_CANDIDATE_VISITED);
			}
        }
        BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE:
    {
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
    }
    break;
    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap); // �̰� ��?
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);
        DeleteObject(g_hTileBrush);
        DeleteObject(g_hGridPen);
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
    {
		Coordinate original_pos = Coordinate{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
        Coordinate zoomPos = original_pos / (Tile::_GRID_SIZE + g_scroll_up) + g_render_start_point;
        if(g_Tile.index_outOf_range(zoomPos))    break;
        switch (wParam)
        {
        case VK_SHIFT:
            g_drawUI = !g_drawUI;
			InvalidateRect(hWnd, NULL, false);
            break;
        case VK_CONTROL:
            g_isPrintPos = !g_isPrintPos;
            InvalidateRect(hWnd, NULL, false);
            break;
        case VK_TAB:
            if (g_ASTAR._isStart())
            {
                pathfinder->pathFind(hWnd);
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case VK_UP:
            if (g_render_start_point._y > 0)
                --g_render_start_point._y;
            InvalidateRect(hWnd, NULL, false);
            break;
		case VK_DOWN:
            if(g_render_start_point._y < Tile::GRID_HEIGHT - 1)
				++g_render_start_point._y;
            InvalidateRect(hWnd, NULL, false);
            break;
        case VK_LEFT:
			if (g_render_start_point._x > 0)
				--g_render_start_point._x;
			InvalidateRect(hWnd, NULL, false);
			break;
        case VK_RIGHT:
			if (g_render_start_point._x < Tile::GRID_WIDTH - 1)
				++g_render_start_point._x;
            InvalidateRect(hWnd, NULL, false);
			break;
        case VK_SPACE:
            g_Tile.set_map_and_setting(hWnd);
			InvalidateRect(hWnd, NULL, false);
			UpdateWindow(hWnd);
            break;
        case 0x31://1��Ű
            pathfinder->reset();
            g_Tile.clear_tile(true, true, true, true, true);
            InvalidateRect(hWnd, NULL, false);
            break;
        case 0x32: // ����� ������ ��ֹ� ���ܳ��� ������ �ʱ�ȭ
        {
            g_Tile.clear_tile(false, true, true, false,true);
			PATHFINDER::_pCurrent_vertex = nullptr;
			InvalidateRect(hWnd, NULL, false);

			if (!PATHFINDER::_open_list_backup.empty())
			{
				for (auto iter = PATHFINDER::_open_list_backup.begin(); iter != PATHFINDER::_open_list_backup.end(); )
				{
					delete iter->second;
					iter = PATHFINDER::_open_list_backup.erase(iter);
				}
			}

			if (!PATHFINDER::_close_list.empty())
			{
				for (auto iter = PATHFINDER::_close_list.begin(); iter != PATHFINDER::_close_list.end(); )
				{
					delete iter->second;
					iter = PATHFINDER::_close_list.erase(iter);
				}
			}
			break;
		}
        case 0x33: // �湮��, �ĺ��� ��ĥ �Ѱ� ����
        {	
            g_draw_CANDIDATE_VISITED = !g_draw_CANDIDATE_VISITED;
			InvalidateRect(hWnd, NULL, false);
		}   
        break;
        case 0x34: // Ȯ�� ��ҷ� �ٲ� ������ �ʱ�������� ����
        {
            g_scroll_up = 0;
            g_render_start_point = Coordinate{ 0,0 };
            InvalidateRect(hWnd, NULL, false);  
		}
        break;
        case 0x35:
            //g_Tile.reappearnce_all_1s_tile(hWnd);   
            //g_Tile.polling(hWnd);
            g_Tile.make_map();
            InvalidateRect(hWnd, NULL, false);
            break;
        case 0x38:
            g_Tile.make_map_render(hWnd);
            InvalidateRect(hWnd, NULL, false);
            break;
        case 0x36:
            pathfinder->test(hWnd);
			InvalidateRect(hWnd, NULL, false);
			break;
        case 0x37:
            if(is_Astar)
			{
				pathfinder = &g_JPS;
				is_Astar = false;
			}
			else
			{
				pathfinder = &g_ASTAR;
				is_Astar = true;
			}
			InvalidateRect(hWnd, NULL, false);
            break;
        default:
            break;
        }
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
void RenderGrid(HDC hdc)
{
    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);

    int iX = 0;
    int iY = 0;

    for (int x = 0; x <= Tile::GRID_WIDTH; ++x)
    {
        MoveToEx(hdc, iX, 0, NULL);
        LineTo(hdc, iX, (Tile::GRID_HEIGHT - g_render_start_point._y) * (Tile::_GRID_SIZE + g_scroll_up));
        iX += (Tile::_GRID_SIZE + g_scroll_up);
    }

    for (int y = 0; y <= Tile::GRID_HEIGHT; ++y)
    {
        MoveToEx(hdc, 0, iY, NULL);
        LineTo(hdc,(Tile::GRID_WIDTH - g_render_start_point._x) * (Tile::_GRID_SIZE + g_scroll_up), iY);
        iY += (Tile::_GRID_SIZE + g_scroll_up);
    }
    SelectObject(hdc, hOldPen);
}
void RenderObstacle(HDC hdc)
{

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);

    int iX = 0;
    int iY = 0;
    SelectObject(hdc, GetStockObject(NULL_PEN));
    for (int y = 0; y < Tile::GRID_HEIGHT; ++y)
    {
        for (int x = 0; x < Tile::GRID_WIDTH; ++x)
        {
            Coordinate pos{ x,y };
            switch ((STATE)g_Tile[pos])
            {
		    	case OBSTACLE:
					hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
					break;
				case START:
					hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(0, 255, 0));
					break;
				case DESTINATION:
					hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(255, 0, 0));
					break;
				case CANDIDATE: // �ĺ���
					hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(0, 0, 255)); // BLUE
					break;
				case VISITED: // �湮��
					hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(255, 0, 255)); // MAGENTA
					break;
                case SEARCHED: // JPS������ ��� Ž����
					hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(255, 255, 0)); // YELLOW
					break;
                default: //NOSTATE
                    break;
            }
            int extended_grid_size = Tile::_GRID_SIZE + g_scroll_up;
            Coordinate render_pixel = (pos - g_render_start_point) * extended_grid_size;
            // �����°� �ƴ� ��쿡�� �׸���.
            switch (g_Tile[pos])
            {
            case NOSTATE:
                break;
			case CANDIDATE:
			case VISITED:
                if (!g_draw_CANDIDATE_VISITED)
                    break;
            default:
				Rectangle(hdc, render_pixel._x + 1, render_pixel._y + 1, render_pixel._x + extended_grid_size + 1, render_pixel._y + extended_grid_size + 1);
                break;
            }


            if (g_draw_CANDIDATE_VISITED && g_scroll_up >= 60 && g_Tile[pos] != NOSTATE && g_Tile[pos] != OBSTACLE)
            {
                bool is_exist_in_openList = false;
                bool is_exist_in_closeList = false;
                do
                {
                    if (!g_isPrintPos)
                        break;

                    std::map<Coordinate, VERTEX*,pointCompare>* pMap = nullptr;

                    if (g_ASTAR._open_list_backup.find(pos) != g_ASTAR._open_list_backup.end())
                    {
                        is_exist_in_openList = true;
                        pMap = &pathfinder->_open_list_backup;
                    }
                    else if(g_ASTAR._close_list.find(pos) != g_ASTAR._close_list.end())
                    {
                        is_exist_in_closeList = true;
                        pMap = &pathfinder->_close_list;
                    }

                    // �Ѵ� ������
                    if (g_Tile[pos] != SEARCHED && !is_exist_in_openList&& !is_exist_in_closeList)
                        break;
					HFONT hFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
						CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
					SelectObject(hdc, hFont);
					SetTextAlign(hdc, TA_LEFT);
					WCHAR TEMP[MAX_PATH];
					SetBkMode(hdc, TRANSPARENT);
					SetTextColor(hdc, RGB(0, 0, 0)); // ���ڻ� ����
                    swprintf_s(TEMP, _countof(TEMP), L"(x,y):(%d,%d)", pos._x, pos._y);
                    TextOut(hdc, render_pixel._x + 1, render_pixel._y + 1, TEMP, lstrlen(TEMP));
                    if(g_Tile[pos] == SEARCHED)
                        continue;
                    swprintf_s(TEMP, _countof(TEMP), L"G : %.3f", (*pMap)[pos]->_G);
                    TextOut(hdc, render_pixel._x + 1, render_pixel._y + 13, TEMP, lstrlen(TEMP));
                    swprintf_s(TEMP, _countof(TEMP), L"H : %d", (*pMap)[pos]->_H);
                    TextOut(hdc, render_pixel._x + 1, render_pixel._y + 25, TEMP, lstrlen(TEMP));
                    swprintf_s(TEMP, _countof(TEMP), L"F : %.5f", (*pMap)[pos]->_F);
                    TextOut(hdc, render_pixel._x + 1, render_pixel._y + 37, TEMP, lstrlen(TEMP));
					DeleteObject(hFont);
                } while (false);
            }
        }
    }
    SelectObject(hdc, hOldBrush);
}
void RenderUI(HDC hdc)
{
	HFONT hFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
    SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, 10, 10, L"UI ǥ�� : ShiftŰ�� ON/OFF", lstrlen(L"UI ǥ�� : ShiftŰ�� ON/OFF"));
    if (g_drawUI)
    {
		WCHAR TEMP[MAX_PATH];
        TextOut(hdc,10,22,L"���콺 ���� ����Ŭ������ ������ ���� ����",lstrlen(L"1. ���콺 ���� ����Ŭ������ ������ ���� ����"));
		TextOut(hdc, 10, 34, L"���콺 ������ ����Ŭ������ ������ ���� ����", lstrlen(L"���콺 ������ ����Ŭ������ ������ ���� ����"));
		TextOut(hdc, 10, 46, L"��Ű�� ��� Ž��", lstrlen(L"��Ű�� ��� Ž��"));
		TextOut(hdc, 10, 58, L"1��Ű�� ������", lstrlen(L"1��Ű�� ������"));
		TextOut(hdc, 10, 70, L"2��Ű�� �湮��, �ĺ��� ����", lstrlen(L"2��Ű�� �湮��, �ĺ��� ����"));
        TextOut(hdc, 10, 82, L"3��Ű�� �湮��,�ĺ��� ��ĥ �Ѱ� ����", lstrlen(L"3��Ű�� �湮��,�ĺ��� ��ĥ �Ѱ� ����"));
		TextOut(hdc, 10, 94, L"4��Ű�� �ʱ���� ����", lstrlen(L"4��Ű�� �ʱ���� ����"));
        TextOut(hdc, 10, 106, L"5��Ű�� �� �����", lstrlen(L"5��Ű�� �� �����"));
        TextOut(hdc, 10, 118, L"6��Ű�� �� �����(������)", lstrlen(L"6��Ű�� �� �����(������)"));
        swprintf_s(TEMP,_countof(TEMP),L"7��Ű�� ��ã�� �����ȯ ���� ��� : %s",is_Astar ? L"Astar" : L"JPS");
        TextOut(hdc, 10, 130, TEMP, lstrlen(TEMP));
		TextOut(hdc, 10, 142, L"����Ű�� ��ĭ�� �̵�", lstrlen(L"����Ű�� ��ĭ�� �̵�"));
		if (g_ASTAR._isSet_start_point)
		{
			swprintf_s(TEMP, _countof(TEMP), L"��� ��ǥ (%d,%d)", g_ASTAR._start_Coordinate._x, g_ASTAR._start_Coordinate._y);
			TextOut(hdc, 10, 154, TEMP, lstrlen(TEMP));
		}
		if (g_ASTAR._isSet_destination)
		{
			swprintf_s(TEMP, _countof(TEMP), L"���� ��ǥ (%d,%d)", g_ASTAR._destination_Coordinate._x, g_ASTAR._destination_Coordinate._y);
            TextOut(hdc, 10, 166, TEMP, lstrlen(TEMP));
		}
    }
   	DeleteObject(hFont);

}
