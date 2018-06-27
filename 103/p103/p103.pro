include( $(VISMON_SRC)/lib.pri )
TARGET = p103
DEFINES+= P103_DLL
LIBS += -lpnet103 -ldboo -ldbmetaapi -lpromgr -lsysapi \
        -lrtdb -lpromonitorapi -lbasetool -lipcapi\
        -lscadaapi
QT+= xml sql network

SOURCES += \
    pro103app.cpp \
    edevice.cpp \
    pro103appprivate.cpp \
    deviceprotocol.cpp \
    gencommand.cpp \
    commandhandler.cpp \
    main.cpp \
    ../../license/licapi/licapp.cpp

HEADERS += \
			protocol_define.h \
    pro103app.h \
    ../../include/comm/p103.h \
    ../../include/comm/p103app.h \
    edevice.h \
    pro103appprivate.h \
    deviceprotocol.h \
    gencommand.h \
    commandhandler.h

