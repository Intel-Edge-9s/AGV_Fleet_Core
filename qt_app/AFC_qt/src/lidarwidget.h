#ifndef LIDARWIDGET_H
#define LIDARWIDGET_H

#include <QWidget>
#include <QPainter>
#include <vector>
#include <cmath>
#include <QTimer>

// 라이다 데이터 한 점의 정보를 담는 구조체
struct LidarPoint {
    float angle;  // 각도 (radian)
    float range;  // 거리 (m)
};

class LidarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LidarWidget(QWidget *parent = nullptr);

    // ROS 노드로부터 받은 전체 스캔 데이터를 업데이트하는 함수
    void setScanData(const std::vector<LidarPoint>& newData);

protected:
    // 실제 그림을 그리는 함수 (Qt 시스템에 의해 자동 호출됨)
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<LidarPoint> scanData; // 라이다 점 구름 데이터 저장소
    const float maxRange = 5.0f;     // 시각화할 최대 반경 (5m)

    QTimer *animationTimer;   // 애니메이션 전용 타이머
    float sweepAngle = 0.0f; // 현재 스캔 빔의 각도
    float auraPulse = 0.0f;  // 아우라의 불투명도 조절용 변수
};

#endif // LIDARWIDGET_H