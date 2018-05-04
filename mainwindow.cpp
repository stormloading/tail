#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QScrollBar>

CodecDesc gCodec[] =
{
    CodecUtf8, "Utf-8",
    CodecGB18030, "GB18030",
//    CodecASCII, "ASCII"
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_scroll = false;
    m_code = CodecUtf8;

    ui->setupUi(this);
    init();

    QString file = QFileDialog::getOpenFileName(this, "select file");
    if (file.isEmpty())
    {
        return;
    }
    m_file.setFileName(file);
    m_file.open(QIODevice::ReadOnly);
    if (!m_file.isOpen())
    {
        return;
    }

    getIndexByRead(10);

    m_file.seek(m_index);
    QString strDis;
    char buf[2048];
    memset(buf, 0, 2047);
    while (m_file.read(buf, 2047) > 0)
    {
        strDis += getCodecString(buf);
        memset(buf, 0, 2048);
    }
    ui->textEdit->textCursor().insertText(strDis);
    m_index = m_file.size();
    m_file.close();

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    m_timer.start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}
#include <QDebug>
void MainWindow::onTimer()
{
    m_file.open(QIODevice::ReadOnly);
    if (!m_file.isOpen())
    {
        return;
    }
    qint64 size = m_file.size();
    if (m_index < size)
    {
        m_file.seek(m_index);
        QString strDis;
        char buf[2048];
        memset(buf, 0, 2048);
        int lineLength = 0;
        while ((lineLength = m_file.readLine(buf, 2047)) > 0)
        {
            qDebug()<<"len"<<lineLength;
            strDis += getCodecString(buf);
            memset(buf, 0, 2048);
        }
        ui->textEdit->textCursor().insertText(strDis);
        m_index = size;
    }
    m_file.close();
    if (m_scroll)
    {
        QScrollBar *scrollbar = ui->textEdit->verticalScrollBar();
        if (scrollbar)
        {
            scrollbar->setSliderPosition(scrollbar->maximum());
        }
    }
}

void MainWindow::onScrollClicked()
{
    m_scroll = ui->ckBox_Scroll->isChecked();
}

void MainWindow::onCodecChanged(int index)
{
    m_code = CodecDisplay(index);
}

void MainWindow::init()
{
    QPalette palette = ui->textEdit->palette();
    palette.setColor(QPalette::All, QPalette::Base, Qt::black);
    palette.setColor(QPalette::All, QPalette::Text, Qt::green);
    ui->textEdit->setPalette(palette);
//    setWindowOpacity(0.7);
    ui->textEdit->setFontFamily(QString::fromLocal8Bit("微软雅黑"));

    ui->textEdit->viewport()->installEventFilter(this);

    initCodec();
    connect(ui->ckBox_Scroll, SIGNAL(clicked(bool)), this, SLOT(onScrollClicked()));
    connect(ui->cBox_Code, SIGNAL(activated(int)), this, SLOT(onCodecChanged(int)));

    ui->textEdit->setReadOnly(true);
}

void MainWindow::initCodec()
{
    for (int i=0; i < sizeof(gCodec)/sizeof(CodecDesc); i++)
    {
        ui->cBox_Code->addItem(gCodec[i].displayName);
    }
}

QString MainWindow::getCodecString(char *ch)
{
    switch (m_code) {
//    case CodecASCII:
//        return QString::fromUtf8(ch);
    case CodecGB18030:
        return QString::fromLocal8Bit(ch);
    case CodecUtf8:
        return QString::fromUtf8(ch);
    default:
        break;
    }

}

qint64 MainWindow::getIndexByRead(int rowCount)
{
    bool allDisplay = false;
    int bufLen = 1000;
    char buf[1000];
    memset(buf, 0, 1000);

    qint64 size = m_file.size();
    if (size <= bufLen)
    {
        bufLen = size;
        allDisplay = true;
        m_index = 0;
    }
    else
    {
        m_index = size - bufLen;
    }

    if (!m_file.seek(m_index))
    {
        ui->textEdit->textCursor().insertText(QString("seek failed!"));
        return -1;
    }
    int lineCount = 0;

    while (m_file.read(buf, bufLen)> 0)
    {
        for (int i = bufLen; i > 0; i --)
        {
            if (buf[i-1] == '\n')
            {
                lineCount ++;
                if (lineCount == rowCount + 1)
                {
                    m_index = m_index + i;
                    allDisplay = true;
                    break;
                }
            }
        }
        if (!allDisplay)
        {
            if (m_index < bufLen)
            {
                bufLen = m_index;
                m_index = 0;
            }
            else
            {
                m_index -= bufLen;
            }
            if (!m_file.seek(m_index))
            {
                ui->textEdit->textCursor().insertText(QString("seek failed!"));
                return -1;
            }
        }
        memset(buf, 0, 1000);
    }
    return m_index;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->textEdit->viewport())
    {
        if (event->type() == QEvent::MouseButtonDblClick ||
                event->type() == QEvent::MouseButtonPress)
        {
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
