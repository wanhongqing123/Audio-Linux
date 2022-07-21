#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audio_mixer_manager_alsa_linux.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonMicUpdate_clicked() {
    GetAlsaSymbolTable()->Load();
}
