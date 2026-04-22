#include "lidarwidget.h"

LidarWidget::LidarWidget(QWidget *parent) : QWidget(parent)
{
    // 배경을 더 깊은 검은색으로 설정하여 효과 극대화
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #050505; border: 2px solid #3e3e4e; border-radius: 5px;");

    sweepAngle = 0.0f;
    auraPulse = 0.0f;

    // 60FPS 애니메이션 타이머 설정
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, [this](){
        // 스캔 빔 회전 (초당 약 1바퀴 속도)
        sweepAngle += 6.0f;
        if (sweepAngle >= 360.0f) sweepAngle = 0.0f;

        // 맥박(Aura) 효과를 위한 Sine 함수 계산
        static float time = 0.0f;
        time += 0.1f;
        auraPulse = (sin(time) + 1.0f) / 2.0f; // 0.0 ~ 1.0 사이 값 반복

        update(); // 화면 갱신 호출 (paintEvent 실행)
    });
    animationTimer->start(16); // 약 16ms 주기로 실행
}

void LidarWidget::setScanData(const std::vector<LidarPoint>& newData)
{
    scanData = newData;
    // update()는 타이머에서 주기적으로 호출되므로 여기서는 생략 가능하지만, 데이터 수신 즉시 반응을 원하면 남겨두어도 좋다.
}

void LidarWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 위젯 중심을 (0,0)으로 이동
    int centerX = width() / 2;
    int centerY = height() / 2;
    painter.translate(centerX, centerY);

    // 스케일 계산 (maxRange가 5m일 때 위젯 크기에 맞춤)
    float scale = (float)width() / (maxRange * 2);


    // 배경 아우라 (Pulsing Aura) - 은은하게 깜빡이는 효과
    float auraRadius = maxRange * scale * 0.9f;
    QRadialGradient auraGradient(0, 0, auraRadius);

    // auraPulse에 따라 투명도(Alpha)를 20 ~ 60 사이로 조절
    int alphaValue = 20 + (int)(40 * auraPulse);
    auraGradient.setColorAt(0.0, QColor(0, 255, 255, alphaValue)); // 중심: 민트색
    auraGradient.setColorAt(1.0, QColor(0, 255, 255, 0));          // 가장자리: 투명

    painter.setBrush(auraGradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), auraRadius, auraRadius);

    // 2. 회전하는 스캔 빔 (Sweep Beam) - 레이더 회전 효과
    // QConicalGradient는 시계 반대방향이 기본이므로 -sweepAngle 사용
    QConicalGradient sweepGradient(0, 0, -sweepAngle);
    sweepGradient.setColorAt(0.0, QColor(0, 255, 255, 120)); // 선두 부분 (밝음)
    sweepGradient.setColorAt(0.1, QColor(0, 255, 255, 40));  // 중간 부분 (잔상)
    sweepGradient.setColorAt(0.2, QColor(0, 255, 255, 0));   // 꼬리 부분 (투명)

    painter.setBrush(sweepGradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), maxRange * scale, maxRange * scale);


    // 가이드라인 (Grid) - 1m 단위 동심원
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#333333"), 1, Qt::DotLine));
    for (int r = 1; r <= (int)maxRange; ++r) {
        painter.drawEllipse(QPointF(0, 0), r * scale, r * scale);
    }


    // 라이다 데이터 포인트 (Obstacles)
    painter.setPen(QPen(QColor("#ff3333"), 3.5)); // 장애물은 강렬한 빨간색 점
    for (const auto& point : scanData) {
        if (point.range <= 0.1f || point.range > maxRange) continue;

        // 극좌표 -> 직교좌표 변환
        float x = point.range * cos(point.angle) * scale;
        float y = -point.range * sin(point.angle) * scale;

        painter.drawPoint(QPointF(x, y));
    }

    // ---------------------------------------------------------
    // 중심부 센서 표시
    // ---------------------------------------------------------
    painter.setBrush(Qt::green);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), 5, 5);
}