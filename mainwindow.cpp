#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audio_device_alsa_linux.h"
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
  AudioDeviceAlsaLinux *device = new AudioDeviceAlsaLinux();
  device->Init();
  /*
  int32_t GetDevicesInfo(const int32_t function,
                                             const bool playback,
                                             const int32_t enumDeviceNo,
                                             char* enumDeviceName,
                                             const int32_t ednLen);
  */
  char device_name[256] = "";
  device->GetDevicesInfo(2, false, 0,device_name, 256);
  device->GetDevicesInfo(2, false, 1,device_name, 256);
  device->GetDevicesInfo(2, false, 2,device_name, 256);
}
