#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFile>
#include <QThread>
#include <QTextCursor>
#include <QTextEdit>
namespace Ui {
class MainWindow;
}

enum CodecDisplay
{
    CodecUtf8,
    CodecGB18030,
    CodecASCII
};

struct CodecDesc
{
    CodecDisplay codec;
    QString displayName;
};

class Worker;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void locateIndex(QString fileName);
    void codecChanged(int codec);

    void filterChanged(QString filter);
    void blockChanged(QString block);

public slots:
    void onOpenFile();

    void onScrollClicked();
    void onCodecChanged(int index);

    void onFontChanged(const QFont &font);
    void onFontSizeChanged(int);

    void onFilterChanged(const QString &text);
    void onBlockChanged(const QString &text);

    void onFullScreenClicked(bool checked);

    void onAutoLineFeed(bool checked);

    //thread slots
    void onDisplayText(QString text);
    void onError(QString error);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    void init();
    void initCodec();

private:
    Ui::MainWindow *ui;
    bool m_scroll;
    QString m_filter;
    QThread m_workThread;
    Worker *m_worker;
    bool m_clear;

    QString m_lastFileName;
    QTextCharFormat m_format;
};

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker(QTextEdit *edit);
    ~Worker();
signals:
    void errorOccured(QString error);
    void displayText(QString text);

public slots:
    void onLocateIndex(QString fileName);
    void onTimer();

    void onCodecChanged(int codec);
    void onFilterChanged(QString filter);
    void onBlockChanged(QString block);

protected:
    qint64 getIndexOfLine(int line);
    QString getCodecString(char* ch);
    void filterLine(QString &lineData);

private:
    QFile *m_file;
    QTimer *m_timer;
    qint64 m_index;
    CodecDisplay m_code;
    int m_rowCountDisplay;

    QString m_filter;
    QStringList m_block;
    QVector<char *> m_vecData;
    QTextEdit *m_editor;
};

#endif // MAINWINDOW_H
