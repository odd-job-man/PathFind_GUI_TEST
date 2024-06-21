#pragma once
struct pointCompare {
	bool operator()(const Coordinate& lhs, const Coordinate& rhs) const;
};

class PATHFINDER
{
public:
	bool _is_find_path_success = false;
	static bool _isSet_start_point;
	static bool _isSet_destination;
	static Coordinate _destination_Coordinate;
	static Coordinate _start_Coordinate;
	static std::map<Coordinate, VERTEX*, pointCompare>_open_list_backup;
	static std::map<Coordinate,VERTEX*,pointCompare> _close_list;
	static VERTEX* _pCurrent_vertex;

	PATHFINDER(bool is_find_path_success = false);
	void test(HWND hWnd);
	void clearList_by_selection(bool is_clear_open_list, bool is_clear_close_list);
	virtual bool pathFind(HWND hWnd) = 0;
	static bool _isStart(void);
	void reset();
	static void drawPolyLine(HDC hdc);
};
