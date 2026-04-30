#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>

#include "pathfinding.h"

using namespace std;

// 노드 구조체 (A* 탐색용)
struct Node {
    Point pt;
    int g, h, f; // g : 지금까지 온 거리(시작점부터 여기까지 비용), h : 남은 예상 거리(예상 비용 / 휴리스틱), f: g+h 총 합 점수
    Node* parent;

    Node(Point p, int g_val, int h_val, Node* p_node = nullptr)
        : pt(p), g(g_val), h(h_val), parent(p_node) {
        f = g + h;
    }
};

// 우선순위 큐를 위한 비교 연산자 (f값이 작을수록 우선순위 높음)
struct CompareNode {
    bool operator()(Node* n1, Node* n2) { return n1->f > n2->f; }
};

// 맨해튼 거리 계산 (Heuristic)
int calculateH(Point p1, Point p2) {
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

// 최적 경로 탐색 함수
vector<Point> findPath(Point start, Point end, int (*grid_map)[8]) {
    // 다음에 방문할 후보지 리스트, priority_queue 함수를 사용, CompareNode 정렬 기준을 따져 f값이 낮은 순으로 불러 옴
    priority_queue<Node*, vector<Node*>, CompareNode> openList;
    // 이미 방문한 경로 리스트
    vector<vector<bool>> closedList(7, vector<bool>(8, false));

    // 시작 노드 추가
    openList.push(new Node(start, 0, calculateH(start, end)));

    int dx[] = {0, 0, 1, -1}; // 상, 하, 우, 좌 이동
    int dy[] = {1, -1, 0, 0};

    while (!openList.empty()) {
        // 최초 while문 실행 시 시작 위치 가 current에 저장됨
        Node* current = openList.top();
        // current에 저장했으니 openList에서 삭제
        openList.pop();

        // 목적지 도착 시 경로 복원
        if (current->pt == end) {
            vector<Point> path;
            while (current != nullptr) {
                path.push_back(current->pt);
                current = current->parent;
            }
            reverse(path.begin(), path.end());
            return path;
        }

        // 현재 위치는 이미 방문한 리스트에 삽입
        closedList[current->pt.y][current->pt.x] = true;

        // 인접 4방향 탐색
        for (int i = 0; i < 4; i++) {
            int nx = current->pt.x + dx[i];
            int ny = current->pt.y + dy[i];

            // 맵 범위 내에 있고, 기둥(1)이 아니며, 이미 방문하지 않은 곳
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 7 &&
                grid_map[ny][nx] == 0 && !closedList[ny][nx]) {

                openList.push(new Node({nx, ny}, current->g + 1, calculateH({nx, ny}, end), current));
            }
        }
    }

    return {}; // 경로 없음
}

// 대피소 탐색 로직
vector<Point> findValidShelterPath(Point currentPos, const vector<Point>& shelters, int (*grid_map)[8]) {
    struct ShelterInfo {
        Point pt;
        int dist;
    };
    vector<ShelterInfo> candidates;

    // 1. 거리 계산 및 후보군 수집 (이미 있는 calculateH 활용)
    for (const auto& s : shelters) {
        candidates.push_back({s, calculateH(currentPos, s)});
    }

    // 2. 가까운 순으로 정렬
    sort(candidates.begin(), candidates.end(), [](const ShelterInfo& a, const ShelterInfo& b) {
        return a.dist < b.dist;
    });

    // 3. 차선책 검증 루프
    for (const auto& cand : candidates) {
        // 조건 1: 대피소 지점이 비어있는가 (0: 이동가능, 2: 임계구역 내 빈칸)
        // 1(장애물)이나 3(타 로봇)이 아닐 때만 진입 시도
        if (grid_map[cand.pt.y][cand.pt.x] == 0 || grid_map[cand.pt.y][cand.pt.x] == 2) {

            // 조건 2: 실제로 갈 수 있는 경로가 있는가?
            vector<Point> path = findPath(currentPos, cand.pt, grid_map);

            if (!path.empty()) {
                return path; // 갈 수 있는 가장 가까운 대피소 경로 반환
            }
        }
    }

    return {}; // 어떤 대피소로도 갈 수 없는 경우
}

// int main() {
//     Point start = {3, 0}; // 좌측 최하단 (배열 인덱스 주의)
//     Point end = {7, 6};   // 우측 최상단

//     vector<Point> result = findPath(start, end);

//     if (result.empty()) {
//         cout << "경로를 찾을 수 없습니다." << endl;
//     } else {
//         cout << "Best Path XY:" << endl;
//         for (auto p : result) {
//             cout << "{" << p.x << ", " << p.y << "}, ";
//         }
//         cout << "Arrived" << endl;
//     }

//     return 0;
// }