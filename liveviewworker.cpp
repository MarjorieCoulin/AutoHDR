#include "liveviewworker.h"

/*! \brief LiveViewWorker constructor
 *
 * Liveview capture starts immediately
 */
LiveViewWorker::LiveViewWorker(QObject *parent) : QObject(parent)
{
    liveViewRun = true;
}

void LiveViewWorker::setCamera(RemoteCamera *camera)
{
    c = camera;
}

/*! \brief Get liveview camera picture
 *
 * Continuous capture in a worker thread.
 */
void LiveViewWorker::captureLiveView()
{
    while (liveViewRun) {
        if (c->captureLiveView() < 0)
            break;

        liveView = QImage("/tmp/liveview.jpg");
        emit imageReady(&liveView);
    }
}

/*! \brief Set liveview worker state
 *
 * state = false definitely stops liveview
 */
void LiveViewWorker::setLiveViewRunState(bool state)
{
    liveViewRun = state;
}
