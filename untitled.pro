#-------------------------------------------------
#
# Project created by QtCreator 2022-07-19T09:30:39
#
#-------------------------------------------------

QT       += core gui
#QT       += multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = untitled
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
        latebindingsymboltable_linux.cc \
        pulseaudiosymboltable_linux.cc \
        alsasymboltable_linux.cc \
        audio_mixer_manager_alsa_linux.cc \
        audio_device_alsa_linux.cc \

HEADERS += \
        mainwindow.h \
        latebindingsymboltable_linux.h \
        pulseaudiosymboltable_linux.h \
        alsasymboltable_linux.h \
        audio_mixer_manager_alsa_linux.h \
        audio_device_alsa_linux.h \

FORMS += \
        mainwindow.ui


#LIBS += -L/lib/aarch64-linux-gun -ldl
LIBS += -L/lib/x86_64-linux-gun -ldl

