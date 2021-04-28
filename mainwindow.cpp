#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QStringLiteral>
#include <QAction>
#include <QException>
#include <QProcess>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMenu>
#include <QString>
#include <QStandardPaths>
#include <QFile>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->delaySlider->setRange(minDelay, maxDelay);
  ui->delaySlider->setSliderPosition(delayBetweenFrames);
  ui->delayEdit->setText(QString::number(delayBetweenFrames));
  CreateSystemTrayIcon();
  InitWallpapersList();
}

MainWindow::~MainWindow()
{
  StopGifWallpaperProcess();
  delete gif_wallpaper;
  delete wallpapersListModel;
  delete wallListIterator;
  delete ui;
}

/* Events */

void MainWindow::on_clickSystemTrayIcon(QSystemTrayIcon::ActivationReason r)
{
  if (r == QSystemTrayIcon::Trigger)
  {
    if (!this->isVisible())
      this->show();
    else this->hide();
  }
}

void MainWindow::on_closeButton_clicked()
{
  QApplication::exit(0);
}

void MainWindow::on_toTrayButton_clicked()
{
  this->hide();
}

void MainWindow::on_importBmpButton_clicked()
{
  ImportBMP();
}

void MainWindow::on_importFromGifButton_clicked()
{
  ImportGIF();
}

void MainWindow::on_runButton_clicked()
{
  if (selectedWallpaperPath.isEmpty()) {
    QMessageBox msg(this);
    msg.setText(tr("Select a wallpaper"));
    msg.exec();
  }
  else {
    RunGifWallpaperProcess(selectedWallpaperPath);
  }
}

void MainWindow::on_wallpapersListView_pressed(const QModelIndex &index)
{
  auto itemText = index.data(Qt::DisplayRole).toString();
  selectedWallpaperName = itemText;
  selectedWallpaperPath = QDir::toNativeSeparators(wallpapersDirectory + QDir::separator() + itemText);
}

void MainWindow::on_wallpapersListView_doubleClicked()
{
  RunGifWallpaperProcess(selectedWallpaperPath);
}

void MainWindow::on_stopButton_clicked()
{
  StopGifWallpaperProcess();
}

void MainWindow::on_delaySlider_valueChanged(int value)
{
  delayBetweenFrames = value;
  ui->delayEdit->setText(QString::number(delayBetweenFrames));
}

void MainWindow::on_removeWalpaper_clicked()
{
  if (selectedWallpaperPath.isEmpty()) {
    QMessageBox msg(this);
    msg.setText(tr("Select a wallpaper to remove"));
    msg.exec();
    return;
  }
  QString dirName = selectedWallpaperName;
  auto ans =  QMessageBox::question(this,
                                    tr("Are you sure?"),
                                    tr("Remove \"") + dirName + "\" ?",
                                    QMessageBox::Yes | QMessageBox::No
                                    );
  if (ans == QMessageBox::Yes) {
    QDir dir(selectedWallpaperPath);
    dir.removeRecursively();
    UpdateWallpaperListView();
  }
}

/* Actions */

void MainWindow::on_actionPath_to_Imagemagick_triggered()
{
  LocateMagick();
}

void MainWindow::on_actionPath_to_gif_wallpaper_triggered()
{
  LocateGifWallpaper();
}


void MainWindow::on_actionQuit_triggered()
{
  QApplication::exit(0);
}

void MainWindow::on_actionHide_to_tray_triggered()
{
   this->hide();
}

void MainWindow::on_actionImport_BMP_triggered()
{
  ImportBMP();
}

void MainWindow::on_actionImport_GIF_triggered()
{
  ImportGIF();
}

/* Procedures */

void MainWindow::StopGifWallpaperProcess() {
  if (gif_wallpaper != nullptr) {
    gif_wallpaper->close();
    delete gif_wallpaper;
    gif_wallpaper = nullptr;
  }
}

void MainWindow::CreateSystemTrayIcon()
{
  QAction *showAction = new QAction(QStringLiteral("Gif-wallpaper"));
  QAction *nextWallpaperAction = new QAction(QStringLiteral("Next"));
  QAction *previousWallpaperAction = new QAction(QStringLiteral("Previous"));
  QAction *stopWallpaperAction = new QAction(QStringLiteral("Stop"));
  QAction *exitAction = new QAction(QStringLiteral("Exit"));

  // FIXME: After next/prev or prev/next actions it runs the same wallpaper
  connect(nextWallpaperAction, &QAction::triggered, this, [=]()
  {
    QString n;
    if (wallListIterator->hasNext()) {
      selectedWallpaperName = wallListIterator->peekNext();
      n = QDir::toNativeSeparators(wallpapersDirectory + QDir::separator() + wallListIterator->next());
    }
    else {
      wallListIterator->toFront();
      selectedWallpaperName = wallListIterator->peekNext();
      n = QDir::toNativeSeparators(wallpapersDirectory + QDir::separator() + wallListIterator->next());
    }
    RunGifWallpaperProcess(n);
  });
  connect(previousWallpaperAction, &QAction::triggered, this, [=]()
  {
    QString p;
    if (wallListIterator->hasPrevious()) {
      selectedWallpaperName = wallListIterator->peekPrevious();
      p = QDir::toNativeSeparators(wallpapersDirectory + QDir::separator() + wallListIterator->previous());
    }
    else {
      wallListIterator->toBack();
      selectedWallpaperName = wallListIterator->peekPrevious();
      p = QDir::toNativeSeparators(wallpapersDirectory + QDir::separator() + wallListIterator->previous());
    }
    RunGifWallpaperProcess(p);
  });
  connect(stopWallpaperAction, &QAction::triggered, this, [=]()
  {
    StopGifWallpaperProcess();
  });
  connect(showAction, &QAction::triggered, this, [=]()
  {
    this->show();
  });
  connect(exitAction , &QAction::triggered, this, [=]()
  {
    QApplication::exit(0);
  });

  QMenu *trayMenu = new QMenu(this);
  trayMenu->addAction(showAction);
  trayMenu->addSeparator();
  trayMenu->addAction(nextWallpaperAction);
  trayMenu->addAction(previousWallpaperAction);
  trayMenu->addAction(stopWallpaperAction);
  trayMenu->addSeparator();
  trayMenu->addAction(exitAction);

  QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);
  connect(trayIcon,
          SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this,
          SLOT(on_clickSystemTrayIcon(QSystemTrayIcon::ActivationReason))
  );

  trayIcon->setIcon(QIcon(":/assets/icon.ico"));
  trayIcon->setContextMenu(trayMenu);
  trayIcon->show();
}

void MainWindow::InitWallpapersList() {
  wallpapersDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(wallpapersDirectory);
  if (!dir.exists())
      dir.mkpath(wallpapersDirectory);
  if (!dir.exists("wallpapers"))
      dir.mkdir("wallpapers");
  dir.cd("wallpapers");
  wallpapersDirectory = dir.path();

  if (dir.isEmpty()) {
  }
  else {
    UpdateWallpaperListView();
  }
}

void MainWindow::UpdateWallpaperListView() {
    QDir dir(wallpapersDirectory);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    wallpapersListModel = new QStringListModel(this);
    QStringList l;
    QFileInfoList list = dir.entryInfoList();
    for (int i = 2; i < list.size(); ++i) {
      QFileInfo fileInfo = list.at(i);
      l.append(fileInfo.fileName());
    }
    wallpapersListModel->setStringList(l);
    wallListIterator = new QListIterator<QString>(wallpapersListModel->stringList());
    ui->wallpapersListView->setModel(wallpapersListModel);
}

void MainWindow::RunConvertProcess(QString gifPath, QString destPath) {
  QString fileName = QFileInfo(gifPath).baseName();
  QDir dir(destPath);
  QFile::copy(gifPath, dir.path() + QDir::separator() + fileName + ".gif");
  QString pathToGif = dir.path() + QDir::separator() + fileName + ".gif";
  QString pathToBmp = dir.path() + QDir::separator() + fileName + ".bmp";
  QStringList args;
  args << "convert" << "-coalesce" << pathToGif << pathToBmp;
  QProcess *convert = new QProcess();
  try {
    // Try to run the process and check if errorOccured() emited.
    // If failed to start, tell the user to install dependencies or set them in settings
    connect(convert, &QProcess::errorOccurred, [=](QProcess::ProcessError err) {
      qDebug() << err;
      if (err == QProcess::FailedToStart) {
        auto ans =  QMessageBox::question(this,
                                          tr("Failed to run convert"),
                                          tr("Unable to run magick.exe process.\nLocate the magick.exe?"),
                                          QMessageBox::Yes | QMessageBox::No
                                          );
        if (ans == QMessageBox::Yes) {
          magickCmd = QFileDialog::getOpenFileName(this,
                                                   tr("Locate magick.exe"),
                                                   QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation),
                                                   tr("Program files (*.exe)")
                                                   );
          if (magickCmd.isEmpty()) {
            QMessageBox msg(this);
            msg.setText(tr("Not setted.\nDownload and install Imagemagick to convert gifs."));
            msg.exec();
            magickCmd = "magick.exe"; // Set default
          }
        }
      }
    });
    convert->start(magickCmd, args);
    convert->setReadChannel(QProcess::StandardError);
    QString errors = convert->readAllStandardError();
    qDebug() << errors;
  }  catch(QException e) {
    qDebug() << e.what();
  }
  convert->waitForFinished();
  delete convert;
}

void MainWindow::RunGifWallpaperProcess(QString pathToWallpaper) {
  StopGifWallpaperProcess();
  gif_wallpaper = new QProcess();
  QStringList args;
  args << pathToWallpaper << QString::number(delayBetweenFrames);
  try {
    // Try to run the process and check if errorOccured() emited.
    // If failed to start, tell the user to install dependencies or set them in settings
    connect(gif_wallpaper, &QProcess::errorOccurred, [=](QProcess::ProcessError err) {
      qDebug() << err;
      if (err == QProcess::FailedToStart) {
        auto ans =  QMessageBox::question(this,
                                          tr("Failed to run gif_wallpaper.exe"),
                                          tr("Unable to run gw.exe process.\nLocate the gif_wallpaper?"),
                                          QMessageBox::Yes | QMessageBox::No
                                          );
        if (ans == QMessageBox::Yes) {
          gif_wallpaperCmd = QFileDialog::getOpenFileName(this,
                                                   tr("Locate gw.exe"),
                                                   QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation),
                                                   tr("Program files (*.exe)")
                                                   );
          if (gif_wallpaperCmd.isEmpty()) {
            QMessageBox msg(this);
            msg.setText(tr("Not setted.\nDownload and gif_wallpaper https://github.com/wvovaw/gif_wallpaper"));
            msg.exec();
            gif_wallpaperCmd = "gw.exe"; // Set default
          }
        }
      }
    });
    gif_wallpaper->start(gif_wallpaperCmd, args);
    ui->statusbar->showMessage(tr("Current wallpaper is \"") + selectedWallpaperName.toLocal8Bit().data() + "\"");
    qDebug(qPrintable(tr("Running gif-wallpaper on ") + pathToWallpaper + tr(" with delay ") + QString::number(delayBetweenFrames)));
    gif_wallpaper->setReadChannel(QProcess::StandardOutput);
    QString o = gif_wallpaper->readAllStandardOutput();
    qDebug() << o;
  }  catch(QException e) {
    qDebug() << e.what();
  }
}

void MainWindow::ImportBMP() {
  auto bmpPathsList = QFileDialog::getOpenFileNames(this,
                                               tr("Select wallpaper frames"),
                                               QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                               tr("Image Files (*.bmp)")
                                               );
  if (bmpPathsList.isEmpty()) return;
  auto wallpaperName = QInputDialog::getText(this, tr("Name this wallpaper"), tr("Name"), QLineEdit::Normal, "", nullptr, Qt::WindowFlags());
  if (!wallpaperName.isEmpty()) {
    QDir dir = QDir(wallpapersDirectory);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 2; i < list.size(); ++i) {
      if (list.at(i).baseName() == wallpaperName) {
        auto ans =  QMessageBox::question(this, tr("This name is already being used."), tr("Overwrite?"), QMessageBox::Yes | QMessageBox::No);
        if (ans == QMessageBox::Yes) {
          // Overwrite folder
          dir.cd(wallpaperName);
          dir.removeRecursively();
          dir.cd(wallpapersDirectory);
          dir.mkdir(wallpaperName);
          dir.cd(wallpaperName);
          for(const auto& item : bmpPathsList) {
            QFile::copy(item, dir.path() + QDir::separator() + QFileInfo(item).fileName());
          }
          UpdateWallpaperListView();
          return;
        }
        else return;
      }
    }
    // mkdir wallpaperName folder && cp * wallpaperName/
    dir.mkdir(wallpaperName);
    dir.cd(wallpaperName);
    for(const auto& item : bmpPathsList) {
      QFile::copy(item, dir.path() + QDir::separator() + QFileInfo(item).fileName());
    }
    // Update model, refresh view
    UpdateWallpaperListView();
  }
}

void MainWindow::ImportGIF() {
  auto gifPath = QFileDialog::getOpenFileName(this,
                                              tr("Open gif"),
                                              QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                              tr("Image Files (*.gif)")
                                              );
  if (gifPath.isEmpty()) return;
  auto wallpaperName = QInputDialog::getText(this, tr("Name this wallpaper"), tr("Name"), QLineEdit::Normal, "", nullptr, Qt::WindowFlags());
  if (!wallpaperName.isEmpty()) {
    QDir dir = QDir(wallpapersDirectory);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 2; i < list.size(); ++i) {
      if (list.at(i).baseName() == wallpaperName) {
        auto ans =  QMessageBox::question(this, tr("This name is already being used."), tr("Overwrite?"), QMessageBox::Yes | QMessageBox::No);
        if (ans == QMessageBox::Yes) {
          // Overwrite folder
          dir.cd(wallpaperName);
          dir.removeRecursively();
          dir.cd(wallpapersDirectory);
          dir.mkdir(wallpaperName);
          dir.cd(wallpaperName);
          RunConvertProcess(gifPath, dir.path());
          UpdateWallpaperListView();
          return;
        }
        else return;
      }
    }
    dir.mkdir(wallpaperName);
    dir.cd(wallpaperName);
    RunConvertProcess(gifPath, dir.path());
    UpdateWallpaperListView();
  }
}

void MainWindow::LocateGifWallpaper() {
  gif_wallpaperCmd = QFileDialog::getOpenFileName(this,
                                                  tr("Locate gw.exe"),
                                                  QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation),
                                                  tr("Program files (*.exe)")
                                                  );
  if (gif_wallpaperCmd.isEmpty()) {
    QMessageBox msg(this);
    msg.setText(tr("Not setted.\nDownload and gif_wallpaper https://github.com/wvovaw/gif_wallpaper"));
    msg.exec();
    gif_wallpaperCmd = "gw.exe"; // Set default
  }
}

void MainWindow::LocateMagick() {
  magickCmd = QFileDialog::getOpenFileName(this, tr("Locate magick.exe"),
                                           QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation),
                                           tr("Program files (*.exe)")
                                           );
  if (magickCmd.isEmpty()) {
    QMessageBox msg(this);
    msg.setText(tr("Not setted.\nDownload and install Imagemagick to convert gifs."));
    msg.exec();
    magickCmd = "magick.exe"; // Set default
  }
}
