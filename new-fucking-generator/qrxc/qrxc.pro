include (qmakeQrxc.pri)

QT = core \
     xml \
     gui

CONFIG += console

DESTDIR = $$dirname(QMAKE_QRXC)
OBJECTS_DIR = .obj
MOC_DIR = .moc

HEADERS += generator.h \
    entity.h \
    editor_file.h \
    editor.h \
    non_graph_type.h \
    property.h \
    sdftocpp.h

SOURCES += generator.cpp \
    entity.cpp \
    editor_file.cpp \
    editor.cpp \
    non_graph_type.cpp \
    property.cpp \
    sdftocpp.cpp \
    main.cpp
