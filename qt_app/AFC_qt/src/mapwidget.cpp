#include "mapwidget.h"

#include <QPaintEvent>
#include <algorithm>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QRect>

MapWidget::MapWidget(QWidget *parent) : QWidget(parent)
{
    // 배경색을 대시보드 테마에 맞춰 어둡게 설정
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "background-color: #1e1e2d; "
        "border: 2px solid #3e3e4e; "
        "border-radius: 5px;"
        );
}

void MapWidget::setRobotPosition(const QString& robotID, int x, int y)
{
    if (robotID == "A") {
        robotAX = x;
        robotAY = y;
    }
    else if (robotID == "B") {
        robotBX = x;
        robotBY = y;
    }

    update(); // 화면 새로 고침
}

void MapWidget::updateMap(const int map[7][8]) {
    for (int r = 0; r < 7; ++r)
        for (int c = 0; c < 8; ++c)
            m_map[r][c] = map[r][c];
    update();
}

void MapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int cellWidth = std::max(1, width() / cols);
    const int cellHeight = std::max(1, height() / rows);

    const int mapWidth = cellWidth * cols;
    const int mapHeight = cellHeight * rows;

    // ---------------------------------------------------------
    // 1. 지도 배경
    // ---------------------------------------------------------
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x1e, 0x1e, 0x2d));
    painter.drawRect(0, 0, mapWidth, mapHeight);

    // ---------------------------------------------------------
    // 2. world_map 기반 셀 색상
    // ---------------------------------------------------------
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            QRect cellRect(col * cellWidth, row * cellHeight, cellWidth, cellHeight);

            if (m_map[row][col] == 1) {
                // 벽/장애물 → 붉은색
                painter.setBrush(QColor(0x5a, 0x10, 0x10));
                painter.setPen(QPen(QColor(0xff, 0x44, 0x44), 1));
                painter.drawRect(cellRect);

                // 'X' 표시
                painter.setPen(QPen(QColor(0xff, 0x44, 0x44, 180), 1));
                painter.drawLine(cellRect.topLeft(), cellRect.bottomRight());
                painter.drawLine(cellRect.topRight(), cellRect.bottomLeft());

            } else if (m_map[row][col] == 2) {
                // 임계구역 → 노란색
                painter.setBrush(QColor(0x3a, 0x30, 0x00));
                painter.setPen(QPen(QColor(0xff, 0xcc, 0x00), 1));
                painter.drawRect(cellRect);
            }
            // 0, 3은 기본 배경 유지
        }
    }

    // ---------------------------------------------------------
    // 3. 격자선 그리기
    // ---------------------------------------------------------
    painter.setPen(QPen(QColor(0x33, 0x33, 0x44), 1));

    for (int i = 0; i <= cols; ++i) {
        int x = i * cellWidth;
        painter.drawLine(x, 0, x, mapHeight);
    }

    for (int j = 0; j <= rows; ++j) {
        int y = j * cellHeight;
        painter.drawLine(0, y, mapWidth, y);
    }

    // ---------------------------------------------------------
    // 4. 좌표 표시
    // ---------------------------------------------------------
    QFont coordFont = painter.font();
    coordFont.setBold(false);
    coordFont.setPointSize(8);
    painter.setFont(coordFont);
    painter.setPen(QColor(0x5f, 0x64, 0x75));

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            QRect cellRect(
                x * cellWidth,
                y * cellHeight,
                cellWidth,
                cellHeight
                );

            painter.drawText(
                cellRect.adjusted(4, 2, -4, -2),
                Qt::AlignTop | Qt::AlignLeft,
                QString("%1,%2").arg(x).arg(y)
                );
        }
    }

    // ---------------------------------------------------------
    // 5. 로봇 그리기 함수
    // ---------------------------------------------------------
    auto drawRobot = [&](int gridX,
                         int gridY,
                         const QString& label,
                         const QColor& fillColor,
                         bool smallOffset)
    {
        if (gridX < 0 || gridX >= cols || gridY < 0 || gridY >= rows) {
            return;
        }

        QRect robotRect(
            gridX * cellWidth,
            gridY * cellHeight,
            cellWidth,
            cellHeight
            );

        robotRect = robotRect.adjusted(5, 5, -5, -5);

        // 같은 칸에 두 로봇이 있을 때 조금 작게 표시
        if (smallOffset) {
            robotRect = robotRect.adjusted(7, 7, -7, -7);
        }

        painter.setBrush(fillColor);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawRoundedRect(robotRect, 8, 8);

        painter.setPen(Qt::black);

        QFont robotFont = painter.font();
        robotFont.setBold(true);
        robotFont.setPointSize(10);
        painter.setFont(robotFont);

        painter.drawText(robotRect, Qt::AlignCenter, label);
    };

    const bool robotAValid =
        robotAX >= 0 && robotAX < cols &&
        robotAY >= 0 && robotAY < rows;

    const bool robotBValid =
        robotBX >= 0 && robotBX < cols &&
        robotBY >= 0 && robotBY < rows;

    const bool robotsOverlap =
        robotAValid &&
        robotBValid &&
        robotAX == robotBX &&
        robotAY == robotBY;

    // ---------------------------------------------------------
    // 6. 로봇 위치 표시
    // ---------------------------------------------------------
    if (robotsOverlap) {
        QRect overlapRect(
            robotAX * cellWidth,
            robotAY * cellHeight,
            cellWidth,
            cellHeight
            );

        overlapRect = overlapRect.adjusted(5, 5, -5, -5);

        painter.setBrush(QColor(0xff, 0x4d, 0x4d));
        painter.setPen(QPen(Qt::white, 2));
        painter.drawRoundedRect(overlapRect, 8, 8);

        painter.setPen(Qt::white);

        QFont overlapFont = painter.font();
        overlapFont.setBold(true);
        overlapFont.setPointSize(10);
        painter.setFont(overlapFont);

        painter.drawText(overlapRect, Qt::AlignCenter, "A / B\n충돌");
    }
    else {
        // B 먼저 그림
        drawRobot(robotBX, robotBY, "BOT B", QColor(0xff, 0xab, 0x00), false);

        // A 나중에 그림
        drawRobot(robotAX, robotAY, "BOT A", QColor(0x00, 0xff, 0xff), false);
    }

    // ---------------------------------------------------------
    // 7. 외곽선
    // ---------------------------------------------------------
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0x55, 0x55, 0x66), 2));
    painter.drawRect(0, 0, mapWidth - 1, mapHeight - 1);
}