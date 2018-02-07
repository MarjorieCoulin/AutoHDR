#include "autohdr_captureend.h"
#include "ui_autohdr_captureend.h"

AutoHDR_CaptureEnd::AutoHDR_CaptureEnd(QWidget *parent)
    : QDialog(parent), ui(new Ui::AutoHDR_CaptureEnd)
{
    ui->setupUi(this);
}

AutoHDR_CaptureEnd::~AutoHDR_CaptureEnd()
{
    delete ui;
}

void AutoHDR_CaptureEnd::setStatus(QString status)
{
    ui->status->setText(status);
}

void AutoHDR_CaptureEnd::setFolder(QString folder)
{
    ui->labelFolder->setText(folder);
}

void AutoHDR_CaptureEnd::on_buttonBox_rejected()
{
    close();
}

void AutoHDR_CaptureEnd::on_buttonBox_accepted()
{
    emit acceptCapture();
    close();
}
