#include "camera.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/*! \brief RemoteCamera constructor
 *
 * Uses reference to instance Config created by
 * AutoHDR_MainWindow
 * Creates a libgphoto2 context
 */
RemoteCamera::RemoteCamera(Config *config, QObject *parent) : QObject(parent)
{
    conf = config;

    /* libGphoto2 context */
    context = gp_context_new();
    /* libGphoto2 camera */
    gp_camera_new(&camera);

    /* Camera connection polling */
    retry = new QTimer(this);
    connect(retry, SIGNAL(timeout()), this, SLOT(connectCamera()));
}

/*! \brief RemoteCamera destructor */
RemoteCamera::~RemoteCamera()
{
    if (cameraConfig.root != NULL)
        gp_widget_free(cameraConfig.root);

    gp_camera_exit(camera, context);
    gp_camera_free(camera);
    gp_context_unref(context);

    delete retry;
}

/*! \brief Camera connection routine
 *
 * Connect camera and load camera capabilities
 */
void RemoteCamera::connectCamera()
{
    retry->stop();

    /* Init camera */
    if (gp_camera_init(camera, context) != GP_OK) {
        /* Retry 2s later */
        retry->start(2000);
        return;
    }

    /* Get config */
    if (initCameraConfig() != GP_OK) {
        /* Retry 2s later */
        retry->start(2000);
        return;
    }

    emit connected();
}

/*! \brief Set current value to camera config
 *
 * Set a string value to a camera config designated by its
 * entry in a dictionnary ad its CameraWidget pointer
 */
int RemoteCamera::setCurrentCameraParam(CameraWidget *config,
                                        const char *configStr,
                                        const char *value)
{
    int ret;

    /* We assume root config and intermediate nodes did not change since init */

    if (!config || !value)
        return GP_ERROR;

    mutex.lock();

    ret = gp_widget_set_value(config, value);
    if (ret < GP_OK) {
        fprintf(stderr, "could not set widget %s with value %s (%d)\n",
                configStr, value, ret);
        goto out;
    }

    ret = gp_camera_set_single_config(camera, configStr, config, context);
    if (ret != GP_OK) {
        /* This stores it on the camera again */
        ret = gp_camera_set_config(camera, cameraConfig.root, context);
        if (ret < GP_OK) {
            fprintf(stderr, "camera_set_config failed: %d\n", ret);
        }
    }

out:
    mutex.unlock();
    return ret;
}

/*! \brief Get current value from camera config
 *
 * Get a string value from a camera config designated by its
 * entry in a dictionnary and its CameraWidget pointer
 */
int RemoteCamera::getCurrentCameraParam(CameraWidget *config,
                                        QString &currentParam)
{
    int ret;
    char *current;

    /* Get current value from config */
    ret = gp_widget_get_value(config, &current);
    if (ret != GP_OK) {
        fprintf(stderr, "Failed to retrieve current value\n");
        return GP_ERROR;
    }
    currentParam = QString(current);
    return GP_OK;
}

/*! \brief Get all possible values from camera config
 *
 * Get a list of values from a camera config designated by its
 * entry in a dictionnary
 */
int RemoteCamera::getCameraConfig(CameraWidget **config, const char *configStr,
                                  QStringList &capabilities)
{
    int ret, choiceCnt;
    CameraWidgetType type;

    /* Find config widget */
    ret = gp_widget_get_child_by_name(cameraConfig.root, configStr, config);
    if (ret < GP_OK)
        ret =
            gp_widget_get_child_by_label(cameraConfig.root, configStr, config);
    if (ret < GP_OK) {
        fprintf(stderr, "Config %s does not exist for this camera\n",
                configStr);
        goto out;
    }

    /* Verify we do get a list */
    ret = gp_widget_get_type(*config, &type);
    if (ret != GP_OK)
        goto out;
    if (type != GP_WIDGET_RADIO) {
        fprintf(stderr, "Expected a list config for %s\n", configStr);
        ret = GP_ERROR;
        goto out;
    }

    capabilities.clear();
    /* Get all possible values from list */
    choiceCnt = gp_widget_count_choices(*config);
    if (!choiceCnt) {
        /* List can be empty. Only choice is then current value */
        QString currentParam;
        getCurrentCameraParam(*config, currentParam);
        capabilities.push_back(currentParam);
    } else
        for (int i = 0; i < choiceCnt; i++) {
            const char *choice;
            ret = gp_widget_get_choice(*config, i, &choice);
            if (ret != GP_OK) {
                fprintf(stderr,
                        "Failed to retrieve all values from radio widget\n");
                goto out;
            }
            capabilities.push_back(QString(choice));
        }

out:
    return ret;
}

/*! \brief Get capabilities for camera ISO, aperture and exposure
 */
int RemoteCamera::initCameraConfig()
{
    int ret;

    /* Get global config */
    mutex.lock();
    ret = gp_camera_get_config(camera, &cameraConfig.root, context);
    mutex.unlock();
    if (ret != GP_OK)
        return ret;

    /* Get all possible ISO values */
    ret = getCameraConfig(&cameraConfig.ISO, conf->getISOKey(),
                          cameraCapabilities.ISO);
    if (ret == GP_OK) {
        ret = getCurrentCameraParam(cameraConfig.ISO, currentParams.ISO);
        if (ret != GP_OK)
            goto out;
    } else
        goto out;

    /* Get all possible aperture values */
    ret = getCameraConfig(&cameraConfig.aperture, conf->getApertureKey(),
                          cameraCapabilities.aperture);
    if (ret == GP_OK) {
        ret = getCurrentCameraParam(cameraConfig.aperture,
                                    currentParams.aperture);
        if (ret != GP_OK)
            goto out;
    } else
        fprintf(stdout, "Camera does not support automatic aperture\n");

    /* Get all possible shutter speeds */
    ret = getCameraConfig(&cameraConfig.exposure, conf->getExposureKey(),
                          cameraCapabilities.exposure);
    if (ret == GP_OK) {
        ret = getCurrentCameraParam(cameraConfig.exposure,
                                    currentParams.exposure);
        if (ret != GP_OK)
            goto out;
    } else
        goto out;

    return ret;
out:
    gp_widget_free(cameraConfig.root);
    return ret;
}

#include <QFileInfo>
/*! \brief Get file suffix in ".<suffix>" format
 */
static QString getFileExtension(char *fileName)
{
    QFileInfo fi = QFileInfo(QString(fileName));
    /* fileName refers to a file on the camera
     * but QFileInfo manages to get suffix ayway */
    return "." + fi.completeSuffix();
}

/*! \brief Take a photo and save it to filesystem
 */
int RemoteCamera::captureShot(QString &capturePath)
{
    int fd, ret;
    CameraFile *file;
    CameraFilePath cam_fp;

    mutex.lock();

    /* Camera file path modified as camera entends it */
    ret = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &cam_fp, context);
    if (ret != GP_OK)
        return ret;

    /* Create captured file */
    capturePath = capturePath + getFileExtension(cam_fp.name);
    fd = open(capturePath.toStdString().c_str(), O_CREAT | O_WRONLY, 0644);
    ret = gp_file_new_from_fd(&file, fd);
    if (ret != GP_OK)
        return ret;

    /* Get file from camera */
    ret = gp_camera_file_get(camera, cam_fp.folder, cam_fp.name,
                             GP_FILE_TYPE_NORMAL, file, context);
    if (ret != GP_OK)
        goto out;

    /* Delete file on camera */
    ret = gp_camera_file_delete(camera, cam_fp.folder, cam_fp.name, context);

    fprintf(stdout, "[Camera] Captured %s\n",
            capturePath.toStdString().c_str());

out:
    mutex.unlock();
    gp_file_free(file);
    return ret;
}

/*! \brief Take a photo preview
 *
 * Photo preview is a smaller picture associated to
 * the camera live view.
 * Preview is saved to liveview.jpg at executable level.
 * It is then read by the liveViewWorker thread.
 */
int RemoteCamera::captureLiveView()
{
    CameraFile *file;
    int fd;
    int ret;

    /* Create file */
    fd = open("liveview.jpg", O_CREAT | O_WRONLY, 0644);
    ret = gp_file_new_from_fd(&file, fd);
    if (ret != GP_OK) {
        fprintf(stderr, "gp_file_new_from_fd failed\n");
        return ret;
    }

    mutex.lock();
    ret = gp_camera_capture_preview(camera, file, context);
    mutex.unlock();
    if (ret < 0) {
        fprintf(stderr, "gp_camera_capture_preview failed\n");
        gp_file_unref(file);
        return ret;
    }

    gp_file_unref(file);
    return ret;
}

/*! \brief Increase exposure by 1 stop
 *
 * Looks in capabilities list for an exposure value greater
 * than current one, then sets it
 * Returns value found
 * Returns an error if no greater exposure exists
 */
int RemoteCamera::increaseExposure(QString &next)
{
    /* Increase exposure by 1 stop */
    int index = cameraCapabilities.exposure.indexOf(currentParams.exposure);
    if (--index < 0)
        return GP_ERROR;
    else {
        next = cameraCapabilities.exposure.at(index);
        if (next == maxExposure)
            return GP_ERROR;
        else
            setCurrentExposure(next);
    }
    return GP_OK;
}

/*! \brief Decrease exposure by 1 stop
 *
 * Looks in capabilities list for an exposure value lower
 * than current one, then sets it
 * Returns value found
 * Returns an error if no lower exposure exists
 */
int RemoteCamera::decreaseExposure(QString &next)
{
    /* Decrease exposure by 1 stop */
    int index = cameraCapabilities.exposure.indexOf(currentParams.exposure);
    if (++index == cameraCapabilities.exposure.size())
        return GP_ERROR;
    else {
        next = cameraCapabilities.exposure.at(index);
        setCurrentExposure(next);
    }
    return GP_OK;
}

/*! \brief Distribute exposure values
 *
 * Distributes exposure values between 2 index in the calabilities list
 * and with a designated spacing.
 * Returns a list of variable length containing distributed values
 */
void RemoteCamera::distributeExposures(QString &first, QString &last,
                                       int spacing, QList<QString> &list)
{
    /* Look for boundaries index */
    int index_first = cameraCapabilities.exposure.indexOf(first);
    int index_last = cameraCapabilities.exposure.indexOf(last);

    /* Distribute exposures between fist and last */
    for (int i = index_first - spacing; i > index_last; i -= spacing)
        list.append(cameraCapabilities.exposure.at(i));
}

/*! \brief Set and applies an ISO value to camera
 */
void RemoteCamera::setCurrentISO(QString &ISO)
{
    if (setCurrentCameraParam(cameraConfig.ISO, "iso",
                              ISO.toStdString().c_str()) == GP_OK)
        currentParams.ISO = ISO;
    else
        fprintf(stderr, "Could not set ISO value\n");
}

/*! \brief Set and applies an aperture value to camera
 */
void RemoteCamera::setCurrentAperture(QString &aperture)
{
    if (setCurrentCameraParam(cameraConfig.aperture, "aperture",
                              aperture.toStdString().c_str()) == GP_OK)
        currentParams.aperture = aperture;
    else
        fprintf(stderr, "Could not set aperture value\n");
}

/*! \brief Set and applies an exposure value to camera
 */
void RemoteCamera::setCurrentExposure(QString &exposure)
{
    if (setCurrentCameraParam(cameraConfig.exposure, "exposure",
                              exposure.toStdString().c_str()) == GP_OK)
        currentParams.exposure = exposure;
    else
        fprintf(stderr, "Could not set exposure value\n");
}

/*! \brief Set max exposure attribute for increaseExposure() use
 */
void RemoteCamera::setMaxExposure(QString exposure)
{
    maxExposure = exposure;
}

/*! \brief Get available ISO capabilities
 */
QStringList RemoteCamera::getCapabilitiesISO()
{
    return cameraCapabilities.ISO;
}
/*! \brief Get available aperture capabilities
 *         (can be void)
 */
QStringList RemoteCamera::getCapabilitiesAperture()
{
    return cameraCapabilities.aperture;
}
/*! \brief Get available exposure capabilities
 */
QStringList RemoteCamera::getCapabilitiesExposure()
{
    return cameraCapabilities.exposure;
}

/*! \brief Get current ISO parameter
 */
QString RemoteCamera::getCurrentISO()
{
    getCurrentCameraParam(cameraConfig.ISO, currentParams.ISO);

    return currentParams.ISO;
}
/*! \brief Get current aperture parameter
 */
QString RemoteCamera::getCurrentAperture()
{
    getCurrentCameraParam(cameraConfig.aperture, currentParams.aperture);

    return currentParams.aperture;
}
/*! \brief Get current exposure parameter
 */
QString RemoteCamera::getCurrentExposure()
{
    getCurrentCameraParam(cameraConfig.exposure, currentParams.exposure);

    return currentParams.exposure;
}
