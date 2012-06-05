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
OTHER_FILES += templates/header.html \
    templates/footer.html
