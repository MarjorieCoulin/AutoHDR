#include "autohdr_mainwindow.h"
#include "ui_autohdr_mainwindow.h"

/*! \brief AutoHDR_MainWindow constructor
 *
 * Creates singletons of Config, RemoteCamera and Sequence
 * that will be used further
 *
 * Displays main UI
 *
 * Manages a variety of signals :
 * - Camera connection
 * - Sequence computing (button Start)
 * - Sequence capture (after a sequence file is loaded)
 * - LiveView display
 * - Sequence state machine sequencer
 * Manages LiveView thread
 */
AutoHDR_MainWindow::AutoHDR_MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AutoHDR_MainWindow)
{
    ui->setupUi(this);
    ui->menuBar->show();

    conf = new Config();
    c = new RemoteCamera(conf);
    s = new Sequence(c, conf);

    /* UI */
    connect(ui->sliderLowerCriteria, SIGNAL(valueChanged(int)), this,
            SLOT(handleLowerCriteria()));
    connect(ui->sliderUpperCriteria, SIGNAL(valueChanged(int)), this,
            SLOT(handleUpperCriteria()));
    connect(ui->sliderNbImgMax, SIGNAL(valueChanged(int)), this,
            SLOT(handleNbImgMax()));
    connect(ui->buttonStart, SIGNAL(pressed()), this, SLOT(handleStart()));
    connect(ui->boxISO, SIGNAL(activated(QString)), this,
            SLOT(handleISO(QString)));
    connect(ui->boxAperture, SIGNAL(activated(QString)), this,
            SLOT(handleAperture(QString)));
    connect(ui->boxExposure, SIGNAL(activated(QString)), this,
            SLOT(handleExposure(QString)));

    /* Camera connection */
    connect(c, &RemoteCamera::connected, this,
            &AutoHDR_MainWindow::cameraConnected);

    /* Sequence computing */
    connect(this, &AutoHDR_MainWindow::startSequenceComputing, s,
            &Sequence::startComputing);
    /* Sequence capture */
    connect(this, &AutoHDR_MainWindow::startSequenceCapture, s,
            &Sequence::sequenceAccepted);

    /* Live View thread */
    liveViewWorker = new LiveViewWorker();
    liveViewWorker->setCamera(c);
    liveViewWorker->moveToThread(&liveViewAcquisition);
    connect(&liveViewAcquisition, &QThread::finished, liveViewWorker,
            &QObject::deleteLater);
    connect(this, &AutoHDR_MainWindow::displayLiveView, liveViewWorker,
            &LiveViewWorker::captureLiveView);
    connect(liveViewWorker, &LiveViewWorker::imageReady, this,
            &AutoHDR_MainWindow::handleLiveView);
    liveViewAcquisition.start();

    /* State machine is sequenced by liveview acquisition */
    connect(liveViewWorker, &LiveViewWorker::imageReady, s,
            &Sequence::runStateMachine);

    /* Load default config */
    conf->load();
    /* Try to connect immediately */
    statusBar()->showMessage("Connecting...");
    c->connectCamera();
}

/*! \brief AutoHDR_MainWindow destructor
 *
 * Deletes instances of Config, RemoteCamera and Sequence
 * Stops LiveView thread
 */
AutoHDR_MainWindow::~AutoHDR_MainWindow()
{
    /* Stop live view */
    liveViewWorker->setLiveViewRunState(false);
    liveViewAcquisition.quit();
    liveViewAcquisition.wait();

    delete conf;
    delete c;
    delete s;
    delete ui;
}

/*! \brief Camera connection signal reception
 *
 * Sets UI as active
 * Loads capabilities into combobox
 * Sets combobox values to current parameters
 * Start liveview
 */
void AutoHDR_MainWindow::cameraConnected()
{
    statusBar()->showMessage("Camera ready.");
    enableUI();

    /* Set capabilities to UI */
    setCapabilities();
    /* Set default values to UI */
    setDefaultParams();

    /* Start liveview right away */
    emit displayLiveView();
}

void AutoHDR_MainWindow::enableUI()
{
    ui->sliderLowerCriteria->setEnabled(true);
    ui->sliderUpperCriteria->setEnabled(true);
    ui->sliderNbImgMax->setEnabled(true);
    ui->buttonStart->setEnabled(true);
    ui->actionSequence_load->setEnabled(true);
}

/*! \brief Clean ISO values list
 *
 * Some elements retrieved from capabilities can have
 * unusable values (ex "Auto", "Max", High1")
 * Removes then and let only numerical values
 */
static QStringList cleanISOCapabilities(QStringList capabilitiesISO)
{
    QStringList l;

    /* Remove elements containing anything other than numbers */
    for (int i = 0; i < capabilitiesISO.size(); i++) {
        QString ISOValue = capabilitiesISO.at(i);
        if (ISOValue.toInt())
            l.push_back(ISOValue);
    }
    return l;
}

/*! \brief Fill combobox with capabilities
 */
void AutoHDR_MainWindow::setCapabilities()
{
    ui->boxISO->clear();
    ui->boxAperture->clear();
    ui->boxExposure->clear();
    ui->boxMaxExposure->clear();

    QStringList capabilitiesISO = c->getCapabilitiesISO();
    /* Clean ISO list from weird elements */
    capabilitiesISO = cleanISOCapabilities(capabilitiesISO);
    ui->boxISO->addItems(capabilitiesISO);
    ui->boxISO->setEnabled(true);

    if (c->getCapabilitiesAperture().empty())
        /* Lens can be manual */
        ui->boxAperture->setEnabled(false);
    else {
        ui->boxAperture->addItems(c->getCapabilitiesAperture());
        ui->boxAperture->setEnabled(true);
    }

    ui->boxExposure->addItems(c->getCapabilitiesExposure());
    ui->boxMaxExposure->addItems(c->getCapabilitiesExposure());
    ui->boxExposure->setEnabled(true);
    ui->boxMaxExposure->setEnabled(true);
}

/*! \brief Set combobox to current camera parameters
 */
void AutoHDR_MainWindow::setDefaultParams()
{
    int i = ui->boxISO->findText(c->getCurrentISO());
    if (i >= 0)
        ui->boxISO->setCurrentIndex(i);

    i = ui->boxAperture->findText(c->getCurrentAperture());
    if (i >= 0)
        ui->boxAperture->setCurrentIndex(i);

    i = ui->boxExposure->findText(c->getCurrentExposure());
    if (i >= 0)
        ui->boxExposure->setCurrentIndex(i);
}

void AutoHDR_MainWindow::handleLowerCriteria()
{
    int v = ui->sliderLowerCriteria->value();

    ui->txtLowerCriteria->setText(QString::number(v) + "%");
}

void AutoHDR_MainWindow::handleUpperCriteria()
{
    int v = ui->sliderUpperCriteria->value();

    ui->txtUpperCriteria->setText(QString::number(v) + "%");
}

void AutoHDR_MainWindow::handleNbImgMax()
{
    int v = ui->sliderNbImgMax->value();

    ui->txtNbImgMax->setText(QString::number(v));
}

/*! \brief Apply new value from ISO combobox to camera
 */
void AutoHDR_MainWindow::handleISO(QString selectedISO)
{
    c->setCurrentISO(selectedISO);
}

/*! \brief Apply new value from aperture combobox to camera
 */
void AutoHDR_MainWindow::handleAperture(QString selectedAperture)
{
    c->setCurrentAperture(selectedAperture);
}

/*! \brief Apply new value from exposure combobox to camera
 */
void AutoHDR_MainWindow::handleExposure(QString selectedExposure)
{
    c->setCurrentExposure(selectedExposure);
}

/*! \brief Start sequence analysis
 *
 * Retrieves analysis parameters from UI
 * and notifies Sequence
 */
void AutoHDR_MainWindow::handleStart()
{
    /* Set search criterias for sequence computing */
    s->setCriterias(ui->sliderLowerCriteria->value(),
                    ui->sliderUpperCriteria->value(),
                    ui->sliderNbImgMax->value());
    /* Set current parameters for reset */
    s->setStartParameters(ui->boxISO->currentText(),
                          ui->boxAperture->currentText(),
                          ui->boxExposure->currentText());
    /* Set exposure beyond which sequence computing
     * won't be able to go */
    c->setMaxExposure(ui->boxExposure->currentText());

    /* Enable sequence save */
    ui->actionSequence_save->setEnabled(true);

    /* Start sequence computing */
    emit startSequenceComputing();
}

/*! \brief Receive liveview image and apply to widget
 */
void AutoHDR_MainWindow::handleLiveView(QImage *image)
{
    ui->liveViewDisplay->setImage(image);
}

#include <QFileDialog>
/*! \brief Load general configuration file
 *
 * From File > Load > Configuration file menu
 */
void AutoHDR_MainWindow::on_actionConfiguration_load_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open configuration file"), "/home/");

    conf->load(fileName);
    /* Reload camera config */
    c->initCameraConfig();
    setCapabilities();
    setDefaultParams();
}

/*! \brief Load sequence file
 *
 * From File > Load > Sequence file menu
 */
void AutoHDR_MainWindow::on_actionSequence_load_triggered()
{
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open sequence file"), "/home/");

    if (!fileName.isEmpty()) {
        /* Load criterias and shots */
        conf->loadSequence(fileName, s);
        /* Set criterias to UI */
        int lower, upper, maxNb;
        s->getCriterias(&lower, &upper, &maxNb);
        ui->sliderLowerCriteria->setValue(lower);
        ui->sliderUpperCriteria->setValue(upper);
        ui->sliderNbImgMax->setValue(maxNb);
        /* Start capture */
        emit startSequenceCapture();
    }
}

/*! \brief Save sequence file
 *
 * From File > Save > Sequence file menu
 */
void AutoHDR_MainWindow::on_actionSequence_save_triggered()
{
    QString fileName =
        QFileDialog::getSaveFileName(this, tr("Save sequence file"), "/home/");

    if (!fileName.isEmpty()) {
        conf->saveSequence(fileName, s);
    }
}
