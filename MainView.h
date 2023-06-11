#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QDialog>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
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

    // 操作类型
    enum class CommandType {
        CT_Copy,
        CT_Move,
        CT_Undo,
    };

    // 开始操作
    void startProcess(CommandType type);

    // 文件拖拽
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    // 拦截 ESC 事件
    void keyPressEvent(QKeyEvent* event) override;

    // 窗口关闭
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::MainView *ui;

    // 记录上一次的操作（只记录拷贝还是剪切）
    CommandType m_lastCommandType;

    // 拷贝线程
    CopyThread* m_copyThread;

    // 输出文件夹
    QString m_outDirectory;

};
#endif // MAINVIEW_H
