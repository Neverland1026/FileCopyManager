#include "MainView.h"
#include "ui_MainView.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QMimeData>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

MainView::MainView(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainView)
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
    QObject::connect(ui->pushButton_srcDirectory, &QPushButton::clicked, this, [&]() {
        ui->lineEdit_srcDirectory->setText(QFileDialog::getExistingDirectory(this,
                                                                             QObject::tr("选择原始文件夹"),
                                                                             QDir::tempPath()));
    });
    QObject::connect(ui->lineEdit_srcDirectory, &QLineEdit::textChanged, this, [&](const QString&) {
        toggleCopyAndCutEnabled();
    });

    // 目标路径
    QObject::connect(ui->pushButton_dstDirectory, &QPushButton::clicked, this, [&]() {
        ui->lineEdit_dstDirectory->setText(QFileDialog::getExistingDirectory(this,
                                                                             QObject::tr("选择目标文件夹"),
                                                                             QDir::tempPath()));
    });
    QObject::connect(ui->lineEdit_dstDirectory, &QLineEdit::textChanged, this, [&](const QString&) {
        toggleCopyAndCutEnabled();
    });

    // 搜索 txt
    QObject::connect(ui->pushButton_targetFile, &QPushButton::clicked, this, [&]() {
        ui->lineEdit_targetFile->setText(QFileDialog::getOpenFileName(this,
                                                                      QObject::tr("选择TXT文件"),
                                                                      QDir::tempPath(),
                                                                      "Txt File(*.TXT *.txt)"));
    });
    QObject::connect(ui->lineEdit_targetFile, &QLineEdit::textChanged, this, [&](const QString&) {
        toggleCopyAndCutEnabled();
    });

    // 复制
    QObject::connect(ui->pushButton_copy, &QPushButton::clicked, this, [&]() {
        startProcess(0);
    });

    // 移动
    QObject::connect(ui->pushButton_cut, &QPushButton::clicked, this, [&]() {
        startProcess(1);
    });

    // 进度条
    QObject::connect(m_copyThread, &CopyThread::sigStart, this, [&]() {
        ui->progressBar->setValue(0);
    });
    QObject::connect(m_copyThread, &CopyThread::sigStop, this, [&]() {
        ui->progressBar->setValue(100);
        if(!ui->textBrowser_copyFailed->toPlainText().isEmpty())
        {
            ui->tabWidget->setCurrentIndex(1);
            QMessageBox::information(this,
                                     QObject::tr("提示"),
                                     QObject::tr("以下文件拷贝失败！"));
        }
        else
        {
            QMessageBox::information(this,
                                     QObject::tr("提示"),
                                     QObject::tr("全部完成！"));
        }

        setMainViewEnabled(true);
    });
    QObject::connect(m_copyThread, &CopyThread::sigCopyFailedItem, this, [&](const QString& fileName) {
        ui->textBrowser_copyFailed->append(fileName);
    });
    QObject::connect(m_copyThread, &CopyThread::sigProgress, this, [&](int value) {
        ui->progressBar->setValue(value);
    });
    QObject::connect(m_copyThread, &CopyThread::sigCopyException, this, [&]() {
        ui->progressBar->setValue(100);
        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("拷贝异常！"));

        setMainViewEnabled(true);
    });
}

void MainView::toggleCopyAndCutEnabled()
{
    const bool enabled = (!ui->lineEdit_srcDirectory->text().isEmpty()
                          && !ui->lineEdit_dstDirectory->text().isEmpty()
                          && !ui->lineEdit_targetFile->text().isEmpty());
    ui->pushButton_copy->setEnabled(enabled);
    ui->pushButton_cut->setEnabled(enabled);
}

void MainView::setMainViewEnabled(bool enabled)
{
    ui->pushButton_srcDirectory->setEnabled(enabled);
    ui->pushButton_dstDirectory->setEnabled(enabled);
    ui->pushButton_targetFile->setEnabled(enabled);
    ui->pushButton_copy->setEnabled(enabled);
    ui->pushButton_cut->setEnabled(enabled);
}

void MainView::startProcess(int type)
{
    if(ui->lineEdit_srcDirectory->text() == ui->lineEdit_dstDirectory->text())
    {
        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("原始路径不能和目标路径一致！"));
        return;
    }

    if(ui->lineEdit_targetFile->text().isEmpty())
    {
        QMessageBox::information(this,
                                 QObject::tr("提示"),
                                 QObject::tr("文件表单不一致！"));
        return;
    }

    // 开始准备拷贝
    setMainViewEnabled(false);
    ui->textBrowser_copyFailed->clear();
    m_copyThread->setCopyInfo(ui->lineEdit_srcDirectory->text(),
                              ui->lineEdit_dstDirectory->text(),
                              ui->lineEdit_targetFile->text(),
                              type);

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
