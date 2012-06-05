# -------------------------------------------------
# Project created by QtCreator 2009-10-15T11:52:02
# -------------------------------------------------
QT += sql
QT -= gui
TARGET = pg_query_analyser
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    query.cpp \
    arg.cpp \
    args.cpp \
    queries.cpp
HEADERS += \
    query.h \
    main.h \
    args.h \
    arg.h \
    queries.h
OTHER_FILES += header.html \
    footer.html

unix|win32: LIBS += -lpcrecpp
