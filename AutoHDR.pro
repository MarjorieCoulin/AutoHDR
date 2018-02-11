#-------------------------------------------------
#
# Project created by QtCreator 2017-12-28T15:47:09
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AutoHDR
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    main.cpp \
    config.cpp \
    camera.cpp \
    liveview.cpp \
    liveviewworker.cpp \
    sequence.cpp \
    captureworker.cpp \
    composition.cpp \
    autohdr_mainwindow.cpp \
    autohdr_computesequence.cpp \
    autohdr_previewsequence.cpp \
    autohdr_capturesequence.cpp \
    autohdr_captureend.cpp \
    autohdr_compose.cpp

HEADERS += \
    config.h \
    camera.h \
    liveview.h \
    liveviewworker.h \
    sequence.h \
    captureworker.h \
    composition.h \
    autohdr_mainwindow.h \
    autohdr_computesequence.h \
    autohdr_previewsequence.h \
    autohdr_capturesequence.h \
    autohdr_captureend.h \
    autohdr_compose.h

FORMS += \
    autohdr_mainwindow.ui \
    autohdr_computesequence.ui \
    autohdr_previewsequence.ui \
    autohdr_capturesequence.ui \
    autohdr_captureend.ui \
    autohdr_compose.ui

LIBS += -lgphoto2 -lgphoto2_port

RESOURCES += \
    resources.qrc
