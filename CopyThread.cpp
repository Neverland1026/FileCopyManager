#include "CopyThread.h"
#include <QFile>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <fstream>

// 是否使用源路径
//#define USE_SRC_DIR

CopyThread::CopyThread(QObject *parent /*= nullptr*/)
    : QThread(parent)
{

}

CopyThread::~CopyThread()
{

}

void CopyThread::setCopyInfo(const QString& srcDir,
                             const QString& dstDir,
                             const QString& targetFile,
                             const int type)
{
    std::get<0>(m_copyInfo) = srcDir;
    std::get<1>(m_copyInfo) = dstDir;
    std::get<2>(m_copyInfo) = targetFile;
    std::get<3>(m_copyInfo) = type;
}

void CopyThread::run()
{
    emit sigStart();

    m_processHistory.resize(0);
    m_processTypeSet.clear();

    // 解析要拷贝的文件名
    QFile file(std::get<2>(m_copyInfo));
    if(file.open(QIODevice::ReadOnly))
    {
        uchar* fPtr = file.map(0, file.size());
        if(fPtr)
        {
            // 转成 QString
            std::string str = (char*)fPtr;
            QString fileContent = QString::fromStdString(str);

            // 分割字段
            QStringList fileList = fileContent.split("\n");

            // 去掉空字符串，去掉字符串最后的 '\r'
            for(int i = fileList.size() - 1; i >= 0; --i)
            {
                if(fileList[i].isEmpty())
                {
                    fileList.removeAt(i);
                }
                else
                {
                    int index = fileList[i].lastIndexOf("\r");
                    if(index >= 0)
                    {
                        fileList[i] = fileList[i].mid(0, index);
                    }
                }
            }

            if(fileList.isEmpty())
            {
                emit sigCopyException();

                return;
            }

// 拿到引用
#ifdef USE_SRC_DIR
            const QString& srcDir = std::get<0>(m_copyInfo);
#endif
            const QString& dstDir = std::get<1>(m_copyInfo);
            const int& type = std::get<3>(m_copyInfo);

            // 开始拷贝
            for(int i = 0; i < fileList.size(); ++i)
            {
                do {
                    // 源文件绝对路径
                    QString srcFile = QDir::fromNativeSeparators(fileList[i]);
#ifdef USE_SRC_DIR

                    srcFile = srcDir + "/" + srcFile;
#endif
                    if(false == QFileInfo::exists(srcFile))
                    {
                        m_processHistory.push_back(std::make_pair(fileList[i], ProcessType::PT_SrcNotExist));
                        m_processTypeSet.insert(ProcessType::PT_SrcNotExist);
                        break;
                    }

                    // 目标文件绝对路径
                    QString dstFile = dstDir + "/" + QFileInfo(srcFile).fileName();
                    if(true == QFileInfo::exists(dstFile))
                    {
                        m_processHistory.push_back(std::make_pair(fileList[i], ProcessType::PT_DstAlreadyExist));
                        m_processTypeSet.insert(ProcessType::PT_DstAlreadyExist);
                        break;
                    }

                    // 按类型操作
                    bool ret = ((0 == type) ? QFile::copy(srcFile, dstFile) : QFile::rename(srcFile, dstFile));
                    if(ret)
                    {
                        m_processHistory.push_back(std::make_pair(fileList[i], ProcessType::PT_Succeed));
                        m_processTypeSet.insert(ProcessType::PT_Succeed);
                    }
                    else
                    {
                        m_processHistory.push_back(std::make_pair(fileList[i], ProcessType::PT_Exception));
                        m_processTypeSet.insert(ProcessType::PT_Exception);
                    }

                } while (0);

                // 刷新进度
                emit sigProgress((float)(i + 1) / fileList.size() * 100);
            }

            // 关闭文件
            file.close();

            // 将文件输出到本地
            emit sigGenerateCSV();
            write();
        }
    }
    else
    {
        emit sigCopyException();

        return;
    }

    emit sigStop();
}

void CopyThread::write(const std::string& fileSuffix /*= ".csv"*/)
{
    // 获取当前时间，构成后缀
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    QString suffix = "_" + date.toString("yyyy_MM_dd") + "_" + time.toString("hh_mm_ss");

    // 创建文件夹
    QString targetDir = "./result" + suffix;
    QDir dir;
    dir.mkpath(targetDir);

    // 开始写文件
    if(m_processTypeSet.count(ProcessType::PT_Succeed) > 0)
    {
        std::ofstream ofs_Succeed(targetDir.toStdString() + "/Succeed" + fileSuffix);
        for(const auto& h : m_processHistory)
        {
            if(ProcessType::PT_Succeed == h.second) ofs_Succeed << h.first.toLocal8Bit().toStdString() << "\n";
        }
        ofs_Succeed.close();
    }
    if(m_processTypeSet.count(ProcessType::PT_SrcNotExist) > 0)
    {
        std::ofstream ofs_SrcNotExist(targetDir.toStdString() + "/SrcNotExist" + fileSuffix);
        for(const auto& h : m_processHistory)
        {
            if(ProcessType::PT_SrcNotExist == h.second) ofs_SrcNotExist << h.first.toStdString() << "\n";
        }
        ofs_SrcNotExist.close();
    }
    if(m_processTypeSet.count(ProcessType::PT_DstAlreadyExist) > 0)
    {
        std::ofstream ofs_DstAlreadyExist(targetDir.toStdString() + "/DstAlreadyExist" = fileSuffix);
        for(const auto& h : m_processHistory)
        {
            if(ProcessType::PT_DstAlreadyExist == h.second) ofs_DstAlreadyExist << h.first.toStdString() << "\n";
        }
        ofs_DstAlreadyExist.close();
    }
    if(m_processTypeSet.count(ProcessType::PT_Exception) > 0)
    {
        std::ofstream ofs_Exception(targetDir.toStdString() + "/Exception" + fileSuffix);
        for(const auto& h : m_processHistory)
        {
            if(ProcessType::PT_Exception == h.second) ofs_Exception << h.first.toStdString() << "\n";
        }
        ofs_Exception.close();
    }
}
