#include "CopyThread.h"
#include <QFile>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <fstream>

#include "omp.h"

// 是否使用源路径
//#define USE_SRC_DIR

CopyThread::CopyThread(QObject *parent /*= nullptr*/)
    : QThread(parent)
    , m_processHistory({})
    , m_processCount({})
    , m_skipFile({})
    , m_outputDir("")
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
    m_srcDir = srcDir;
    m_dstDir = dstDir;
    m_copyInfo = targetFile;
    switch (type) {
    case 0: m_commandType = CommandType::CT_CopySrc2Dst; break;
    case 1: m_commandType = CommandType::CT_MoveSrc2Dst; break;
    case 2: m_commandType = CommandType::CT_DeleteDst; break;
    case 3: m_commandType = CommandType::CT_MoveDst2Src; break;
    default: m_commandType = CommandType::CT_CopySrc2Dst; break;
    }
}

void CopyThread::run()
{
    emit sigStart();

    m_processHistory.resize(0);
    m_processCount.clear();
    m_processCount[OperateResultType::ORT_Succeed] = 0;
    m_processCount[OperateResultType::ORT_SrcNotExist] = 0;
    m_processCount[OperateResultType::ORT_DstAlreadyExist] = 0;
    m_processCount[OperateResultType::ORT_Exception] = 0;
    m_outputDir.clear();

    // 解析要拷贝的文件名
    QFile file(m_copyInfo);
    if(file.open(QIODevice::ReadOnly))
    {
        uchar* fPtr = file.map(0, file.size());
        if(fPtr)
        {
            // 拷贝、剪切才清空
            if(CommandType::CT_CopySrc2Dst == m_commandType || CommandType::CT_MoveSrc2Dst == m_commandType)
            {
                m_skipFile.clear();
            }

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

            emit sigUpdateRange(0, fileList.size());

            // 开始拷贝
            for(int i = 0; i < fileList.size(); ++i)
            {
                const QString& targetFile = fileList[i];

                do {
                    // 源文件绝对路径
                    QString srcFile = QDir::fromNativeSeparators(targetFile);

#ifdef USE_SRC_DIR
                    srcFile = m_srcDir + "/" + srcFile;
#endif

                    // 目标文件绝对路径
                    QString dstFile = m_dstDir + "/" + QFileInfo(srcFile).fileName();

                    // 预判断状态
                    if(CommandType::CT_CopySrc2Dst == m_commandType || CommandType::CT_MoveSrc2Dst == m_commandType)
                    {
                        if(false == QFileInfo::exists(srcFile))
                        {
                            m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::ORT_SrcNotExist));
                            ++m_processCount[OperateResultType::ORT_SrcNotExist];
                            break;
                        }

                        if(true == QFileInfo::exists(dstFile))
                        {
                            m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::ORT_DstAlreadyExist));
                            ++m_processCount[OperateResultType::ORT_DstAlreadyExist];

                            // 已经存在的文件，需要跳过
                            m_skipFile.insert(dstFile);

                            break;
                        }
                    }
                    else if(CommandType::CT_DeleteDst == m_commandType)
                    {
                        // 不存在失败
                        if((false == QFileInfo::exists(dstFile))
                            || (m_skipFile.contains(dstFile)))
                        {
                            m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::/*ORT_Exception*/ORT_Succeed));
                            ++m_processCount[OperateResultType::/*ORT_Exception*/ORT_Succeed];
                            break;
                        }
                    }
                    else if(CommandType::CT_MoveDst2Src == m_commandType)
                    {
                        // 不存在、或者上一次跳过的文件，不参与撤销，并且应当视为成功
                        if((false == QFileInfo::exists(dstFile))
                            || (true == QFileInfo::exists(srcFile))
                            || (m_skipFile.contains(dstFile)))
                        {
                            m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::/*ORT_SrcNotExist*/ORT_Succeed));
                            ++m_processCount[OperateResultType::/*ORT_SrcNotExist*/ORT_Succeed];
                            break;
                        }
                    }

                    // 按类型操作
                    bool ret = false;
                    switch (m_commandType) {
                    case CommandType::CT_CopySrc2Dst:
                        ret = QFile::copy(srcFile, dstFile);
                        break;
                    case CommandType::CT_MoveSrc2Dst:
                        ret = QFile::rename(srcFile, dstFile);
                        break;
                    case CommandType::CT_DeleteDst:
                        QFile::setPermissions(dstFile, QFile::ReadOther | QFile::WriteOther);
                        ret = QFile::remove(dstFile);
                        break;
                    case CommandType::CT_MoveDst2Src:
                        ret = QFile::rename(dstFile, srcFile);
                        break;
                    default:
                        ret = QFile::copy(srcFile, dstFile);
                        break;
                    }

                    if(ret)
                    {
                        m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::ORT_Succeed));
                        ++m_processCount[OperateResultType::ORT_Succeed];
                    }
                    else
                    {
                        if(CommandType::CT_CopySrc2Dst == m_commandType || CommandType::CT_MoveSrc2Dst == m_commandType)
                        {
                            m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::ORT_Exception));
                            ++m_processCount[OperateResultType::ORT_Exception];
                        }
                        else
                        {
                            // 撤销不存在失败
                            m_processHistory.push_back(std::make_pair(targetFile, OperateResultType::/*ORT_SrcNotExist*/ORT_Succeed));
                            ++m_processCount[OperateResultType::/*ORT_SrcNotExist*/ORT_Succeed];
                        }
                    }

                } while (0);

                // 刷新进度
                emit sigProgress(i + 1, fileList.size());
            }

            // 关闭文件
            file.close();

            // 关闭缓冲
            file.unmap(fPtr);

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

    emit sigStop(m_outputDir,
                 m_processCount[OperateResultType::ORT_Succeed],
                 m_processCount[OperateResultType::ORT_SrcNotExist],
                 m_processCount[OperateResultType::ORT_DstAlreadyExist],
                 m_processCount[OperateResultType::ORT_Exception]);
}

void CopyThread::write(const std::string& fileSuffix /*= ".csv"*/)
{
    // 获取当前时间，构成后缀
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    QString suffix = date.toString("yyyy_MM_dd") + "_" + time.toString("hh_mm_ss");

    // 创建根文件夹
    m_outputDir = QCoreApplication::applicationDirPath() + "/result";
    QDir dir;
    dir.mkpath(m_outputDir);

    // 创建子文件夹
    m_outputDir = m_outputDir + "/" + suffix;
    dir = QDir(m_outputDir);
    dir.mkpath(m_outputDir);

    // 定义打印
    auto out = [&](const std::string& fileName, const OperateResultType type)
    {
        if(m_processCount[type] > 0)
        {
            std::ofstream ofs("./result/" + suffix.toStdString() + "/" + fileName + fileSuffix);
            for(const auto& h : m_processHistory)
            {
                if(type == h.second)
                {
                    ofs << h.first.toLocal8Bit().toStdString() << "\n";
                }
            }
            ofs.close();
        }
    };

    // 写文件
    out("Succeed", OperateResultType::ORT_Succeed);
    out("SrcNotExist", OperateResultType::ORT_SrcNotExist);
    out("DstAlreadyExist", OperateResultType::ORT_DstAlreadyExist);
    out("Exception", OperateResultType::ORT_Exception);
}
