#-------------------------------------------------
#
# Project created by QtCreator 2013-02-13T11:23:25
#
#-------------------------------------------------

QT       -= core gui

TARGET = eveH5
TEMPLATE = lib

VERSION += 6.0
DEFINES += EVEH5_LIBRARY

CONFIG += dll

QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    IFile.cpp \
    IData.cpp \
    IH5File.cpp \
    IMetaData.cpp \
    ih5filev2.cpp \
    ih5filev3.cpp \
    ih5filev4.cpp \
    ih5filev5.cpp \
    attributemetadata.cpp \
    ifilemetadata.cpp \
    ichainmetadata.cpp

HEADERS += \
    eve.h \
    IFile.h \
    IData.h \
    IH5File.h \
    IMetaData.h \
    ih5filev2.h \
    ih5filev3.h \
    ih5filev4.h \
    ih5filev5.h \
    attributemetadata.h \
    ifilemetadata.h \
    ichainmetadata.h

#linux-g++-32 {
#    LIBS +=  -L/home/eden/src/hdf5/hdf5-1.8.9/hdf5/lib-static
#}

linux-g++-64 {
    LIBS +=  -L/home/eden/src/hdf5/hdf5-1.10.1-gcc7/hdf5/lib64
}

unix:INCLUDEPATH += /home/eden/src/hdf5/hdf5-1.10.1-gcc7/hdf5/include

LIBS +=  -l:libhdf5_cpp.a -l:libhdf5.a -lz


















