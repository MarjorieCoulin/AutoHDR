#ifndef AUTOHDR_CAPTUREEND_H
#define AUTOHDR_CAPTUREEND_H

#include <QDialog>

namespace Ui
{
class AutoHDR_CaptureEnd;
}

class AutoHDR_CaptureEnd : public QDialog
{
    Q_OBJECT

  public:
    explicit AutoHDR_CaptureEnd(QWidget *parent = 0);
    ~AutoHDR_CaptureEnd();

    void setStatus(QString status);
    void setFolder(QString folder);

  signals:
    void acceptCapture();

  private:
    Ui::AutoHDR_CaptureEnd *ui;

  private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // AUTOHDR_CAPTUREEND_H
