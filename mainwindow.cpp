#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QSystemTrayIcon>
#include <QPushButton>
#include <QStringLiteral>
#include <QAction>
#include <QDebug>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  //setWindowFlags(Qt::Tool);
  CreateSystemTrayIcon(); // Create a tray
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::CreateSystemTrayIcon()
{
  QAction *showAction = new QAction(QStringLiteral("Show"));
  QAction *exitAction = new QAction(QStringLiteral("Exit"));
  connect(showAction, &QAction::triggered, this, [=]()
  {
    this->show();
  });
  connect(exitAction , &QAction::triggered, this, [=]()
  {
    qDebug()<<"exit";
    QApplication::exit(0);
  });

  QMenu *trayMenu = new QMenu(this);
  trayMenu->addAction(showAction);
  trayMenu->addAction(exitAction);

  QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);
  trayIcon->setIcon(QIcon(":/assets/icon.ico"));
  trayIcon->setContextMenu(trayMenu);
  trayIcon->show();
}

void MainWindow::on_closeButton_clicked()
{
  qDebug()<<"exit";
  QApplication::exit(0);
}

void MainWindow::on_toTrayButton_clicked()
{
  this->hide();
}

void MainWindow::on_actionQuit_triggered()
{
  qDebug()<<"exit";
  QApplication::exit(0);
}

void MainWindow::on_actionHide_to_tray_triggered()
{
   this->hide();
}

void MainWindow::on_runButton_clicked()
{

}
