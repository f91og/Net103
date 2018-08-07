include( $(IMOON_SRC)/lib.pri )
TARGET = pnet103
DEFINES+= PNET103_DLL
QT += network
LIBS += -lbasetool -lpromonitorapi
HEADERS += \
    pnet103app.h \
    ../../include/comm/pnet103.h \
    ../../include/comm/pnet103app.h \
    clientcenter.h \
    tcpsocket.h \
    gateway.h \
    device.h \
    netpacket.h

SOURCES += \
    pnet103app.cpp \
    clientcenter.cpp \
    tcpsocket.cpp \
    gateway.cpp \
    device.cpp \
    netpacket.cpp
