#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_closeButton_clicked();

  void on_toTrayButton_clicked();

  void on_runButton_clicked();

  void on_actionQuit_triggered();

  void CreateSystemTrayIcon();

  void on_actionHide_to_tray_triggered();

private:
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
