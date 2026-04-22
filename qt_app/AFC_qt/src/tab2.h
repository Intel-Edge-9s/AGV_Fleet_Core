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

QT_BEGIN_NAMESPACE
namespace Ui { class Tab2; }
QT_END_NAMESPACE

class Tab2 : public QWidget
{
    Q_OBJECT

public:
    explicit Tab2(QWidget *parent = nullptr);
    ~Tab2();

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