#ifndef AUTOHDR_COMPOSE_H
#define AUTOHDR_COMPOSE_H

#include <QDialog>
#include <QString>

namespace Ui
{
class AutoHDR_Compose;
}

class AutoHDR_Compose : public QDialog
{
    Q_OBJECT

  public:
    explicit AutoHDR_Compose(QWidget *parent = 0);
    ~AutoHDR_Compose();

    void updateStatus(QString status);

  signals:
    void abortCompose();

  private slots:
    void handleStop();

  private:
    Ui::AutoHDR_Compose *ui;
};

#endif // AUTOHDR_COMPOSE_H
