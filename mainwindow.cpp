#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_file.setFileName("test");
    m_file.open(QIODevice::ReadOnly);
    if (!m_file.isOpen())
    {
        return;
    }

    char buf[100];
    memset(buf, 0, 100);
    int bufLen = 100;
    qint64 size = m_file.size();
    m_index = size - bufLen;
    if (!m_file.seek(m_index))
    {
        return;
    }
    int lineCount = 0;

    while (lineCount < 10 && m_file.read(buf, bufLen)> 0)
    {
        bool done = false;
        for (int i = 100; i > 0; i --)
        {
            if (buf[i-1] == '\n')
            {
                lineCount ++;
                if (lineCount == 10)
                {
                    m_index = m_index + i +1;
                    done = true;
                    break;
                }
            }
        }
        if (!done)
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
                return;
            }
        }
        memset(buf, 0, 100);
    }

    m_file.seek(m_index);
    QString strDis;
    memset(buf, 0, 100);
    while (m_file.read(buf, 99) > 0)
    {
        strDis += buf;
        memset(buf, 0, 100);
    }
    ui->textEdit->textCursor().insertText(strDis);
    m_index = size;
    m_file.close();

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    m_timer.start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

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
        char buf[100];
        memset(buf, 0, 100);
        while (m_file.read(buf, 99) > 0)
        {
            strDis += buf;
            memset(buf, 0, 100);
        }
        ui->textEdit->textCursor().insertText(strDis);
        m_index = size;
    }
    m_file.close();
}
