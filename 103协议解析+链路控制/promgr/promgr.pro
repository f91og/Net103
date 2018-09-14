include( $(VISMON_SRC)/lib.pri )
TARGET = promgr
DEFINES+= PROMGR_DLL
LIBS += -ldboo -lrtdb -ldbmetaapi
QT += xml sql

HEADERS += \
    ../../include/comm/promgr.h \
    promgrapp.h \
    probase.h \
    ../../include/comm/probase.h \
    ../../include/comm/promgrapp.h

SOURCES += \
    promgrapp.cpp \
    probase.cpp
