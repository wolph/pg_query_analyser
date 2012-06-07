QT += sql
QT -= gui
TARGET = pg_query_analyser

target.path = /usr/bin
INSTALLS += target

templates.path = /usr/share/pg_query_analyser/templates
templates.files = templates/*
INSTALLS += templates

static.path = /usr/share/pg_query_analyser/static
static.files = static/*
INSTALLS += static

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
OTHER_FILES += templates/header.html \
    templates/footer.html

RESOURCES = pg_query_analyser.qrc

