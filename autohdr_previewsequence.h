#ifndef AUTOHDR_PREVIEWSEQUENCE_H
#define AUTOHDR_PREVIEWSEQUENCE_H

#include <QDialog>
#include <QTimer>

namespace Ui
{
class AutoHDR_PreviewSequence;
}

class AutoHDR_PreviewSequence : public QDialog
{
    Q_OBJECT

  public:
    explicit AutoHDR_PreviewSequence(QWidget *parent = 0);
    ~AutoHDR_PreviewSequence();

    /* Setters */
    void setLowerPreview(QImage preview);
    void setUpperPreview(QImage preview);
    void setLabelResults(QString str);

    void startCountdown();
    void stopCountdown();

  public slots:
    void captureCountdown();

  signals:
    void acceptSequence();
    void rejectSequence();

  private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();

  private:
    Ui::AutoHDR_PreviewSequence *ui;
    QTimer *timerStart;
    int countdown;
};

#endif // AUTOHDR_PREVIEWSEQUENCE_H
