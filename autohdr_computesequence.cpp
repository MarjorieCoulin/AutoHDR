#include "autohdr_computesequence.h"
#include "ui_autohdr_computesequence.h"

AutoHDR_ComputeSequence::AutoHDR_ComputeSequence(QWidget *parent)
    : QDialog(parent), ui(new Ui::AutoHDR_ComputeSequence)
{
    ui->setupUi(this);

    /* UI */
    connect(ui->buttonStop, SIGNAL(pressed()), this, SLOT(handleStop()));
}

AutoHDR_ComputeSequence::~AutoHDR_ComputeSequence()
{
    delete ui;
}

void AutoHDR_ComputeSequence::updateStatus(QString status)
{
    ui->status->setText(status);
}

void AutoHDR_ComputeSequence::setProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

void AutoHDR_ComputeSequence::handleStop()
{
    emit abortComputeSequence();
}
