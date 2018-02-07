#ifndef AUTOHDR_COMPUTESEQUENCE_H
#define AUTOHDR_COMPUTESEQUENCE_H

#include <QDialog>
#include <QString>

namespace Ui
{
class AutoHDR_ComputeSequence;
}

class AutoHDR_ComputeSequence : public QDialog
{
    Q_OBJECT

  public:
    explicit AutoHDR_ComputeSequence(QWidget *parent = 0);
    ~AutoHDR_ComputeSequence();

    void updateStatus(QString status);
    void setProgress(int progress);

  private slots:
    void handleStop();

  private:
    Ui::AutoHDR_ComputeSequence *ui;

  signals:
    void abortComputeSequence();
};

#endif // AUTOHDR_COMPUTESEQUENCE_H
