#ifndef LIVEVIEWWORKER_H
#define LIVEVIEWWORKER_H

#include "camera.h"
#include <QImage>
#include <QObject>

class LiveViewWorker : public QObject
{
    Q_OBJECT
  public:
    explicit LiveViewWorker(QObject *parent = nullptr);

    void setCamera(RemoteCamera *camera);
    void setLiveViewRunState(bool state);

  signals:
    void imageReady(QImage *image);

  public slots:
    void captureLiveView();

  private:
    RemoteCamera *c;
    QImage liveView;
    bool liveViewRun;
};

#endif // LIVEVIEWWORKER_H
