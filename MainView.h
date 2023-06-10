#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QDialog>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "CopyThread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainView; }
QT_END_NAMESPACE

class MainView : public QDialog
{
    Q_OBJECT

public:

    MainView(QWidget *parent = nullptr);
    ~MainView();

    // 初始化
    void init();

    // 判断按键使能
    void toggleCopyAndCutEnabled();

    // 设置界面使能
    void setMainViewEnabled(bool enabled);

protected:

    // 拷贝 / 移动
    void startProcess(int type);

    // 文件拖拽
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    // 窗口关闭
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::MainView *ui;

    // 拷贝线程
    CopyThread* m_copyThread;

};
#endif // MAINVIEW_H
