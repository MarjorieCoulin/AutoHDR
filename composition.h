#ifndef COMPOSITION_H
#define COMPOSITION_H

#include "autohdr_compose.h"
#include "config.h"
#include "sequence.h"
#include <QObject>
#include <QProcess>

class Composition : public QObject
{
    Q_OBJECT

  public:
    explicit Composition(Sequence *seq = nullptr, Config *config = nullptr,
                         QObject *parent = nullptr);
    ~Composition();

  public slots:
    void startComposition();
    void abortComposition();
    void handleError(QProcess::ProcessError error);
    void handleFinished(int code, QProcess::ExitStatus status);

  private:
    AutoHDR_Compose composeDialog;
    QProcess *p;
    Config *conf;
    Sequence *s;
};

#endif // COMPOSITION_H
