#include "sequence.h"
#include "captureworker.h"
#include "composition.h"
#include <QMessageBox>

/*! \brief Sequence constructor
 *
 * Uses references to instances of RemoteCamera and Config
 * created by AutoHDR_MainWindow
 *
 * Creates an instance of Composition
 *
 * Manages a variety of signals :
 * - Compute sequence abort from AutoHDR_ComputeSequence UI
 * - Sequence accept or reject from AutoHDR_PreviewSequence UI
 * - Capture sequence progress and error from CaptureWorker
 * - Capture sequence abort from AutoHDR_CaptureSequence UI
 * - Composition start from AutoHDR_ComputeEnd UI
 * - Sequence state machine sequencer
 * Manages CaptureWorker thread
 */
Sequence::Sequence(RemoteCamera *cam, Config *conf, QWidget *parent)
    : QWidget(parent)
{
    c = cam;
    config = conf;
    state = CS_IDLE;
    comp = new Composition(this);

    /* Link with compute window */
    connect(&computeSequenceDialog,
            &AutoHDR_ComputeSequence::abortComputeSequence, this,
            &Sequence::computeSequenceAborted);

    /* Link with sequence preview window */
    connect(&previewSequenceDialog, &AutoHDR_PreviewSequence::acceptSequence,
            this, &Sequence::sequenceAccepted);
    connect(&previewSequenceDialog, &AutoHDR_PreviewSequence::rejectSequence,
            this, &Sequence::sequenceRejected);

    /* Link with capture window */
    connect(&captureSequenceDialog,
            &AutoHDR_CaptureSequence::abortCaptureSequence, this,
            &Sequence::captureSequenceAborted);

    /* Link with end capture dialog */
    connect(&captureEndDialog, &AutoHDR_CaptureEnd::acceptCapture, comp,
            &Composition::startComposition);

    /* Capture thread */
    captureWorker = new CaptureWorker(config, c, this);
    captureWorker->moveToThread(&sequenceAcquisition);
    connect(&sequenceAcquisition, &QThread::finished, captureWorker,
            &QObject::deleteLater);
    connect(this, &Sequence::startSequenceCapture, captureWorker,
            &CaptureWorker::captureSequence);
    connect(captureWorker, &CaptureWorker::captureProgress, this,
            &Sequence::handleCaptureProgress);
    connect(captureWorker, &CaptureWorker::captureError, this,
            &Sequence::captureSequenceError);
    sequenceAcquisition.start();
}

/*! \brief Sequence destructor
 *
 * Stops CaptureWorker thread
 */
Sequence::~Sequence()
{
    /* Stop capture if need be */
    captureWorker->setCaptureRunState(false);
    sequenceAcquisition.quit();
    sequenceAcquisition.wait();

    delete comp;
}

/*! \brief Save current parameters at compute sequence start
 */
void Sequence::setStartParameters(QString ISO, QString ap, QString exp)
{
    startParam.ISO = ISO;
    startParam.aperture = ap;
    startParam.exposure = exp;
}

/*! \brief Trigger computing state machine
 */
void Sequence::startComputing()
{
    if (state == CS_IDLE) {
        state = CS_START;
        clearSequence();
    }
}

/*! \brief Stall computing state machine
 */
void Sequence::abortComputing()
{
    state = CS_IDLE;
}

/*! \brief Check overexposition of a pixel
 *
 * A pixel is overexposed if at least one of its
 * color components is above given threshold
 */
static bool isOverExposed(QRgb px, unsigned char overexp_threshold)
{
    return ((((px & 0x00FF0000) >> 16) >= overexp_threshold) ||
            (((px & 0x0000FF00) >> 8) >= overexp_threshold) ||
            ((px & 0x000000FF) >= overexp_threshold));
}

/*! \brief Check underexposition of a pixel
 *
 * A pixel is underexposed if all of its
 * color components are under given threshold
 */
static bool isUnderExposed(QRgb px, unsigned char underexp_threshold)
{
    return ((((px & 0x00FF0000) >> 16) <= underexp_threshold) &&
            (((px & 0x0000FF00) >> 8) <= underexp_threshold) &&
            ((px & 0x000000FF) <= underexp_threshold));
}

enum ExpositionType { UNDER_EXPOSITION, OVER_EXPOSITION };

/*! \brief Rate image (under- or over-) exposition
 *
 * Analyses every pixel of an image and returns a percentage
 * of under- or over-exposition
 */
static int rateImageExposition(Config *conf, QImage &image, ExpositionType exp)
{
    int n = 0;
    int w = image.width();
    int h = image.height();
    bool (*pf)(QRgb, unsigned char);
    unsigned char threshold;

    /* If the image is invalid
     * make the function harmless */
    if (!w || !h)
        return -1;

    if (exp == UNDER_EXPOSITION) {
        pf = &isUnderExposed;
        threshold = conf->getBlackThreshold();
    } else {
        pf = &isOverExposed;
        threshold = conf->getWhiteThreshold();
    }

    /* Analyse every pixel of the image */
    for (int i = 0; i < w; i++)
        for (int j = 0; j < h; j++)
            if (pf(image.pixel(i, j), threshold))
                n++;

    /* Return an exposition percentage */
    return n * 100 / (w * h);
}

/*! \brief Save a sequence boundary
 *
 * Saves an image and its associated parameters
 * to the shots list
 */
void Sequence::setSequenceBoundary()
{
    shotParameters sp = {.ISO = c->getCurrentISO(),
                         .aperture = c->getCurrentAperture(),
                         .exposure = c->getCurrentExposure(),
                         .preview = currentView,
                         .path = QString()};
    shots.append(sp);
}

/*! \brief Sets the number of shots needed between 2 boundaries
 *
 * 1 shot needed if no parameter was changed during analysis
 * 2 shots needed if boundaries are not too far from each other
 * Generate more shots in between if the exposure gap is wider
 */
void Sequence::distributeShots(int stops)
{
    QList<QString> distExp;
    int n;
    int shotsGap = config->getShotsGap();

    if (!stops) {
        /* This is no need for an HDR photo */
        /* Boundaries are 2 identic shots
         * delete one */
        shots.removeLast();
        goto out;
    }

    /* Distance between 2 shots must not exceed 2EV
     * Number of images = number of gaps - 1 */
    n = stops / shotsGap - 1;
    /* In case of floating value, take one more shot */
    if ((n + 1) * shotsGap != stops)
        n++;

    if (n > (nbImgMax - 2)) {
        /* Error if we exceed max desired value
         * (max number of images includes boundaries) */
        shots.clear();
        goto out;
    }
    if (n == 2)
        /* There are already 2 different shots
         * saved as boundaries */
        goto out;

    /* We need to generate shots between boundaries */
    c->distributeExposures(shots.first().exposure, shots.last().exposure,
                           shotsGap, distExp);
    /* We get the list of exposures needed in between */
    for (int i = 0; i < distExp.size(); i++) {
        /* Build new shot to insert */
        shotParameters p = {
            .ISO = shots.first().ISO,
            .aperture = shots.first().aperture,
            .exposure = distExp.at(i),
            .preview = QImage(), /* No preview */
            .path = QString()    /* No path */
        };
        shots.insert(i + 1, p);
    }

out:
    fprintf(stdout, "[Sequence] Sequence needs %d shots : ", shots.size());
    for (int i = 0; i < shots.size(); i++)
        fprintf(stdout, " %s", shots.at(i).exposure.toStdString().c_str());
    fprintf(stdout, "\n");
}

/*! \brief Sequence analysis state machine
 *
 * Triggered by liveview update
 * States available :
 * - CS_IDLE : State machine not running
 * - CS_START : State machine initializing
 * - CS_LOWER_CRITERIA_SEEKING : Looking for lower exposure criteria
 * - CS_LOWER_CRITERIA_FOUND
 * - CS_UPPER_CRITERIA_SEEKING : Looking for upper exposure criteria
 * - CS_UPPER_CRITERIA_FOUND
 */
void Sequence::runStateMachine(QImage *image)
{
    static int stopsNb;
    static QString exposure;

    if (state != CS_IDLE) {
        /* Check is exposure was correctly updated */
        if (c->getCurrentExposure() == exposure) {
            /* Update reference image */
            currentView = QImage(*image);
        }
    }

    /* State machine */
    switch (state) {
    case CS_IDLE:
        /* Computing is not running */
        break;

    case CS_START:
        /* Update UI and start seeking for 1st criteria */
        manageSequenceUI();
        state = CS_LOWER_CRITERIA_SEEKING;
        stopsNb = 0;
        exposure = c->getCurrentExposure();
        fprintf(stdout, "[Sequence] Starting lower criteria seeking\n");
        break;

    case CS_LOWER_CRITERIA_SEEKING: {
        /* Look for less than % of overexposed pixels
         * in the first shot of the sequence (underexposed) */
        int overExpRate =
            rateImageExposition(config, currentView, OVER_EXPOSITION);
        if (overExpRate < 0) /* Error, try later */
            break;
        if (overExpRate < overExpCrit) {
            /* Found an image that meets lower criteria */
            setSequenceBoundary();
            state = CS_LOWER_CRITERIA_FOUND;
        } else {
            /* Decrease exposition */
            if (c->decreaseExposure(exposure) != GP_OK) {
                state = CS_IDLE;
                manageSequenceError("Reached camera under-exposition limit");
            } else
                stopsNb++;
        }
        break;
    }

    case CS_LOWER_CRITERIA_FOUND:
        manageSequenceUI();
        state = CS_UPPER_CRITERIA_SEEKING;
        exposure = c->getCurrentExposure();
        fprintf(stdout, "[Sequence] Starting upper criteria seeking\n");
        break;

    case CS_UPPER_CRITERIA_SEEKING: {
        /* Look for less than % of underexposed pixels
         * in the last shot of the sequence (overexposed) */
        int underExpRate =
            rateImageExposition(config, currentView, UNDER_EXPOSITION);
        if (underExpRate < 0) /* Error, try later */
            break;
        if (underExpRate < underExpCrit) {
            /* Found an image that meets upper criteria */
            setSequenceBoundary();
            state = CS_UPPER_CRITERIA_FOUND;
        } else {
            /* Increase exposition */
            if (c->increaseExposure(exposure) != GP_OK) {
                state = CS_IDLE;
                manageSequenceError("Reached maximum exposition");
            } else
                stopsNb++;
        }
        break;
    }

    case CS_UPPER_CRITERIA_FOUND:
        distributeShots(stopsNb);
        if (!shots.size()) {
            state = CS_IDLE;
            manageSequenceError("Maximum shots in sequence exceeded");
        } else {
            manageSequenceUI();
            state = CS_IDLE;
        }
        break;
    }
}

void Sequence::getCriterias(int *lower, int *upper, int *nb)
{
    *lower = overExpCrit;
    *upper = underExpCrit;
    *nb = nbImgMax;
}

int Sequence::getShotsNb()
{
    return shots.size();
}

shotParameters Sequence::getShotParameters(int n)
{
    return shots.at(n);
}

void Sequence::setShot(shotParameters sp)
{
    shots.push_back(sp);
}

void Sequence::setShotPath(int n, const QString path)
{
    shots[n].path = path;
}

/*! \brief Set analysis criterias
 *
 * Check values (from Config or AutoHDR_MainWindow UI)
 * and save for use during analysis
 */
void Sequence::setCriterias(int lower, int upper, int nb)
{
    /* Criterias are percentages */

    if (lower <= 0)
        overExpCrit = 1;
    else if (lower > 100)
        overExpCrit = 100; /* Not realist but hey. */
    else
        overExpCrit = lower;

    if (upper <= 0)
        underExpCrit = 1;
    else if (upper > 100)
        underExpCrit = 100; /* Not realist but hey. */
    else
        underExpCrit = upper;

    nbImgMax = nb;
}

/*! \brief Reset sequence shots list
 */
void Sequence::clearSequence()
{
    shots.clear();
}

/*! \brief Reset camara parameters
 *
 * Reset Camera to initial ISO, aperture and exposure values
 * (as set before starting computing sequence)
 */
void Sequence::resetParams()
{
    /* Reset Camera to initial ISO, aperture and exposure values
     * (as set before starting computing sequence) */
    c->setCurrentISO(startParam.ISO);
    c->setCurrentAperture(startParam.aperture);
    c->setCurrentExposure(startParam.exposure);
}

/*! \brief Stop analysis
 */
void Sequence::computeSequenceAborted()
{
    /* Stop any sequence computing */
    abortComputing();
    /* Reset camera parameters to those selected */
    resetParams();

    computeSequenceDialog.close();
}

/*! \brief Accept analysis, start capture
 */
void Sequence::sequenceAccepted()
{
    previewSequenceDialog.close();
    previewSequenceDialog.stopCountdown();

    resetParams();

    captureSequenceDialog.show();
    emit startSequenceCapture();
}

/*! \brief Reject analysis
 */
void Sequence::sequenceRejected()
{
    previewSequenceDialog.close();
    previewSequenceDialog.stopCountdown();

    resetParams();
}

/*! \brief Manage sequence dialogs
 *
 * Show, update or close sequence dialogs
 * according to state machine
 */
void Sequence::manageSequenceUI()
{
    static QString startExposure;

    switch (state) {
    case CS_START:
        /* set current ISO and aperture
         * in case operator changed them on camera */
        resetParams();
        /* Get initial exposure */
        startExposure = c->getCurrentExposure();
        /* Display computing dialog */
        computeSequenceDialog.show();
        computeSequenceDialog.updateStatus("Computing sequence...");
        computeSequenceDialog.setProgress(10);
        break;

    case CS_LOWER_CRITERIA_FOUND:
        computeSequenceDialog.updateStatus("Found lower criteria.");
        computeSequenceDialog.setProgress(50);
        /* Back to initial parameters */
        resetParams();
        break;

    case CS_UPPER_CRITERIA_FOUND:
        /* Display results dialog */
        computeSequenceDialog.close();
        previewSequenceDialog.show();
        previewSequenceDialog.setLabelResults(
            QString("Measurements completed ! Capture will take ") +
            QString::number(getShotsNb()) + QString(" shot(s)."));
        previewSequenceDialog.setLowerPreview(shots.first().preview);
        previewSequenceDialog.setUpperPreview(shots.last().preview);
        previewSequenceDialog.startCountdown();
        break;

    default:
        break;
    }
}

/*! \brief Manage sequence error
 *
 * Show a MessageBox with error diagnostic
 */
void Sequence::manageSequenceError(QString msg)
{
    computeSequenceDialog.close();
    resetParams();

    QMessageBox::critical(this, "Error", msg);
}

/*! \brief Manage capture progress
 *
 * Show, update or close sequence dialogs
 * according to capture progress
 */
void Sequence::handleCaptureProgress(int nbImg, int total)
{
    if (nbImg == total) {
        /* Finished */
        captureSequenceDialog.close();
        /* Display end dialog */
        captureEndDialog.show();
        captureEndDialog.setStatus(QString::number(total) +
                                   " images successfully captured.");
        resetParams();
    } else {
        captureSequenceDialog.setProgress(100 * nbImg / total);
        captureSequenceDialog.setStatus("Took shot " + QString::number(nbImg) +
                                        " of " + QString::number(total));
    }
}

/*! \brief Manage capture abort
 *
 * Stops CaptureWorker thread
 */
void Sequence::captureSequenceAborted()
{
    /* Abort capture thread */
    captureWorker->setCaptureRunState(false);

    resetParams();

    captureSequenceDialog.close();
}

/*! \brief Manage capture error
 */
void Sequence::captureSequenceError()
{
    resetParams();

    captureSequenceDialog.close();

    QMessageBox::critical(this, "Error", "Sequence capture failed");
}
