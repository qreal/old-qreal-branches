QRXML_ROOT = ..

!include (../editorsSdk.pri):message("editorsSdk.pri not found at ..")

QT += xml

TEMPLATE = lib
CONFIG += plugin
DESTDIR = ../../qrgui/plugins/
INCLUDEPATH += ..

OBJECTS_DIR = ../obj/
MOC_DIR = ../moc/

SOURCES += handmadeEditor.cpp \
	kroki.cpp \

HEADERS += handmadeEditor.h \
	kroki.h \
	actor.h \
	connection.h

RESOURCES += shapes/handmadeEditor.qrc
