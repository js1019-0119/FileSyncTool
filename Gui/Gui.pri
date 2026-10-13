#ui层

INCLUDEPATH += $$PWD/Catalogue
INCLUDEPATH += $$PWD/ScanDifft
INCLUDEPATH += $$PWD/Task
INCLUDEPATH += $$PWD/Log
INCLUDEPATH += $$PWD/ProgressBar

include($$PWD/Catalogue/Catalogue.pri)
include($$PWD/ScanDifft/ScanDifft.pri)
include($$PWD/Task/Task.pri)
include($$PWD/Log/Log.pri)
include($$PWD/ProgressBar/ProgressBar.pri)


FORMS += \
    $$PWD/mainwindow.ui

HEADERS += \
    $$PWD/mainwindow.h \
    $$PWD/mainwindowuidata.h \
    $$PWD/badgedelegate.h \
    $$PWD/scansettingsdialog.h \
    $$PWD/syncmodedialog.h

SOURCES += \
    $$PWD/mainwindow.cpp \
    $$PWD/badgedelegate.cpp \
    $$PWD/scansettingsdialog.cpp \
    $$PWD/syncmodedialog.cpp
