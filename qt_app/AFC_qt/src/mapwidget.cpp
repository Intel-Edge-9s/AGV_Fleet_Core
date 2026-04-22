#include "mapwidget.h"

MapWidget::MapWidget(QWidget *parent) : QWidget(parent)
{
    // 배경색을 대시보드 테마에 맞춰 어둡게 설정
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #1e1e2d; border: 2px solid #3e3e4e; border-radius: 5px;");
}

void MapWidget::setRobotPosition(const QString& robotID, int x, int y)
{
    if (robotID == "A") {
        robotAX = x; robotAY = y;
    } else if (robotID == "B") {
        robotBX = x; robotBY = y;
    }
    update(); // 화면을 새로 고침 (paintEvent를 강제로 호출함)
}

void MapWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 부드럽게 그리기

    int cellWidth = width() / cols;
    int cellHeight = height() / rows;

    // 격자(Grid) 그리기
    painter.setPen(QPen(QColor("#333344"), 1)); // 선 색상
    for (int i = 0; i <= cols; ++i)
        painter.drawLine(i * cellWidth, 0, i * cellWidth, height()); // 세로줄
    for (int j = 0; j <= rows; ++j)
        painter.drawLine(0, j * cellHeight, width(), j * cellHeight); // 가로줄

    // 기둥(Column) 그리기 (0,0부터 가로 3칸, 세로 4칸 영역)
    painter.setBrush(QColor("#2d2d3d"));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, cellWidth * 3, cellHeight * 4);

    // 기둥 텍스트 표시
    painter.setPen(QColor("#777788"));
    painter.drawText(QRect(0, 0, cellWidth * 3, cellHeight * 4), Qt::AlignCenter, "기둥 영역");

    // 터틀봇 B 표시 (주황색 테마)
    if (robotBX >= 0 && robotBX < cols && robotBY >= 0 && robotBY < rows) {
        painter.setBrush(QColor("#ffab00")); // 주의/작업 색상
        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(robotBX * cellWidth, robotBY * cellHeight, cellWidth, cellHeight);

        painter.setPen(Qt::black);
        QFont font = painter.font(); font.setBold(true); painter.setFont(font);
        painter.drawText(QRect(robotBX * cellWidth, robotBY * cellHeight, cellWidth, cellHeight), Qt::AlignCenter, "BOT B");
    }

    // 터틀봇 A 표시 (사이언 테마)
    // A를 나중에 그려서 겹칠 경우 A가 위에 보이게 함
    if (robotAX >= 0 && robotAX < cols && robotAY >= 0 && robotAY < rows) {
        painter.setBrush(QColor("#00ffff")); // 안전/대기 색상
        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(robotAX * cellWidth, robotAY * cellHeight, cellWidth, cellHeight);

        painter.setPen(Qt::black);
        QFont font = painter.font(); font.setBold(true); painter.setFont(font);
        painter.drawText(QRect(robotAX * cellWidth, robotAY * cellHeight, cellWidth, cellHeight), Qt::AlignCenter, "BOT A");
    }
}