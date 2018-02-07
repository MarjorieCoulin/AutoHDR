#include "composition.h"
#include <QMessageBox>

Composition::Composition(Sequence *seq, Config *config, QObject *parent)
    : QObject(parent)
{
    s = seq;
    conf = config;
    p = new QProcess(this);

    /* Link with process */
    connect(p, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(handleError(QProcess::ProcessError)));
    connect(p, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(handleFinished(int, QProcess::ExitStatus)));

    /* Link with compose dialog */
    connect(&composeDialog, &AutoHDR_Compose::abortCompose, this,
            &Composition::abortComposition);
}

/*! \brief Composition destructor
 *
 * Kills any ongoing composition process
 */
Composition::~Composition()
{
    p->kill();

    delete p;
}

/*! \brief Start composition
 *
 * Create a luminance-hdr-cli command line process
 * and executes it
 *
 * Saves HDR and LDR files accordingly to config
 */
void Composition::startComposition()
{
    QString program = "luminance-hdr-cli";
    QStringList arguments;
    int n = s->getShotsNb();

    arguments << "--save" << conf->getCompFolder() + "hdr_result.tif";
    arguments << "--output" << conf->getCompFolder() + "ldr_result.tif";
    for (int i = 0; i < n; i++)
        arguments << s->getShotParameters(i).path;

    p->start(program, arguments);

    composeDialog.show();
}

/*! \brief Stop composition
 *
 * Abort luminance-hdr-cli process
 */
void Composition::abortComposition()
{
    composeDialog.close();
    p->kill();
}

/*! \brief Handle composition error
 *
 * Gets the luminance-hdr-cli return error value
 * and shows an adapted error message
 */
void Composition::handleError(QProcess::ProcessError error)
{
    composeDialog.close();

    switch (error) {
    case QProcess::FailedToStart:
        QMessageBox::critical(&composeDialog, "Error",
                              "Failed to start HDR composition");
        break;
    case QProcess::Crashed:
        QMessageBox::critical(&composeDialog, "Error",
                              "HDR composition crashed");
        break;
    default:
        QMessageBox::critical(&composeDialog, "Error", "HDR composition error");
        break;
    }
}

/*! \brief Handle composition success
 */
void Composition::handleFinished(int code, QProcess::ExitStatus status)
{
    composeDialog.close();

    if (status == QProcess::NormalExit) {
        QString statusStr =
            "HDR composition ended with code " + QString::number(code);
        QMessageBox::information(&composeDialog, "Information", statusStr);
    }
}
