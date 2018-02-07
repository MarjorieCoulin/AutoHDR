#ifndef CAPTUREWORKER_H
#define CAPTUREWORKER_H

#include "camera.h"
#include "sequence.h"
#include <QObject>

class Config;

class CaptureWorker : public QObject
{
    Q_OBJECT
  public:
    explicit CaptureWorker(Config *config = nullptr,
                           RemoteCamera *camera = nullptr,
                           Sequence *seq = nullptr, QObject *parent = nullptr);

    void setCaptureRunState(bool state);

  signals:
    void captureProgress(int imgNb, int total);
    void captureError();

  public slots:
    void captureSequence();

  private:
    RemoteCamera *c;
    Sequence *s;
    Config *conf;
    bool captureRun;
};

#endif // CAPTUREWORKER_H
