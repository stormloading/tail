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
    m_scroll = true;
    m_code = CodecUtf8;
    m_rowCountDisplay = 1000;
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
    while (m_vecData.size() > 0)
    {
        char *lineData = m_vecData[0];
        delete [] lineData;
        m_vecData.removeFirst();
    }
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
            QString tmp = getCodecString(buf);
            if (!m_filter.isEmpty())
            {
                if (tmp.contains(m_filter))
                {
                    strDis += tmp;
                }
            }
            else if (!m_block.isEmpty())
            {
                if (!tmp.contains(m_block))
                {
                    strDis += tmp;
                }
            }
            else
            {
                strDis += tmp;
            }
            ui->textEdit->textCursor().insertText(strDis);
            char *lineData = new char[lineLength + 1];
            memset(lineData, 0, lineLength + 1);
            memcpy(lineData, buf, lineLength);
            m_vecData.push_back(lineData);

            while (m_vecData.size() > m_rowCountDisplay)
            {
                qDebug()<<"del"<<m_vecData.at(0);
                char *lineData = m_vecData[0];
                delete [] lineData;
                m_vecData.removeFirst();
            }
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
    m_timer.stop();
    m_code = CodecDisplay(index);

    ui->textEdit->clear();
    QString strDis;
    for (int i=0; i < m_vecData.size(); i++)
    {
        QString tmp = getCodecString(m_vecData.at(i));
        if (!m_filter.isEmpty())
        {
            if (tmp.contains(m_filter))
            {
                strDis += tmp;
            }
        }
        else if (!m_block.isEmpty())
        {
            if (!tmp.contains(m_block))
            {
                strDis += tmp;
            }
        }
    }
    ui->textEdit->textCursor().insertText(strDis);
    m_timer.start(100);
}

void MainWindow::onFontChanged(const QFont &font)
{
    ui->textEdit->setCurrentFont(font);
    ui->textEdit->setFontPointSize(ui->cBox_PointSize->currentText().toInt());
}

void MainWindow::onFontSizeChanged(int)
{
    ui->textEdit->setFontPointSize(ui->cBox_PointSize->currentText().toInt());
}

void MainWindow::onFilterChanged(const QString &text)
{
    m_filter = text;
    if (text.isEmpty())
    {
        ui->lEdit_Block->setEnabled(true);
    }
    else
    {
        m_timer.stop();
        ui->lEdit_Block->setEnabled(false);
        ui->textEdit->clear();
        QString strDis;
        for (int i=0; i < m_vecData.size(); i++)
        {
            QString tmp = getCodecString(m_vecData.at(i));
            if (tmp.contains(text))
            {
                strDis += tmp;
            }
        }
        ui->textEdit->textCursor().insertText(strDis);
        m_timer.start(100);
    }
}

void MainWindow::onBlockChanged(const QString &text)
{
    m_block = text;
    if (text.isEmpty())
    {
        ui->lEdit_Filter->setEnabled(true);
    }
    else
    {
        m_timer.stop();
        ui->lEdit_Filter->setEnabled(false);
        ui->textEdit->clear();
        QString strDis;
        for (int i=0; i < m_vecData.size(); i++)
        {
            QString tmp = getCodecString(m_vecData.at(i));
            if (!tmp.contains(text))
            {
                strDis += tmp;
            }
        }
        ui->textEdit->textCursor().insertText(strDis);
        m_timer.start(100);
    }
}

void MainWindow::init()
{
    ui->textEdit->setReadOnly(true);
    QPalette palette = ui->textEdit->palette();
    palette.setColor(QPalette::All, QPalette::Base, Qt::black);
    palette.setColor(QPalette::All, QPalette::Text, Qt::green);
    ui->textEdit->setPalette(palette);
//    setWindowOpacity(0.7);
//    ui->textEdit->setWindowOpacity(0);
    QFont font(QString::fromLocal8Bit("微软雅黑"), 10);
    ui->textEdit->setCurrentFont(font);
    ui->cBox_Font->setCurrentFont(font);
    ui->cBox_PointSize->setCurrentText(QString::number(10));

    ui->textEdit->viewport()->installEventFilter(this);

    initCodec();
    connect(ui->ckBox_Scroll, SIGNAL(clicked(bool)), this, SLOT(onScrollClicked()));
    connect(ui->cBox_Code, SIGNAL(activated(int)), this, SLOT(onCodecChanged(int)));

    connect(ui->cBox_Font, SIGNAL(currentFontChanged(const QFont&)), this, SLOT(onFontChanged(const QFont&)));
    connect(ui->cBox_PointSize, SIGNAL(currentIndexChanged(int)), this, SLOT(onFontSizeChanged(int)));

    connect(ui->lEdit_Filter, SIGNAL(textChanged(QString)), this, SLOT(onFilterChanged(QString)));
    connect(ui->lEdit_Block, SIGNAL(textChanged(QString)), this, SLOT(onBlockChanged(QString)));
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
