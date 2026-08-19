QT       += core gui
QT       += serialport
QT       += charts
QT       += concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
DEFINES += QT_DEPRECATED_WARNINGS

LIBS += -lpsapi

QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

RESOURCES += darkstyle.qrc
RC_ICONS = $$PWD/Resource/NI.ico

SOURCES += \
    GaugeWidget.cpp \
    main.cpp \
    mainwindow.cpp \
    module/wave_view/mwaveview.cpp \
    managers/analoginputmanager.cpp \
    managers/analogoutputmanager.cpp \
    managers/digitalio_manager.cpp \
    managers/canmanager.cpp \
    managers/linmanager.cpp \
    utils/waveformgenerator.cpp

HEADERS += \
    gaugewidget.h \
    mainwindow.h \
    module/wave_view/mwaveview.h \
    include/NIDAQmx.h \
    include/PCANBasic.h \
    managers/analoginputmanager.h \
    managers/analogoutputmanager.h \
    managers/digitalio_manager.h \
    managers/canmanager.h \
    managers/linmanager.h \
    utils/waveformgenerator.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += "C:/pcan-basic/Include"
INCLUDEPATH += "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C/include"
INCLUDEPATH += $$PWD

LIBS += -L$$PWD/lib64/msvc -lNIDAQmx
LIBS += -L$$PWD -lpcanbasic
LIBS += "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C/lib64/msvc/nixnet.lib"


DISTFILES += \
    PCANBasic.dll \
    darkstyle.qrc \
    Resource/NI.ico
