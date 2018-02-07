#ifndef AUTOHDR_CAPTURESEQUENCE_H
#define AUTOHDR_CAPTURESEQUENCE_H

#include <QDialog>

namespace Ui
{
class AutoHDR_CaptureSequence;
}

class AutoHDR_CaptureSequence : public QDialog
{
    Q_OBJECT

  public:
    explicit AutoHDR_CaptureSequence(QWidget *parent = 0);
    ~AutoHDR_CaptureSequence();

    void setStatus(QString status);
    void setProgress(int progress);

  private slots:
    void handleStop();

  private:
    Ui::AutoHDR_CaptureSequence *ui;

  signals:
    void abortCaptureSequence();
};

#endif // AUTOHDR_CAPTURESEQUENCE_H
