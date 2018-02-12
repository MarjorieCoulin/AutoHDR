#include "config.h"
#include "sequence.h"
#include <QDir>
#include <QDomDocument>
#include <QMessageBox>
#include <QTextStream>

/*! \brief Config constructor
 *
 * Sets default general config
 */
Config::Config(QWidget *parent) : QWidget(parent)
{
    /* Default libghoto2 config */
    gpConfig = {"iso", "aperture", "shutterspeed"};
    /* Shots capture at executable level by default */
    captureFolder = QDir::currentPath() + "/";
    compFolder = QDir::currentPath() + "/";
    /* Default thresholds */
    whiteThreshold = 254;
    blackThreshold = 5;
    shotsGap = 6;
}

/*! \brief Load general config
 *
 * Open and parse config file
 * Default path is a "autohdr_config.xml" file at executable level
 */
void Config::load(QString configPath)
{
    QDomDocument config;
    QFile configFile(configPath);

    if (!configFile.open(QIODevice::ReadOnly)) {
        /* No config, use default */
        fprintf(stdout, "[Config] No configuration file\n");
        return;
    }
    if (!config.setContent(&configFile)) {
        /* Corrupted config, use default */
        QMessageBox::critical(this, "Error", "Could not read config file");
        configFile.close();
        return;
    }

    QDomElement root = config.documentElement();
    if (root.tagName() != "autohdr_config") {
        /* Cannot find XML root node */
        QMessageBox::critical(this, "Error", "Invalid config file root node");
        configFile.close();
        return;
    }

    /* Find elements in config file */
    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            if (e.tagName() == "camera") {
                gpConfig.ISO = e.attribute("key_iso", "iso");
                gpConfig.aperture = e.attribute("key_ap", "aperture");
                gpConfig.exposure = e.attribute("key_exp", "shutterspeed");
            }
            if (e.tagName() == "analysis") {
                QString wth = e.attribute("white_threshold", "254");
                QString bth = e.attribute("black_threshold", "5");
                whiteThreshold = wth.toInt();
                blackThreshold = bth.toInt();
                QString ev_gap = e.attribute("ev_gap", "2");
                QString ev_exp = e.attribute("ev_exp", "3");
                shotsGap = ev_gap.toInt() * ev_exp.toInt();
            }
            if (e.tagName() == "capture") {
                captureFolder = e.attribute("folder", "default");
                if (captureFolder == "default")
                    captureFolder = QDir::currentPath() + "/";
            }
            if (e.tagName() == "composition") {
                compFolder = e.attribute("folder", "default");
                if (compFolder == "default")
                    compFolder = QDir::currentPath() + "/";
            }
        }
        node = node.nextSibling();
    }

    fprintf(stdout, "[Config] Current AutoHDR configuration :\n");
    fprintf(stdout, "\tAnalysis thresholds : white %d, black %d\n",
            whiteThreshold, blackThreshold);
    fprintf(stdout, "\tGap between shots : %d\n", shotsGap);
    fprintf(stdout, "\tCapture folder : %s\n",
            captureFolder.toStdString().c_str());
    fprintf(stdout, "\tComposition folder : %s\n",
            compFolder.toStdString().c_str());
}

/*! \brief Load sequence file
 *
 * Open and parse file (no default path)
 * Set attributes to sequence (criterias, shots)
 * If all went well, start sequence capture automatically
 */
void Config::loadSequence(QString sequencePath, Sequence *s)
{
    QDomDocument config;
    QFile configFile(sequencePath);

    if (!configFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open sequence file");
        return;
    }
    if (!config.setContent(&configFile)) {
        /* Corrupted file */
        QMessageBox::critical(this, "Error", "Could not read sequence file");
        configFile.close();
        return;
    }

    QDomElement root = config.documentElement();
    if (root.tagName() != "autohdr_sequence") {
        /* Cannot find XML root node */
        QMessageBox::critical(this, "Error", "Invalid sequence file root node");
        configFile.close();
        return;
    }

    /* Find elements in config file */
    s->clearSequence();
    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            if (e.tagName() == "criterias") {
                s->setCriterias(e.attribute("lower").toInt(),
                                e.attribute("upper").toInt(),
                                e.attribute("max_nb").toInt());
            }
            if (e.tagName() == "sequence") {
                QDomNode s_node = node.firstChild();
                while (!s_node.isNull()) {
                    QDomElement s_e = s_node.toElement();
                    if (!s_e.isNull()) {
                        if (s_e.tagName() == "shot") {
                            shotParameters sp = {
                                .ISO = s_e.attribute("ISO"),
                                .aperture = s_e.attribute("aperture"),
                                .exposure = s_e.attribute("exposure"),
                                .preview = QImage(),
                                .path = QString()};
                            s->setShot(sp);
                        }
                    }
                    s_node = s_node.nextSibling();
                }
            }
        }
        node = node.nextSibling();
    }
}

/*! \brief Save sequence file
 *
 * Save criterias and analysis (distributed shots) to a XML file
 */
void Config::saveSequence(QString sequencePath, Sequence *s)
{
    /* Create file */
    QFile sequenceFile(sequencePath);
    if (!sequenceFile.open(QIODevice::WriteOnly)) {
        sequenceFile.close();
        QMessageBox::critical(this, "Error", "Unable to write sequence file");
        return;
    }

    /* Create content */
    QDomDocument doc("XML");
    QDomElement root = doc.createElement("autohdr_sequence");
    doc.appendChild(root);

    QDomElement criterias = doc.createElement("criterias");
    int lower, upper, nb;
    s->getCriterias(&lower, &upper, &nb);
    criterias.setAttribute("lower", lower);
    criterias.setAttribute("upper", upper);
    criterias.setAttribute("max_nb", nb);
    root.appendChild(criterias);

    QDomElement sequence = doc.createElement("sequence");
    for (int i = 0; i < s->getShotsNb(); i++) {
        QDomElement shot = doc.createElement("shot");
        shotParameters sp = s->getShotParameters(i);
        shot.setAttribute("ISO", sp.ISO);
        shot.setAttribute("aperture", sp.aperture);
        shot.setAttribute("exposure", sp.exposure);
        sequence.appendChild(shot);
    }
    root.appendChild(sequence);

    /* Fill file */
    QString fileContent = doc.toString();
    QTextStream stream(&sequenceFile);
    stream << fileContent;
    sequenceFile.close();
}

/*! \brief Get ISO camera dictionnary key
 */
QString Config::getISOKey()
{
    return gpConfig.ISO;
}

/*! \brief Get aperture camera dictionnary key
 */
QString Config::getApertureKey()
{
    return gpConfig.aperture;
}

/*! \brief Get exposure camera dictionnary key
 */
QString Config::getExposureKey()
{
    return gpConfig.exposure;
}

/*! \brief Get capture folder
 */
QString Config::getCaptureFolder()
{
    return captureFolder;
}

/*! \brief Get white pixel threshold definition
 *
 * 8 bit value, not far from 0xFF
 */
unsigned char Config::getWhiteThreshold()
{
    return whiteThreshold;
}

/*! \brief Get black pixel threshold definition
 *
 * 8 bit value
 * Greater than zero because of digital noise
 */
unsigned char Config::getBlackThreshold()
{
    return blackThreshold;
}

/*! \brief Get spacing between shots, in stops
 */
unsigned int Config::getShotsGap()
{
    return shotsGap;
}

/*! \brief Get composition folder
 */
QString Config::getCompFolder()
{
    return compFolder;
}

/*! \brief Get file naming for capture
 *
 * Default for now is "Image_<number>"
 */
QString Config::getShotName(int shotNb)
{
    return QString("Image_" + QString::number(shotNb));
}
