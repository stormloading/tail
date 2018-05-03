#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFile>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void onTimer();

private:
    Ui::MainWindow *ui;
    QTimer m_timer;
    QFile m_file;
    qint64 m_index;
};

#endif // MAINWINDOW_H
