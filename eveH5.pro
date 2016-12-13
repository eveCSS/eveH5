#-------------------------------------------------
#
# Project created by QtCreator 2013-02-13T11:23:25
#
#-------------------------------------------------

QT       -= core gui

TARGET = eveH5
TEMPLATE = lib

VERSION += 3.0
DEFINES += EVEH5_LIBRARY

CONFIG += dll

SOURCES += \
    IEveDataInfo.cpp \
    IEveData.cpp \
    IEveH5File.cpp \
    IEveJoinData.cpp

HEADERS += \
    IEveDataInfo.h \
    IEveData.h \
    IEveH5File.h \
    IEveJoinData.h \
    eveH5.h

#linux-g++-32 {
#    LIBS +=  -L/home/eden/src/hdf5/hdf5-1.8.9/hdf5/lib-static
#}

linux-g++-64 {
    LIBS +=  -L/home/eden/src/hdf5/hdf5-1.8.17/hdf5/lib64
}

unix:INCLUDEPATH += /home/eden/src/hdf5/hdf5-1.8.17/hdf5/include

LIBS +=  -l:libhdf5_cpp.a -l:libhdf5.a -lz


















