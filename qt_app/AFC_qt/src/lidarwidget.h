#ifndef LIDARWIDGET_H
#define LIDARWIDGET_H

#include <QWidget>
#include <QTimer>

#include <vector>

// 라이다 데이터 한 점의 정보를 담는 구조체
struct LidarPoint {
    float angle = 0.0f;  // 각도, radian
    float range = 0.0f;  // 거리, meter
};

class LidarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LidarWidget(QWidget *parent = nullptr);

    // ROS 노드로부터 받은 전체 스캔 데이터를 업데이트하는 함수
    void setScanData(const std::vector<LidarPoint>& newData);

protected:
    // 실제 그림을 그리는 함수
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<LidarPoint> scanData;

    static constexpr float MAX_RANGE = 5.0f;

    // 기존 lidarwidget.cpp와 호환하기 위한 별칭
    // cpp에서 maxRange를 사용하고 있으므로 유지
    const float maxRange = MAX_RANGE;

    QTimer *animationTimer = nullptr;

    float sweepAngle = 0.0f;
    float auraPulse = 0.0f;
};

#endif // LIDARWIDGET_H