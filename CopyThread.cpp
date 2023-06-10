#include "CopyThread.h"
#include <QFile>
#include <QStringList>
#include <QFileInfo>
#include <QDir>

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

            // 去掉空字符串，去掉字符串最后的 \r
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
            const QString& srcDir = std::get<0>(m_copyInfo);
            const QString& dstDir = std::get<1>(m_copyInfo);
            const int& type = std::get<3>(m_copyInfo);

            // 开始拷贝
            for(size_t i = 0; i < fileList.size(); ++i)
            {
                // 源文件绝对路径
                QString srcFile = QDir::fromNativeSeparators(fileList[i]);
                if(!QFileInfo(srcFile).isFile() || !QFileInfo::exists(srcFile))
                {
                    srcFile = srcDir + "/" + srcFile;
                }

                // 目标文件绝对路径
                QString dstFile = dstDir + "/" + QFileInfo(srcFile).fileName();

                // 按类型操作
                if(0 == type)
                {
                    if(false == QFile::copy(srcFile, dstFile))
                    {
                        emit sigCopyFailedItem(srcFile);
                    }
                }
                else
                {
                    if(false == QFile::rename(srcFile, dstFile))
                    {
                        emit sigCopyFailedItem(srcFile);
                    }
                }

                emit sigProgress((float)(i + 1) / fileList.size() * 100);

                QThread::sleep(1);
            }

            // 关闭文件
            file.close();
        }
    }
    else
    {
        emit sigCopyException();

        return;
    }

    emit sigStop();
}
