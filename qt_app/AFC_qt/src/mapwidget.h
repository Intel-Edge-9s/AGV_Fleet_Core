#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QString>

class MapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    // 로봇 위치를 업데이트하고 화면을 다시 그리는 함수
    void setRobotPosition(const QString& robotID, int x, int y);

protected:
    // 실제 그림을 그리는 함수
    void paintEvent(QPaintEvent *event) override;

private:
    // 두 로봇의 좌표를 각각 관리
    int robotAX = -1; int robotAY = -1;
    int robotBX = -1; int robotBY = -1;
    const int cols = 8; // 가로 칸 수
    const int rows = 7; // 세로 칸 수
};

#endif