#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QWidget>

class Sequence;

#define CONFIG_FILENAME "autohdr_config.xml"

class Config : public QWidget
{
    Q_OBJECT

  public:
    explicit Config(QWidget *parent = nullptr);

    void load(QString configPath = CONFIG_FILENAME);
    void loadSequence(QString sequencePath, Sequence *s);
    void saveSequence(QString sequencePath, Sequence *s);

    /* Getters */
    QString getISOKey();
    QString getApertureKey();
    QString getExposureKey();
    QString getCaptureFolder();
    unsigned char getWhiteThreshold();
    unsigned char getBlackThreshold();
    unsigned int getShotsGap();
    QString getCompFolder();
    QString getShotName(int shotNb);

  private:
    /* Camera */
    struct {
        QString ISO;
        QString aperture;
        QString exposure;
    } gpConfig;
    /* Analysis */
    unsigned char whiteThreshold;
    unsigned char blackThreshold;
    unsigned int shotsGap;
    /* Capture */
    QString captureFolder;
    /* Composition */
    QString compFolder;
};

#endif // CONFIG_H
