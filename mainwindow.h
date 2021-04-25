#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QStringListModel>
#include <QProcess>

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
  /* Events */
  void on_closeButton_clicked();

  void on_toTrayButton_clicked();

  void on_runButton_clicked();

  void on_importFromGifButton_clicked();

  void on_importBmpButton_clicked();

  void on_stopButton_clicked();

  void on_clickSystemTrayIcon(QSystemTrayIcon::ActivationReason);

  void on_wallpapersListView_pressed(const QModelIndex &index);

  void on_wallpapersListView_doubleClicked();

  void on_speedSlider_valueChanged(int value);

  /* Actions */

  void on_actionImport_GIF_triggered();

  void on_actionImport_BMP_triggered();

  void on_actionSettings_triggered();

  void on_actionHide_to_tray_triggered();

  void on_actionQuit_triggered();

  /* Procedures */

  void CreateSystemTrayIcon();

  void ImportBMP();

  void ImportGIF();

  void InitWallpapersList();

  void UpdateWallpaperListView();

  void RunConvertProcess(QString gifPath, QString dest);

  void RunGifWallpaperProcess(QString pathToWallpaper);

  void CheckForDependencies();

private:
  Ui::MainWindow *ui;
  QProcess *gif_wallpaper = nullptr;
  QStringListModel *wallpapersListModel;
  QString wallpapersDirectory;
  QString selectedWallpaperPath;
  QString gif_wallpaperCmd = "gw.exe";
  QString magickCmd = "magick.exe";
  int16_t minDelay = 10, delayBetweenFrames = 60, maxDelay = 120;
};
#endif // MAINWINDOW_H
