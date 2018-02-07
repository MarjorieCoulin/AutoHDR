#include "autohdr_compose.h"
#include "ui_autohdr_compose.h"

AutoHDR_Compose::AutoHDR_Compose(QWidget *parent)
    : QDialog(parent), ui(new Ui::AutoHDR_Compose)
{
    ui->setupUi(this);

    /* UI */
    connect(ui->buttonStop, SIGNAL(pressed()), this, SLOT(handleStop()));
}

AutoHDR_Compose::~AutoHDR_Compose()
{
    delete ui;
}

void AutoHDR_Compose::updateStatus(QString status)
{
    ui->status->setText(status);
}

void AutoHDR_Compose::handleStop()
{
    emit abortCompose();
}
