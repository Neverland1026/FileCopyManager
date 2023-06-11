﻿#include "MainView.h"
#include "ui_MainView.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QDesktopServices>
#include <QMimeData>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

// 是否使用源路径
//#define USE_SRC_DIR

MainView::MainView(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainView)
    , m_lastCommandType(CommandType::CT_Copy)
    , m_copyThread(new CopyThread(this))
{
    ui->setupUi(this);

    this->setFixedSize(600, this->minimumHeight());

    init();
}

MainView::~MainView()
{
    delete ui;
}

void MainView::init()
{
    this->setWindowIcon(QIcon(":/images/logo.svg"));
    this->setAcceptDrops(true);
    this->setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);
    this->setAcceptDrops(true);

    QTimer::singleShot(0, this, [&]() {
        //::SetWindowPos((HWND)(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    });

// 源路径
#ifdef USE_SRC_DIR
    ui->pushButton_srcDirectory->setEnabled(true);
    QObject::connect(ui->pushButton_srcDirectory, &QPushButton::clicked, this, [&]() {
        ui->lineEdit_srcDirectory->setText(QFileDialog::getExistingDirectory(this,
                                                                             QObject::tr("选择原始文件夹"),
                                                                             QDir::tempPath()));
    });
    ui->lineEdit_srcDirectory->setEnabled(true);
    QObject::connect(ui->lineEdit_srcDirectory, &QLineEdit::textChanged, this, [&](const QString&) {
        toggleCopyAndCutEnabled();
    });
#endif

    // 目标路径
    QObject::connect(ui->pushButton_dstDirectory, &QPushButton::clicked, this, [&]() {
        static QString s_directory = "";
        s_directory = s_directory.isEmpty() ? QDir::tempPath() : s_directory;
        ui->lineEdit_dstDirectory->setText(QFileDialog::getExistingDirectory(this,
                                                                             QObject::tr("选择目标文件夹"),
                                                                             s_directory));
        if(false == ui->lineEdit_dstDirectory->text().isEmpty())
        {
            s_directory = ui->lineEdit_dstDirectory->text();
        }
    });
    QObject::connect(ui->lineEdit_dstDirectory, &QLineEdit::textChanged, this, [&](const QString&) {
        toggleCopyAndCutEnabled();
    });

    // 搜索 txt
    QObject::connect(ui->pushButton_targetFile, &QPushButton::clicked, this, [&]() {
        static QString s_directory = "";
        s_directory = s_directory.isEmpty() ? QDir::tempPath() : s_directory;
        ui->lineEdit_targetFile->setText(QFileDialog::getOpenFileName(this,
                                                                      QObject::tr("选择TXT文件"),
                                                                      s_directory,
                                                                      "Txt File(*.TXT *.txt)"));
        if(false == ui->lineEdit_targetFile->text().isEmpty())
        {
            s_directory = QFileInfo(ui->lineEdit_targetFile->text()).filePath();
        }
    });
    QObject::connect(ui->lineEdit_targetFile, &QLineEdit::textChanged, this, [&](const QString&) {
        toggleCopyAndCutEnabled();
    });

    // 复制
    QObject::connect(ui->pushButton_copy, &QPushButton::clicked, this, [&]() {
        startProcess(CommandType::CT_Copy);
    });

    // 移动
    QObject::connect(ui->pushButton_cut, &QPushButton::clicked, this, [&]() {
        startProcess(CommandType::CT_Move);
    });

    // 撤销
    QObject::connect(ui->pushButton_undo, &QPushButton::clicked, this, [&]() {
        ui->pushButton_undo->setEnabled(false);
        startProcess(CommandType::CT_Undo);
    });

    // 打开输出文件夹
    QObject::connect(ui->pushButton_outDirectory, &QPushButton::clicked, this, [&]() {
        if(QFileInfo::exists(m_outDirectory) && QFileInfo(m_outDirectory).isDir())
        {
            QDesktopServices::openUrl(QUrl("file:///" + m_outDirectory));
        }
    });

    // 进度条
    ui->progressBar->setAlignment(Qt::AlignCenter);
    ui->progressBar->setFormat(QString("%1 / %2").arg(0).arg(0));
    QObject::connect(m_copyThread, &CopyThread::sigStart, this, [&]() {
        ui->progressBar->reset();
    });
    QObject::connect(m_copyThread, &CopyThread::sigCopyException, this, [&]() {
        ui->progressBar->reset();
        setMainViewEnabled(true);
        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("拷贝异常！"));

    });
    QObject::connect(m_copyThread, &CopyThread::sigUpdateRange, this, [&](int valueMinimum, int valueMaximum) {
        ui->progressBar->setFormat(QString("%1 / %2").arg(valueMinimum).arg(valueMaximum));
        ui->progressBar->setRange(valueMinimum, valueMaximum);
    });
    QObject::connect(m_copyThread, &CopyThread::sigProgress, this, [&](int index, int total) {
        ui->progressBar->setFormat(QString("%1 / %2").arg(index).arg(total));
        ui->progressBar->setValue(index);
    });
    QObject::connect(m_copyThread, &CopyThread::sigGenerateCSV, this, [&]() {
        ui->progressBar->setValue(-2);
    });
    QObject::connect(m_copyThread, &CopyThread::sigStop, this, [&](const QString& outputDir,
                                                                   const int succeed,
                                                                   const int srcNotExist,
                                                                   const int dstAlreadyExist,
                                                                   const int exception) {
        m_outDirectory = outputDir;

        ui->progressBar->setValue(ui->progressBar->maximum());
        ui->progressBar->setFormat(QObject::tr("%1 / %2 (成功：%3  失败：%4)")
                                       .arg(ui->progressBar->maximum())
                                       .arg(ui->progressBar->maximum())
                                       .arg(succeed)
                                       .arg(srcNotExist + dstAlreadyExist + exception));

        setMainViewEnabled(true);

        ui->pushButton_outDirectory->setEnabled(true);

        if(CommandType::CT_Undo != m_lastCommandType)
        {
            ui->pushButton_undo->setEnabled(succeed > 0);
        }

        if(ui->checkBox_openResultDir->isChecked())
        {
            ui->pushButton_outDirectory->clicked();
        }

        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("全部完成！"));

    });
}

void MainView::toggleCopyAndCutEnabled()
{
    bool enabled = (!ui->lineEdit_dstDirectory->text().isEmpty()
                    && !ui->lineEdit_targetFile->text().isEmpty());
#ifdef USE_SRC_DIR
    enabled = enabled && (!ui->lineEdit_srcDirectory->text().isEmpty());
#endif
    ui->pushButton_copy->setEnabled(enabled);
    ui->pushButton_cut->setEnabled(enabled);
}

void MainView::setMainViewEnabled(bool enabled)
{
#ifdef USE_SRC_DIR
    ui->pushButton_srcDirectory->setEnabled(enabled);
#endif
    ui->pushButton_dstDirectory->setEnabled(enabled);
    ui->pushButton_targetFile->setEnabled(enabled);
    ui->pushButton_copy->setEnabled(enabled);
    ui->pushButton_cut->setEnabled(enabled);
}

void MainView::startProcess(CommandType type)
{
#ifdef USE_SRC_DIR
    if(ui->lineEdit_srcDirectory->text() == ui->lineEdit_dstDirectory->text())
    {
        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("原始路径不能和目标路径一致！"));
        return;
    }
#endif

    if(false == QFileInfo::exists(ui->lineEdit_targetFile->text()))
    {
        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("文件表单不一致！"));
        return;
    }

    // 开始准备拷贝
    setMainViewEnabled(false);
    ui->textBrowser_copyFailed->clear();

    // 分情况讨论
    if(CommandType::CT_Copy == type || CommandType::CT_Move == type)
    {
        m_copyThread->setCopyInfo(ui->lineEdit_srcDirectory->text(),
                                  ui->lineEdit_dstDirectory->text(),
                                  ui->lineEdit_targetFile->text(),
                                  (CommandType::CT_Copy == type ? 0 : 1));
    }
    else if(CommandType::CT_Undo == type)
    {
        m_copyThread->setCopyInfo(ui->lineEdit_srcDirectory->text(),
                                  ui->lineEdit_dstDirectory->text(),
                                  ui->lineEdit_targetFile->text(),
                                  (CommandType::CT_Copy == m_lastCommandType ? 2 : 3));
    }

    m_lastCommandType = type;

    m_copyThread->start();
}

void MainView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void MainView::dropEvent(QDropEvent* event)
{
    // 获取MIME数据
    const QMimeData *mimeData = event->mimeData();

    // 如果数据中包含 URL
    if(mimeData->hasUrls())
    {
        // 获取URL列表
        QList<QUrl> urlList = mimeData->urls();

        // 将其中的第一个 URL 表示为本地文件路径，toLocalFile() 转换为本地文件路径
        QString fileName = urlList.at(0).toLocalFile();
        QFileInfo fi(fileName);
        if(fi.exists())
        {
            QLineEdit* child = static_cast<QLineEdit*>(childAt(event->pos()));
            if(Q_NULLPTR == child || !child->inherits("QLineEdit"))
            {
                return;
            }

            child->setText(fileName);
        }
    }
}

void MainView::keyPressEvent(QKeyEvent* event)
{
    switch(event->key())
    {
    case Qt::Key_Escape:
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MainView::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton button = QMessageBox::information(this,
                                                                  QObject::tr("提示"),
                                                                  QObject::tr("确定要退出吗？"),
                                                                  QMessageBox::StandardButton::Ok,
                                                                  QMessageBox::StandardButton::Cancel);
    if(button == QMessageBox::StandardButton::Ok)
    {
        m_copyThread->quit();
        m_copyThread->wait();

        event->accept();
    }
    else
    {
        event->ignore();
    }
}
