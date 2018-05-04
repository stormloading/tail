#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFile>
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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void onTimer();
    void onScrollClicked();
    void onCodecChanged(int index);

    void onFontChanged(const QFont &font);
    void onFontSizeChanged(int);

    void onFilterChanged(const QString &text);
    void onBlockChanged(const QString &text);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    void init();
    void initCodec();
    QString getCodecString(char* ch);
    qint64 getIndexByRead(int rowCount);

private:
    Ui::MainWindow *ui;
    QTimer m_timer;
    QFile m_file;
    qint64 m_index;
    bool m_scroll;
    int m_rowCountDisplay;
    CodecDisplay m_code;
    QVector<char *> m_vecData;
    QString m_filter;
    QString m_block;
};

#endif // MAINWINDOW_H
