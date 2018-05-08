#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QScrollBar>

#define TimerInterval 500
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
    ui->setupUi(this);
    init();

    onOpenFile();
}

MainWindow::~MainWindow()
{
    delete ui;
    m_workThread.quit();
    m_workThread.wait();
}

void MainWindow::onOpenFile()
{
    QString file = QFileDialog::getOpenFileName(this, "select file");
    if (file.isEmpty())
    {
        return;
    }

    ui->textEdit->clear();
    emit locateIndex(file);
    setWindowTitle("tail -f " + file);
}

void MainWindow::onScrollClicked()
{
    m_scroll = ui->ckBox_Scroll->isChecked();
    if (m_scroll)
    {
        QScrollBar *scrollbar = ui->textEdit->verticalScrollBar();
        if (scrollbar)
        {
            scrollbar->setSliderPosition(scrollbar->maximum());
        }
    }
}

void MainWindow::onCodecChanged(int index)
{
    emit codecChanged(index);
    ui->textEdit->clear();
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
    emit filterChanged(text);
    if (text.isEmpty())
    {
        ui->lEdit_Block->setEnabled(true);
    }
    else
    {
        ui->lEdit_Block->setEnabled(false);
        ui->textEdit->clear();
    }
}

void MainWindow::onBlockChanged(const QString &text)
{
    emit blockChanged(text);
    if (text.isEmpty())
    {
        ui->lEdit_Filter->setEnabled(true);
    }
    else
    {
        ui->lEdit_Filter->setEnabled(false);
        ui->textEdit->clear();
    }
}

void MainWindow::onDisplayText(QString text)
{
    ui->textEdit->textCursor().insertText(text);
    if (m_scroll)
    {
        QScrollBar *scrollbar = ui->textEdit->verticalScrollBar();
        if (scrollbar)
        {
            scrollbar->setSliderPosition(scrollbar->maximum());
        }
    }
}

void MainWindow::onError(QString error)
{
    ui->textEdit->textCursor().insertText(error);
}

void MainWindow::init()
{
    QMenu *menu = new QMenu("File");
    menu->addAction("OpenFile", this, SLOT(onOpenFile()), QKeySequence(Qt::CTRL + Qt::Key_O));
    menuBar()->addMenu(menu);

    ui->textEdit->setReadOnly(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::All, QPalette::Window, Qt::gray);
    palette.setColor(QPalette::All, QPalette::WindowText, Qt::black);
    this->setPalette(palette);

    palette = ui->textEdit->palette();
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

    //multi thread
    m_worker = new Worker;
    m_worker->moveToThread(&m_workThread);
    m_workThread.start();
    connect(this, SIGNAL(locateIndex(QString)), m_worker, SLOT(onLocateIndex(QString)));
    connect(m_worker, SIGNAL(errorOccured(QString)), this, SLOT(onError(QString)));
    connect(m_worker, SIGNAL(displayText(QString)), this, SLOT(onDisplayText(QString)));
    connect(this, SIGNAL(codecChanged(int)), m_worker, SLOT(onCodecChanged(int)));
    connect(this, SIGNAL(filterChanged(QString)), m_worker, SLOT(onFilterChanged(QString)));
    connect(this, SIGNAL(blockChanged(QString)), m_worker, SLOT(onBlockChanged(QString)));
}

void MainWindow::initCodec()
{
    for (int i=0; i < sizeof(gCodec)/sizeof(CodecDesc); i++)
    {
        ui->cBox_Code->addItem(gCodec[i].displayName);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->textEdit->viewport())
    {
        if (event->type() == QEvent::MouseButtonDblClick ||
                event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::ContextMenu)
        {
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

Worker::Worker()
{
    m_code = CodecUtf8;
    m_rowCountDisplay = 1000;
    m_timer = NULL;
    m_file = NULL;
}

Worker::~Worker()
{
    while (m_vecData.size() > 0)
    {
        char *lineData = m_vecData[0];
        delete [] lineData;
        m_vecData.removeFirst();
    }
    if (m_timer != NULL)
    {
        delete m_timer;
        m_timer = NULL;
    }
    if (m_file != NULL)
    {
        delete m_file;
        m_file = NULL;
    }
}

void Worker::onLocateIndex(QString fileName)
{
    if (m_timer != NULL && m_timer->isActive())
    {
        m_timer->stop();
    }
    while (m_vecData.size() > 0)
    {
        char *lineData = m_vecData[0];
        delete [] lineData;
        m_vecData.removeFirst();
    }

    if (m_file == NULL)
    {
        m_file = new QFile;
    }
    m_file->setFileName(fileName);
    m_file->open(QIODevice::ReadOnly);
    if (!m_file->isOpen())
    {
        emit errorOccured("Open File Failed!");
        return;
    }

    m_index = getIndexOfLine(10);
    m_file->close();

    if (m_index == -1)
    {
        return;
    }

    if (m_timer == NULL)
    {
        m_timer = new QTimer;
        connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    }
    m_timer->start(TimerInterval);
}

void Worker::onTimer()
{
    m_file->open(QIODevice::ReadOnly);
    if (!m_file->isOpen())
    {
        emit errorOccured("Open File Failed!");
        m_timer->stop();
        return;
    }
    qint64 size = m_file->size();
    if (m_index < size)
    {
        if (!m_file->seek(m_index))
        {
            emit errorOccured("Seek Failed!");
            m_timer->stop();
            return;
        }
        QString strDis;
        char buf[2048];
        memset(buf, 0, 2048);
        int lineLength = 0;
        while ((lineLength = m_file->readLine(buf, 2047)) > 0)
        {
            QString tmp = getCodecString(buf);
            filterLine(tmp);
            strDis += tmp;
            char *lineData = new char[lineLength + 1];
            memset(lineData, 0, lineLength + 1);
            memcpy(lineData, buf, lineLength);
            m_vecData.push_back(lineData);

            while (m_vecData.size() > m_rowCountDisplay)
            {
                char *lineData = m_vecData[0];
                delete [] lineData;
                m_vecData.removeFirst();
            }
            memset(buf, 0, 2048);
        }
        emit displayText(strDis);
        m_index = size;
    }
    m_file->close();
}

void Worker::onCodecChanged(int codec)
{
    m_code = CodecDisplay(codec);

    m_timer->stop();
    QString strDis;
    for (int i=0; i < m_vecData.size(); i++)
    {
        QString tmp = getCodecString(m_vecData.at(i));
        filterLine(tmp);
        strDis += tmp;
    }
    emit displayText(strDis);
    m_timer->start(TimerInterval);
}

void Worker::onFilterChanged(QString filter)
{
    m_timer->stop();
    m_filter = filter;
    QString strDis;
    for (int i=0; i < m_vecData.size(); i++)
    {
        QString tmp = getCodecString(m_vecData.at(i));
        filterLine(tmp);
        strDis += tmp;
    }
    emit displayText(strDis);
    m_timer->start(TimerInterval);
}

void Worker::onBlockChanged(QString block)
{
    m_timer->stop();
    m_block = block;
    QString strDis;
    for (int i=0; i < m_vecData.size(); i++)
    {
        QString tmp = getCodecString(m_vecData.at(i));
        filterLine(tmp);
        strDis += tmp;
    }
    emit displayText(strDis);
    m_timer->start(TimerInterval);
}

qint64 Worker::getIndexOfLine(int line)
{
    bool allDisplay = false;
    int bufLen = 1000;
    char buf[1000];
    memset(buf, 0, 1000);

    qint64 index = -1;
    qint64 size = m_file->size();
    if (size <= bufLen)
    {
        bufLen = size;
        allDisplay = true;
        index = 0;
    }
    else
    {
        index = size - bufLen;
    }

    if (!m_file->seek(index))
    {
        emit errorOccured("Seek Failed!");
        return -1;
    }
    int lineCount = 0;

    while (m_file->read(buf, bufLen)> 0)
    {
        for (int i = bufLen; i > 0; i --)
        {
            if (buf[i-1] == '\n')
            {
                lineCount ++;
                if (lineCount == line + 1)
                {
                    index = index + i;
                    allDisplay = true;
                    break;
                }
            }
        }
        if (!allDisplay)
        {
            if (index < bufLen)
            {
                bufLen = index;
                index = 0;
            }
            else
            {
                index -= bufLen;
            }
            if (!m_file->seek(index))
            {
                emit errorOccured("Seek Failed!");
                return -1;
            }
        }
        memset(buf, 0, 1000);
    }
    return index;
}

QString Worker::getCodecString(char *ch)
{
    switch (m_code) {
//    case CodecASCII:
//        return QString::fromUtf8(ch);
    case CodecGB18030:
        return QString::fromLocal8Bit(ch);
    case CodecUtf8:
        return QString::fromUtf8(ch);
    default:
        return QString::fromUtf8(ch);
        break;
    }
}

void Worker::filterLine(QString &lineData)
{
    if (!m_filter.isEmpty())
    {
        if (!lineData.contains(m_filter))
        {
            lineData.clear();
        }
    }
    if (!m_block.isEmpty())
    {
        if (lineData.contains(m_block))
        {
            lineData.clear();
        }
    }
}
