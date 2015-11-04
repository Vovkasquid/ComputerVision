#-------------------------------------------------
#
# Project created by QtCreator 2014-12-05T15:59:39
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = ComputerVision
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra -pedantic

SOURCES += main.cpp \
    ../libs/sockethandler.cpp \
    ../libs/parserxml.cpp \
    ../libs/log.cpp

HEADERS += ../libs/log.h \
    ../libs/parserxml.h \
    ../libs/sockethandler.h

LIBS += `pkg-config opencv --libs`
