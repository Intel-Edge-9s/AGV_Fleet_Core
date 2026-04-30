#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QString>

class MapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr);

    // 로봇 위치를 업데이트하고 화면을 다시 그리는 함수
    // robotID: "A" 또는 "B"
    // x: column, y: row
    void setRobotPosition(const QString& robotID, int x, int y);
    void updateMap(const int map[7][8]);  // ← 추가

protected:
    // 실제 그림을 그리는 함수
    void paintEvent(QPaintEvent *event) override;

private:
    int m_map[7][8] = {};  // ← 추가 (내부 맵 복사본)
    static constexpr int COLS = 8;
    static constexpr int ROWS = 7;

    // 두 로봇의 좌표를 각각 관리
    int robotAX = -1;
    int robotAY = -1;

    int robotBX = -1;
    int robotBY = -1;

    // 기존 mapwidget.cpp와 호환용 별칭
    // cpp에서 cols/rows를 쓰고 있으므로 유지
    const int cols = COLS;
    const int rows = ROWS;
};

#endif // MAPWIDGET_H