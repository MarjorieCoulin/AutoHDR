#include "captureworker.h"
#include "config.h"

/*! \brief CaptureWorker constructor
 *
 * Uses references to instances of Sequence, RemoteCamera
 * and Config created by AutoHDR_MainWindow
 */
CaptureWorker::CaptureWorker(Config *config, RemoteCamera *camera,
                             Sequence *seq, QObject *parent)
    : QObject(parent)
{
    conf = config;
    c = camera;
    s = seq;
    captureRun = true;
}

/*! \brief Capture sequence
 *
 * Continously takes all photos defined in shots list.
 * Names and stores the files accordingly to config
 */
void CaptureWorker::captureSequence()
{
    if (!c || !s || !conf)
        return;

    int n = s->getShotsNb();

    for (int i = 0; i < n; i++) {
        if (!captureRun)
            break;
        /* Set up camera */
        shotParameters sp = s->getShotParameters(i);
        c->setCurrentISO(sp.ISO);
        c->setCurrentAperture(sp.aperture);
        c->setCurrentExposure(sp.exposure);
        /* Capture */
        QString fp = conf->getCaptureFolder() + conf->getShotName(i);
        if (c->captureShot(fp) == GP_OK) {
            s->setShotPath(i, fp);
            emit captureProgress(i + 1, n);
        } else {
            emit captureError();
            break;
        }
    }
}

/*! \brief Set capture state
 *
 * If state = false capture will stop after current photo
 * is captured
 */
void CaptureWorker::setCaptureRunState(bool state)
{
    captureRun = state;
}
