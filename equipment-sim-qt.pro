QT += core gui widgets

CONFIG += c++17

TARGET = equipment_sim_qt
TEMPLATE = app

INCLUDEPATH += include

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/EquipmentWorker.cpp

HEADERS += \
    include/MainWindow.h \
    include/EquipmentWorker.h \
    include/Job.h \
    include/ThreadSafeQueue.h

unix: LIBS += -lpthread
