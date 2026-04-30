#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <vector>
#include <algorithm>

// 좌표 구조체
struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};


// 외부에서 사용할 함수
std::vector<Point> findPath(Point start, Point end, int (*grid_map)[8]);
// 사용 가능한 최선의 대피소 경로를 찾아 반환하는 함수
std::vector<Point> findValidShelterPath(Point currentPos, const std::vector<Point>& shelters, int (*grid_map)[8]);

#endif