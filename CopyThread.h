#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include <QObject>
#include <QThread>

class CopyThread : public QThread
{
    Q_OBJECT

public:

    CopyThread(QObject *parent = nullptr);
    ~CopyThread();

    // 设置拷贝信息
    void setCopyInfo(const QString& srcDir,
                     const QString& dstDir,
                     const QString& targetFile,
                     const int type);

signals:

    // 开始
    void sigStart();

    // 结束
    void sigStop();

    // 拷贝失败的文件
    void sigCopyFailedItem(const QString& fileName);

    // 实时进度
    void sigProgress(int value);

    // 拷贝异常
    void sigCopyException();

protected:

    // 核心函数
    void run() override;

private:

    // 源文件夹、目标文件夹、文件集合
    std::tuple<QString, QString, QString, int> m_copyInfo;

};

#endif // COPYTHREAD_H
