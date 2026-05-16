#ifndef TAB2_H
#define TAB2_H

#include <QWidget>
#include <QTimer>
#include <QDateTime>

// DB 및 차트 관련 헤더
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QtCharts>
#include <QVBoxLayout>
// 표에 그리기 기능을 커스텀하기 위한 헤더
#include <QStyledItemDelegate>
#include <QPainter>

QT_BEGIN_NAMESPACE
namespace Ui { class Tab2; }
QT_END_NAMESPACE

// 재고 수량에 따라 표 안에 색칠된 원을 그려주는 클래스
class StatusDelegate : public QStyledItemDelegate {
public:
    explicit StatusDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        // 1. 기본 배경(선택 효과 등) 그리기
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text = "";

        option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);

        // 2. 같은 줄에 있는 3번 컬럼(재고)의 숫자를 가져옴
        int stock = index.sibling(index.row(), 4).data().toInt();

        // 3. 수량에 따른 색상 결정
        QColor color;
        if (stock < 20) {
            color = QColor("#FF5252"); // 20개 미만: 빨강
        } else if (stock < 50) {
            color = QColor("#FFAB40"); // 50개 미만: 주황
        } else {
            color = QColor("#40C4FF"); // 그 외: 파랑
        }

        // 4. 동그라미 그리기
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true); // 테두리 부드럽게
        painter->setBrush(color);
        painter->setPen(Qt::NoPen); // 테두리 선 없음

        int radius = 8; // 원의 반지름 크기
        QPoint center = option.rect.center(); // 칸의 정중앙 좌표
        painter->drawEllipse(center, radius, radius);
        painter->restore();
    }
};

class Tab2 : public QWidget
{
    Q_OBJECT

public:
    explicit Tab2(QWidget *parent = nullptr);
    ~Tab2();

public slots:
    void addCommandRecord(const QString& robotName, const QString& dest,
                          const QString& action, const QString& result,
                          const QString& itemName, int count);

private slots:
    void on_btn_search_inv_clicked(); // 재고 검색 버튼
    void updateRealTimeData();        // 실시간 갱신 슬롯

private:
    Ui::Tab2 *ui;

    // 데이터베이스 관련
    QSqlDatabase db;
    QTimer *dataTimer;
    QSqlTableModel *historyModel;
    QSqlTableModel *inventoryModel;

    void setupDatabase();
    void cleanupOldData(); //  DB에 이미 저장되어 있는 길고 복잡한 UUID와 영문 구역(S, Z, X, D 등)을 터틀봇_A, 터틀봇_B 및 하역장X, 상차장 등으로 일괄 변환하는 함수
    void setupRealTimeTimer();
    void setupHistoryView();
    void setupInventoryView();
    void updateRobotList();

    // 차트 관련 객체
    QChart            *chart;
    QStackedBarSeries *series;
    QBarCategoryAxis  *axisX;
    QValueAxis        *axisY;
    QChartView        *chartView;

    void setupInventoryChart(); // 차트 초기 설정
    void updateChart();          // 차트 데이터 갱신
};

#endif // TAB2_H
