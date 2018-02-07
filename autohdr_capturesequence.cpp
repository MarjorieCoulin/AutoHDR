#include "autohdr_capturesequence.h"
#include "ui_autohdr_capturesequence.h"

AutoHDR_CaptureSequence::AutoHDR_CaptureSequence(QWidget *parent)
    : QDialog(parent), ui(new Ui::AutoHDR_CaptureSequence)
{
    ui->setupUi(this);

    /* UI */
    connect(ui->buttonStop, SIGNAL(pressed()), this, SLOT(handleStop()));
}

AutoHDR_CaptureSequence::~AutoHDR_CaptureSequence()
{
    delete ui;
}

void AutoHDR_CaptureSequence::setProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

void AutoHDR_CaptureSequence::setStatus(QString status)
{
    ui->status->setText(status);
}

void AutoHDR_CaptureSequence::handleStop()
{
    emit abortCaptureSequence();
}
