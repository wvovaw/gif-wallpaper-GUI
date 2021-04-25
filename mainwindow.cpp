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
  ui->speedSlider->setRange(minDelay, maxDelay);
  ui->speedSlider->setSliderPosition(delayBetweenFrames);
  ui->speedEdit->setText(QString::number(delayBetweenFrames));
  CheckForDependencies();
  CreateSystemTrayIcon();
  InitWallpapersList();
}

MainWindow::~MainWindow()
{
  gif_wallpaper->close();
  delete gif_wallpaper;
  delete wallpapersListModel;
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
  gif_wallpaper->close();
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
    msg.setText("Select a wallpaper");
    msg.exec();
  }
  else {
    RunGifWallpaperProcess(selectedWallpaperPath);
  }
}

void MainWindow::on_wallpapersListView_pressed(const QModelIndex &index)
{
  auto itemText = index.data(Qt::DisplayRole).toString();
  selectedWallpaperPath = QDir::toNativeSeparators(wallpapersDirectory + QDir::separator() + itemText);
}

void MainWindow::on_wallpapersListView_doubleClicked()
{
  RunGifWallpaperProcess(selectedWallpaperPath);
}

void MainWindow::on_stopButton_clicked()
{
   gif_wallpaper->close();
}

void MainWindow::on_speedSlider_valueChanged(int value)
{
  delayBetweenFrames = value;
  ui->speedEdit->setText(QString::number(delayBetweenFrames));
}


/* Actions */

void MainWindow::on_actionQuit_triggered()
{
  gif_wallpaper->close();
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

void MainWindow::on_actionSettings_triggered()
{
  // TODO: Open dialog with 2 inputs: Path to magick.exe and to gw.exe. 2 inputs, 2 buttons that open FileDialog
}

/* Procedures */

void MainWindow::CreateSystemTrayIcon()
{
  QAction *showAction = new QAction(QStringLiteral("Gif-wallpaper"));
  QAction *nextWallpaperAction = new QAction(QStringLiteral("Next"));
  QAction *previousWallpaperAction = new QAction(QStringLiteral("Previous"));
  QAction *stopWallpaperAction = new QAction(QStringLiteral("Stop"));
  QAction *exitAction = new QAction(QStringLiteral("Exit"));

  connect(nextWallpaperAction, &QAction::triggered, this, [=]()
  {
    // TODO: next wallpaper form wallpapersListModel
  });
  connect(previousWallpaperAction, &QAction::triggered, this, [=]()
  {
    // TODO: prev wallpaper from wallpapersListModel
  });
  connect(stopWallpaperAction, &QAction::triggered, this, [=]()
  {
    if (gif_wallpaper != nullptr) {
      gif_wallpaper->close();
      delete gif_wallpaper;
      gif_wallpaper = nullptr;
    }
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
    // TODO: Check if the command exists, if not, say user to install or add path to the command in settings
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
  if (gif_wallpaper != nullptr) {
    gif_wallpaper->close();
    delete gif_wallpaper;
    gif_wallpaper = nullptr;
  }
  gif_wallpaper = new QProcess();
  QStringList args;
  args << pathToWallpaper << QString::number(delayBetweenFrames);
  qDebug(qPrintable("Running gif-wallpaper on " + pathToWallpaper + " with speed " + QString::number(delayBetweenFrames)));
  try {
    gif_wallpaper->start(magickCmd, args);
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
  auto wallpaperName = QInputDialog::getText(this, "Name this wallpaper", "Name", QLineEdit::Normal, "", nullptr, Qt::WindowFlags());
  if (!wallpaperName.isEmpty()) {
    QDir dir = QDir(wallpapersDirectory);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 2; i < list.size(); ++i) {
      if (list.at(i).baseName() == wallpaperName) {
        auto ans =  QMessageBox::question(this, "This name is already being used.", "Overwrite?", QMessageBox::Yes | QMessageBox::No);
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
  auto wallpaperName = QInputDialog::getText(this, "Name this wallpaper", "Name", QLineEdit::Normal, "", nullptr, Qt::WindowFlags());
  if (!wallpaperName.isEmpty()) {
    QDir dir = QDir(wallpapersDirectory);
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 2; i < list.size(); ++i) {
      if (list.at(i).baseName() == wallpaperName) {
        auto ans =  QMessageBox::question(this, "This name is already being used.", "Overwrite?", QMessageBox::Yes | QMessageBox::No);
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

void MainWindow::CheckForDependencies() {
  // TODO: Check if gw.exe and magick.exe exitst in the PATH. Create MessageBox with corresponding info
}
