QT += xml
TEMPLATE =  lib
CONFIG += plugin
DESTDIR = ../../qrgui/plugins/
MOC_DIR = .moc
RCC_DIR = .moc

win32 | win32-msvc* {
        CONFIG(debug, debug|release) {
                QRXC = ..\..\qrxc\debug\qrxc.exe
        } else:CONFIG(release, debug|release){
                QRXC = ..\..\qrxc\release\qrxc.exe
        } else {
                error(Windows build definitely needs to be fixed)
        }
} else {
        QRXC = ../../qrxc/qrxc
}

!exists($$QREAL_EDITOR_NAME$$quote(\generated\pluginInterface.h)) {
	COMMAND = cd $$QREAL_EDITOR_NAME && $$QRXC $$QREAL_XML
	SYS = $$system($$COMMAND) 
}

include(extraCompilers.pri)

SOURCES		+= \
			../../qrgui/umllib/sdfrenderer.cpp \
			../../qrgui/umllib/uml_nodeelement.cpp \
			../../qrgui/umllib/uml_edgeelement.cpp \
			../../qrgui/umllib/uml_element.cpp \
			../../qrgui/kernel/ids.cpp \

HEADERS		+= \
			../../qrgui/umllib/sdfrenderer.h \
			../../qrgui/umllib/uml_nodeelement.h \
			../../qrgui/umllib/uml_edgeelement.h \
			../../qrgui/umllib/uml_element.h \
			../../qrgui/kernel/ids.h \
			../editorinterface.h \


