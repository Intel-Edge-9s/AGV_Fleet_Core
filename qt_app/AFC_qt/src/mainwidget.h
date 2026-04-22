#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "tab1.h"
#include "tab2.h"
#include "rosnode.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

private:
    Ui::MainWidget *ui;
    Tab1 *m_tab1;
    Tab2 *m_tab2;
    RosNode *m_rosNode;
};
#endif // MAINWIDGET_H