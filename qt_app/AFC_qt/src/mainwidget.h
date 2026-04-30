#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

#include "tab1.h"
#include "tab2.h"
#include "rosnode.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;
}
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

private:
    Ui::MainWidget *ui = nullptr;

    Tab1 *m_tab1 = nullptr;
    Tab2 *m_tab2 = nullptr;

    // ROS2 spin을 담당하는 QThread 기반 노드
    RosNode *m_rosNode = nullptr;
};

#endif // MAINWIDGET_H