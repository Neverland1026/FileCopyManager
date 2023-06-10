#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include <QObject>
#include <QThread>
#include <QVector>
#include <QPair>
#include <set>

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

    // 实时进度
    void sigProgress(int value);

    // 开始写出到本地
    void sigGenerateCSV();

    // 拷贝异常
    void sigCopyException();

protected:

    // 核心函数
    void run() override;

    // 写文件
    void write(const std::string& fileSuffix = ".csv");

private:

    // 源文件夹、目标文件夹、文件集合
    std::tuple<QString, QString, QString, int> m_copyInfo;

    enum class ProcessType {
        PT_Succeed,
        PT_SrcNotExist,
        PT_DstAlreadyExist,
        PT_Exception,
    };

    // 记录每项的拷贝状态
    QVector<QPair<QString, ProcessType>> m_processHistory;
    std::set<ProcessType> m_processTypeSet;

};

#endif // COPYTHREAD_H
