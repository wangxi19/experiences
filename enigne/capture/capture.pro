TEMPLATE = libs
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
    $$PWD/pcapcapture.h

SOURCES += \
    $$PWD/pcapcapture.cpp

INCLUDEPATH += /usr/local/include/pcap/
INCLUDEPATH += /root/Documents/Public/include/
LIBS += -L/usr/lib64/ -ldbus-1 -L/usr/local/lib/ -l:libpcap.a -L/root/Documents/Public/lib/ -l:libPacketDecoder.so
