#ifndef SEQUENCE__H
#define SEQUENCE__H

#include "autohdr_captureend.h"
#include "autohdr_capturesequence.h"
#include "autohdr_computesequence.h"
#include "autohdr_previewsequence.h"
#include "camera.h"
#include "config.h"
#include <QImage>
#include <QList>
#include <QString>
#include <QThread>
#include <QWidget>

class CaptureWorker;
class Composition;

typedef struct {
    QString ISO;
    QString aperture;
    QString exposure;
    QImage preview;
    QString path;
} shotParameters;

enum SequenceState {
    CS_IDLE = 0,
    CS_START,
    CS_LOWER_CRITERIA_SEEKING,
    CS_LOWER_CRITERIA_FOUND,
    CS_UPPER_CRITERIA_SEEKING,
    CS_UPPER_CRITERIA_FOUND
};

class Sequence : public QWidget
{
    Q_OBJECT

  public:
    explicit Sequence(RemoteCamera *cam = nullptr, Config *conf = nullptr,
                      QWidget *parent = nullptr);
    ~Sequence();

    /* Getters */
    void getCriterias(int *lower, int *upper, int *nb);
    int getShotsNb();
    shotParameters getShotParameters(int n);

    /* Setters */
    void setCriterias(int lower, int upper, int nb);
    void setStartParameters(QString ISO, QString ap, QString exp);
    void setShot(shotParameters sp);
    void setShotPath(int n, const QString path);

    void clearSequence();

  signals:
    void startSequenceCapture();

  public slots:
    void startComputing();
    void abortComputing();
    void runStateMachine(QImage *image);

    void sequenceAccepted();
    void sequenceRejected();
    void computeSequenceAborted();
    void handleCaptureProgress(int nbImg, int total);
    void captureSequenceAborted();
    void captureSequenceError();

  private:
    RemoteCamera *c;
    Config *config;
    SequenceState state;
    QImage currentView;
    shotParameters startParam;
    /* Criterias */
    int overExpCrit;  /* Overexposition criteria in less exposed shot */
    int underExpCrit; /* Underexposition criteria in most exposed shot */
    int nbImgMax;
    /* Results */
    QList<shotParameters> shots;
    /* Composition */
    Composition *comp;

    /* Final shots capture */
    QThread sequenceAcquisition;
    CaptureWorker *captureWorker;

    /* Dialogs */
    AutoHDR_ComputeSequence computeSequenceDialog;
    AutoHDR_PreviewSequence previewSequenceDialog;
    AutoHDR_CaptureSequence captureSequenceDialog;
    AutoHDR_CaptureEnd captureEndDialog;

    void resetParams();
    void manageSequenceUI();
    void manageSequenceError(QString msg);
    void setSequenceBoundary();
    void distributeShots(int stops);
};

#endif // SEQUENCE__H
