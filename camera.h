#ifndef CAMERA_H
#define CAMERA_H
#include "config.h"
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QStringList>
#include <QTimer>

/* libgphoto2 */
#include <gphoto2/gphoto2-camera.h>

class RemoteCamera : public QObject
{
    Q_OBJECT

  public:
    explicit RemoteCamera(Config *config = nullptr, QObject *parent = nullptr);
    ~RemoteCamera();

    int initCameraConfig();
    int captureShot(QString &capturePath);
    int captureLiveView();
    int increaseExposure(QString &next);
    int decreaseExposure(QString &next);

    void distributeExposures(QString &first, QString &last, int spacing,
                             QList<QString> &list);

    /* Getters */
    QStringList getCapabilitiesISO();
    QStringList getCapabilitiesAperture();
    QStringList getCapabilitiesExposure();
    QString getCurrentISO();
    QString getCurrentAperture();
    QString getCurrentExposure();
    /* Setters */
    void setCurrentISO(QString &ISO);
    void setCurrentAperture(QString &aperture);
    void setCurrentExposure(QString &exposure);
    void setMaxExposure(QString exposure);

  public slots:
    void connectCamera();

  signals:
    void connected();

  private:
    Config *conf;

    /* libGphoto2 */
    Camera *camera;
    GPContext *context;
    struct {
        CameraWidget *root = NULL;
        CameraWidget *ISO = NULL;
        CameraWidget *aperture = NULL;
        CameraWidget *exposure = NULL;
    } cameraConfig;
    /* Context accessibility */
    QMutex mutex;
    /* Connection */
    QTimer *retry;

    /* Lists what the camera can do */
    struct {
        QStringList ISO;
        QStringList aperture;
        QStringList exposure;
    } cameraCapabilities;
    /* Current camera parameters */
    struct {
        QString ISO;
        QString aperture;
        QString exposure;
    } currentParams;
    QString maxExposure;

    int getCameraConfig(CameraWidget **config, QString configStr,
                        QStringList &capabilities);
    int getCurrentCameraParam(CameraWidget *config, QString &currentParam);
    int setCurrentCameraParam(CameraWidget *config, const char *configStr,
                              const char *value);
};

#endif // CAMERA_H
