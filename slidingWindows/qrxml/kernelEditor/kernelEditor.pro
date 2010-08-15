QREAL_XML = ../commonXml/kernel_metamodel.xml
QREAL_EDITOR_NAME = kernelEditor

include (../editorsCommon.pri)

HEADERS += \
	../../qrgui/editorManager/listener.h

HEADERS += \
	contextMenuActionTrigger.h \
	methodeditordialog.h \

SOURCES += \
	../../qrgui/kernel/ids.cpp \
	contextMenuActionTrigger.cpp \
	methodeditordialog.cpp \

FORMS += \
	methodeditordialog.ui \

