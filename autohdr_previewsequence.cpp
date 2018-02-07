#include "autohdr_previewsequence.h"
#include "ui_autohdr_previewsequence.h"

AutoHDR_PreviewSequence::AutoHDR_PreviewSequence(QWidget *parent)
    : QDialog(parent), ui(new Ui::AutoHDR_PreviewSequence)
{
    ui->setupUi(this);

    timerStart = new QTimer(this);
    connect(timerStart, SIGNAL(timeout()), this, SLOT(captureCountdown()));
}

AutoHDR_PreviewSequence::~AutoHDR_PreviewSequence()
{
    delete ui;
    delete timerStart;
}

/*! \brief Start countdown before automatic capture
 *
 * Countdown is set to 10 seconds
 */
void AutoHDR_PreviewSequence::startCountdown()
{
    countdown = 10;
    captureCountdown();
}

/*! \brief Stop countdown to automatic capture
 */
void AutoHDR_PreviewSequence::stopCountdown()
{
    timerStart->stop();
}

/*! \brief Manage automatic capture countdown
 *
 * If countdown times out, start capture
 * Else update countdown label and wait for another 1s
 */
void AutoHDR_PreviewSequence::captureCountdown()
{
    timerStart->stop();

    if (!countdown) {
        emit acceptSequence();
    } else {
        ui->labelStart->setText("Start Capture ? Automatic start in " +
                                QString::number(countdown) + "s...");
        countdown--;
        timerStart->start(1000);
    }
}

void AutoHDR_PreviewSequence::setLowerPreview(QImage preview)
{
    ui->lowerPreview->setPixmap(QPixmap::fromImage(preview));
}

void AutoHDR_PreviewSequence::setUpperPreview(QImage preview)
{
    ui->upperPreview->setPixmap(QPixmap::fromImage(preview));
}

void AutoHDR_PreviewSequence::setLabelResults(QString str)
{
    ui->labelResults->setText(str);
}

void AutoHDR_PreviewSequence::on_buttonBox_rejected()
{
    emit rejectSequence();
}

void AutoHDR_PreviewSequence::on_buttonBox_accepted()
{
    emit acceptSequence();
}
