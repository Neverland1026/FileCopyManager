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

    // 拷贝异常
    void sigCopyException();

    // 更新进度区间
    void sigUpdateRange(int valueMinimum, int valueMaximum);

    // 实时进度
    void sigProgress(int index, int total);

    // 开始写出到本地
    void sigGenerateCSV();

    // 结束
    void sigStop(const QString& outputDir,
                 const int succeed,
                 const int srcNotExist,
                 const int dstAlreadyExist,
                 const int exception);

protected:

    // 核心函数
    void run() override;

    // 写文件
    void write(const std::string& fileSuffix = ".txt");

private:

    // 源文件夹、目标文件夹、文件集合
    std::tuple<QString, QString, QString, int> m_copyInfo;

    // 拷贝结果枚举
    enum class ProcessType {
        PT_Succeed,
        PT_SrcNotExist,
        PT_DstAlreadyExist,
        PT_Exception,
    };

    // 记录每项的拷贝状态
    QVector<QPair<QString, ProcessType>> m_processHistory;
    std::map<ProcessType, int> m_processCount;

    // 成果输出文件夹
    QString m_outputDir;

};

#endif // COPYTHREAD_H
