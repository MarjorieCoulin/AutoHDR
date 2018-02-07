#ifndef AUTOHDR_MAINWINDOW_H
#define AUTOHDR_MAINWINDOW_H
#include "camera.h"
#include "config.h"
#include "liveviewworker.h"
#include "sequence.h"
#include <QMainWindow>
#include <QThread>

namespace Ui
{
class AutoHDR_MainWindow;
}

class AutoHDR_MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit AutoHDR_MainWindow(QWidget *parent = 0);
    ~AutoHDR_MainWindow();

  signals:
    void displayLiveView();
    void startSequenceComputing();
    void startSequenceCapture();

  public slots:
    void cameraConnected();
    void handleLiveView(QImage *image);

  private slots:
    /* Parameters */
    void handleLowerCriteria();
    void handleUpperCriteria();
    void handleNbImgMax();
    void handleStart();
    void handleISO(QString selectedISO);
    void handleAperture(QString selectedAperture);
    void handleExposure(QString selectedExposure);
    /* Menu */
    void on_actionConfiguration_load_triggered();
    void on_actionSequence_load_triggered();
    void on_actionSequence_save_triggered();

  private:
    Ui::AutoHDR_MainWindow *ui;
    RemoteCamera *c;
    Sequence *s;
    Config *conf;
    /* Continuous live shot for display */
    QThread liveViewAcquisition;
    LiveViewWorker *liveViewWorker;

    void enableUI();
    void setCapabilities();
    void setDefaultParams();
};

#endif // AUTOHDR_MAINWINDOW_H
