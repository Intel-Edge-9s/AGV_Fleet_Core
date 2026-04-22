#include "tab2.h"
#include "ui_tab2.h"

Tab2::Tab2(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tab2)
{
    ui->setupUi(this);

    dataTimer = nullptr;
    historyModel = nullptr;
    inventoryModel = nullptr;

    // [1] UI 기초 설정: 실시간 시계 (데이터 타이머와 분리)
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, [=](){
        ui->lbl_clock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    clockTimer->start(1000);

    // [2] 데이터베이스 연결
    setupDatabase();

    // [3] 데이터 모델 초기화
    historyModel = new QSqlTableModel(this, db);
    inventoryModel = new QSqlTableModel(this, db);

    // [4] DB가 열려있을 때만 상세 데이터 뷰 설정
    if (db.isOpen()) {
        setupHistoryView();
        setupInventoryView();
    }

    // [5] 시각화 및 실시간 기능 시동
    setupInventoryChart();
    updateChart();
    updateRobotList();
    setupRealTimeTimer();
}

Tab2::~Tab2()
{
    if (db.isOpen()) db.close();
    delete ui;
}

// DB 연결
void Tab2::setupDatabase() {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        db = QSqlDatabase::database("qt_sql_default_connection");

        if (db.isOpen() && db.isValid()) {
            qDebug() << "기존 DB 연결이 이미 활성화되어 있어 재사용합니다.";
            return;
        }
    } else {
        db = QSqlDatabase::addDatabase("QMARIADB");
    }

    db.setHostName("127.0.0.1");
    db.setDatabaseName("amr_turtle_db");
    db.setUserName("iot");
    db.setPassword("pwiot");

    if (!db.open()) {
        qDebug() << "------------------------------------------";
        qDebug() << "CRITICAL: MariaDB 연결에 실패했습니다!";
        qDebug() << "Reason:" << db.lastError().text();
        qDebug() << "Driver Error:" << db.lastError().driverText();
        qDebug() << "------------------------------------------";
    } else {
        qDebug() << "MariaDB 연결 성공! [amr_turtle_db] (Tab2)";
    }
}

// 테이블 뷰 틀 잡기
void Tab2::setupHistoryView() {
    historyModel->setTable("command_msg");
    historyModel->setSort(7, Qt::DescendingOrder);
    historyModel->select();

    ui->tv_history->setModel(historyModel);

    ui->tv_history->setColumnHidden(0, true);
    ui->tv_history->setColumnHidden(5, true);

    historyModel->setHeaderData(2, Qt::Horizontal, tr("구역"));
    historyModel->setHeaderData(3, Qt::Horizontal, tr("결과"));
    historyModel->setHeaderData(4, Qt::Horizontal, tr("동작"));
    historyModel->setHeaderData(7, Qt::Horizontal, tr("시간"));

    ui->tv_history->resizeColumnsToContents();
}

// 테이블 뷰 틀 잡기
void Tab2::setupInventoryView() {
    inventoryModel->setTable("product_inventory");
    inventoryModel->select();

    ui->tv_inventory->setModel(inventoryModel);

    ui->tv_inventory->setColumnHidden(0, true);
    ui->tv_inventory->setColumnHidden(4, true);
    ui->tv_inventory->setColumnHidden(5, true);

    inventoryModel->setHeaderData(1, Qt::Horizontal, tr("아이템명"));
    inventoryModel->setHeaderData(2, Qt::Horizontal, tr("유형"));
    inventoryModel->setHeaderData(3, Qt::Horizontal, tr("재고"));
    inventoryModel->setHeaderData(6, Qt::Horizontal, tr("단위"));

    ui->date_start->setDate(QDate::currentDate().addDays(-7));
    ui->date_end->setDate(QDate::currentDate());
}

// 차트 외형 설정
void Tab2::setupInventoryChart() {
    chart = new QChart();
    chart->setTitle("실시간 재고 현황");
    chart->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    series = new QStackedBarSeries();
    chart->addSeries(series);

    axisX = new QBarCategoryAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QValueAxis();
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");

    if (!ui->chart_container->layout()) {
        new QVBoxLayout(ui->chart_container);
    }

    ui->chart_container->layout()->setContentsMargins(0, 0, 0, 0);
    ui->chart_container->layout()->addWidget(chartView);

    chart->setTitleBrush(QBrush(Qt::white));
    QFont titleFont = chart->titleFont();
    titleFont.setBold(true);
    titleFont.setPointSize(14);
    chart->setTitleFont(titleFont);

    axisX->setLabelsBrush(QBrush(Qt::white));
    QFont axisFont = axisX->labelsFont();
    axisFont.setWeight(QFont::Medium);
    axisX->setLabelsFont(axisFont);

    axisY->setLabelsBrush(QBrush(Qt::white));

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(Qt::white);
}

// 타이머 가동
void Tab2::setupRealTimeTimer() {
    if (dataTimer) {
        dataTimer->stop();
        delete dataTimer;
        dataTimer = nullptr;
    }
    dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, &Tab2::updateRealTimeData);
    dataTimer->start(2000);
}

// 타이머 슬롯
void Tab2::updateRealTimeData() {
    if (!db.isOpen()) return;
    updateRobotList();
    if (historyModel) {
        historyModel->select();
    }
}

// 로봇 카드 갱신
void Tab2::updateRobotList() {
    QVBoxLayout *listLayout = qobject_cast<QVBoxLayout*>(ui->vl_robotList_2->layout());
    if (!listLayout) return;

    QStringList activeRobotNames;
    QSqlQuery query("SELECT turtlebot_name, status FROM turtlebot_info");
    int row = 1;

    while (query.next()) {
        QString name = query.value(0).toString();
        QString status = query.value(1).toString();
        activeRobotNames << name;

        QFrame *existingCard = ui->vl_robotList_2->findChild<QFrame*>(name);

        if (existingCard) {
            QLabel *lblNo = existingCard->findChild<QLabel*>("lblNo");
            QLabel *lblIcon = existingCard->findChild<QLabel*>("lblIcon");
            QLabel *lblStatus = existingCard->findChild<QLabel*>("lblStatus");

            if (lblNo) lblNo->setText(QString::number(row++));
            if (lblStatus) lblStatus->setText(status);

            if (lblIcon) {
                if (status == "정상") {
                    lblIcon->setStyleSheet("color: #4CAF50; font-size: 18px;");
                } else {
                    lblIcon->setStyleSheet("color: #F44336; font-size: 18px;");
                }
            }
        } else {
            QFrame *card = new QFrame();
            card->setObjectName(name);
            card->setMinimumHeight(55);
            card->setMaximumHeight(55);
            card->setStyleSheet("QFrame { background-color: #3D3D3D; border-radius: 8px; margin: 2px; } "
                                "QFrame:hover { background-color: #4D4D4D; border: 1px solid #0078D7; }");

            QHBoxLayout *cardLayout = new QHBoxLayout(card);
            cardLayout->setContentsMargins(15, 0, 15, 0);
            cardLayout->setSpacing(10);

            QLabel *lblNo = new QLabel(QString::number(row++));
            lblNo->setObjectName("lblNo");
            lblNo->setFixedWidth(30);
            lblNo->setStyleSheet("color: #AAAAAA;");

            QLabel *lblName = new QLabel(name);
            lblName->setFixedWidth(100);
            lblName->setStyleSheet("font-weight: bold; color: white;");

            QLabel *lblIcon = new QLabel("●");
            lblIcon->setObjectName("lblIcon");
            lblIcon->setFixedWidth(30);
            lblIcon->setAlignment(Qt::AlignCenter);

            QLabel *lblStatus = new QLabel(status);
            lblStatus->setObjectName("lblStatus");
            lblStatus->setFixedWidth(80);
            lblStatus->setAlignment(Qt::AlignCenter);

            if (status == "정상") {
                lblIcon->setStyleSheet("color: #4CAF50; font-size: 18px;");
            } else {
                lblIcon->setStyleSheet("color: #F44336; font-size: 18px;");
            }

            cardLayout->addWidget(lblNo);
            cardLayout->addWidget(lblName);
            cardLayout->addWidget(lblIcon);
            cardLayout->addWidget(lblStatus);

            listLayout->insertWidget(listLayout->count() - 1, card);
        }
    }

    for (int i = 0; i < listLayout->count() - 1; ++i) {
        QLayoutItem *item = listLayout->itemAt(i);
        if (QWidget *w = item->widget()) {
            if (!activeRobotNames.contains(w->objectName())) {
                listLayout->takeAt(i);
                w->deleteLater();
                --i;
            }
        }
    }
}

// 차트 막대 갱신
void Tab2::updateChart() {
    if (!inventoryModel) return;

    series->clear();
    axisX->clear();

    if (inventoryModel->rowCount() == 0) {
        axisY->setRange(0, 100);
        chart->setTitle("검색 결과가 없습니다.");
        return;
    }

    chart->setTitle("실시간 재고 현황");

    QStringList categories;
    int maxStock = 0;

    for (int i = 0; i < inventoryModel->rowCount(); ++i) {
        QString name = inventoryModel->data(inventoryModel->index(i, 1)).toString();
        int stock = inventoryModel->data(inventoryModel->index(i, 3)).toInt();

        QBarSet *set = new QBarSet(name);

        for (int j = 0; j < i; ++j) {
            *set << 0;
        }

        *set << stock;

        if (stock < 20) {
            set->setBrush(QBrush(QColor("#FF5252")));
        } else if (stock < 50) {
            set->setBrush(QBrush(QColor("#FFAB40")));
        } else {
            set->setBrush(QBrush(QColor("#40C4FF")));
        }

        series->append(set);
        categories << name;

        if (stock > maxStock) maxStock = stock;
    }

    axisX->append(categories);
    int yRange = (maxStock < 100) ? 100 : maxStock + 20;
    axisY->setRange(0, yRange);
    axisY->setTickCount(6);
}

// search 버튼 클릭
void Tab2::on_btn_search_inv_clicked()
{
    if (!inventoryModel) return;

    QDate startDate = ui->date_start->date();
    QDate endDate = ui->date_end->date();

    if (startDate > endDate) {
        qDebug() << "검색 실패: 시작일이 종료일보다 늦습니다.";
        return;
    }

    QString startStr = startDate.toString("yyyy-MM-dd");
    QString endStr = endDate.toString("yyyy-MM-dd");
    QString filter = QString("updated_at BETWEEN '%1' AND '%2'").arg(startStr, endStr);

    inventoryModel->setFilter(filter);

    if (inventoryModel->select()) {
        updateChart();
    } else {
        qDebug() << "조회 에러:" << inventoryModel->lastError().text();
    }
}