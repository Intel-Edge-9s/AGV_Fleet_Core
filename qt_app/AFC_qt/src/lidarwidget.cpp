#include "lidarwidget.h"

#include <QPaintEvent>
#include <QRadialGradient>
#include <QConicalGradient>
#include <algorithm>
#include <cmath>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QFont>

LidarWidget::LidarWidget(QWidget *parent) : QWidget(parent)
{
    // 배경을 더 깊은 검은색으로 설정하여 효과 극대화
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "background-color: #050505; "
        "border: 2px solid #3e3e4e; "
        "border-radius: 5px;"
        );

    sweepAngle = 0.0f;
    auraPulse = 0.0f;

    // 60FPS 애니메이션 타이머 설정
    animationTimer = new QTimer(this);

    connect(animationTimer, &QTimer::timeout, this, [this]() {
        // 스캔 빔 회전
        sweepAngle += 6.0f;
        if (sweepAngle >= 360.0f) {
            sweepAngle -= 360.0f;
        }

        // 맥박 효과
        static float time = 0.0f;
        time += 0.1f;
        auraPulse = (std::sin(time) + 1.0f) / 2.0f;

        update();
    });

    animationTimer->start(16);
}

void LidarWidget::setScanData(const std::vector<LidarPoint>& newData)
{
    scanData.clear();
    scanData.reserve(newData.size());

    for (const auto& point : newData) {
        // 유효하지 않은 range 제거
        if (!std::isfinite(point.range)) {
            continue;
        }

        if (point.range <= 0.05f || point.range > maxRange) {
            continue;
        }

        if (!std::isfinite(point.angle)) {
            continue;
        }

        scanData.push_back(point);
    }

    // 데이터 수신 즉시 한 번 갱신
    update();
}

void LidarWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int widgetWidth = width();
    const int widgetHeight = height();

    if (widgetWidth <= 2 || widgetHeight <= 2) {
        return;
    }

    // 위젯 중심을 원점으로 이동
    const int centerX = widgetWidth / 2;
    const int centerY = widgetHeight / 2;
    painter.translate(centerX, centerY);

    // width만 쓰면 화면 비율에 따라 원이 잘릴 수 있으므로 짧은 변 기준 사용
    const float drawableRadius = static_cast<float>(std::min(widgetWidth, widgetHeight)) * 0.45f;
    const float scale = drawableRadius / maxRange;

    // ---------------------------------------------------------
    // 1. 배경 아우라
    // ---------------------------------------------------------
    const float auraRadius = drawableRadius * 0.95f;

    QRadialGradient auraGradient(0, 0, auraRadius);

    int alphaValue = 20 + static_cast<int>(40.0f * auraPulse);
    auraGradient.setColorAt(0.0, QColor(0, 255, 255, alphaValue));
    auraGradient.setColorAt(0.7, QColor(0, 180, 220, alphaValue / 3));
    auraGradient.setColorAt(1.0, QColor(0, 255, 255, 0));

    painter.setBrush(auraGradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), auraRadius, auraRadius);

    // ---------------------------------------------------------
    // 2. 회전하는 스캔 빔
    // ---------------------------------------------------------
    QConicalGradient sweepGradient(0, 0, -sweepAngle);
    sweepGradient.setColorAt(0.00, QColor(0, 255, 255, 130));
    sweepGradient.setColorAt(0.04, QColor(0, 255, 255, 80));
    sweepGradient.setColorAt(0.12, QColor(0, 255, 255, 25));
    sweepGradient.setColorAt(0.20, QColor(0, 255, 255, 0));

    painter.setBrush(sweepGradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), drawableRadius, drawableRadius);

    // ---------------------------------------------------------
    // 3. 거리 가이드라인
    // ---------------------------------------------------------
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0x33, 0x33, 0x33), 1, Qt::DotLine));

    for (int r = 1; r <= static_cast<int>(maxRange); ++r) {
        const float radius = static_cast<float>(r) * scale;
        painter.drawEllipse(QPointF(0, 0), radius, radius);
    }

    // 십자 기준선
    painter.setPen(QPen(QColor(0x25, 0x25, 0x25), 1, Qt::DashLine));
    painter.drawLine(QPointF(-drawableRadius, 0), QPointF(drawableRadius, 0));
    painter.drawLine(QPointF(0, -drawableRadius), QPointF(0, drawableRadius));

    // ---------------------------------------------------------
    // 4. LiDAR 포인트 표시
    // ---------------------------------------------------------
    painter.setPen(QPen(QColor(0xff, 0x33, 0x33), 4.0));

    for (const auto& point : scanData) {
        const float x = point.range * std::cos(point.angle) * scale;
        const float y = -point.range * std::sin(point.angle) * scale;

        painter.drawPoint(QPointF(x, y));
    }

    // ---------------------------------------------------------
    // 5. 중심 센서 표시
    // ---------------------------------------------------------
    painter.setBrush(QColor(0x00, 0xff, 0x66));
    painter.setPen(QPen(QColor(0xff, 0xff, 0xff), 1));
    painter.drawEllipse(QPointF(0, 0), 6, 6);

    // ---------------------------------------------------------
    // 6. 상태 텍스트
    // ---------------------------------------------------------
    painter.resetTransform();

    painter.setPen(QColor(0x8a, 0x8a, 0x9a));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    QString statusText = QString("LiDAR Points: %1").arg(scanData.size());
    painter.drawText(
        QRect(10, 10, widgetWidth - 20, 25),
        Qt::AlignLeft | Qt::AlignVCenter,
        statusText
        );
}