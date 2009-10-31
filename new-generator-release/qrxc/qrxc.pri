include (qmakeQrxc.pri)

CONFIG(debug, debug|release) {
	QR_PLUGIN_CONFIG = CONFIG+=debug
} else:CONFIG(release, debug|release){
	QR_PLUGIN_CONFIG = CONFIG+=release
} else {
	error(CONFIG is wrong, can not determine build configuration for plugins)
}

qrxc.name = QRXC
qrxc.depends = ../pluginTemplate.cpp $$QMAKE_QRXC $$QREAL_XML_DEPENDS
qrxc.commands = $$QMAKE_QRXC ${QMAKE_FILE_IN} generated/${QMAKE_FILE_IN_BASE}.pro 
qrxc.input = QREAL_XML
qrxc.output = generated/${QMAKE_FILE_IN_BASE}.pro
qrxc.variable_out = QRXC_PROJECTS

project_compiler.name = QRXC Project compiler
project_compiler.commands = cd generated && $(QMAKE) $$QR_PLUGIN_CONFIG ${QMAKE_FILE_IN_BASE}.pro && $(MAKE)
project_compiler.input = QRXC_PROJECTS
project_compiler.output = generated/Makefile

QMAKE_EXTRA_COMPILERS *= qrxc project_compiler

# Хак - qmake генерит мэйкфайл, который всегда пытается собрать TARGET, для
# чего вызывает линковщик. Проекты генерируемых редакторов собираются в сгенерированные 
# проекты, и линковать их бессмысленно. Убедить qmake не вызывать линкер не удалось,
# так что мы подсовываем ему в качестве линковщика наш генератор с параметром
# -fake_linker, который просто возвращает 0.
QMAKE_LINK = $$QMAKE_QRXC
QMAKE_LFLAGS = -fake_linker
CONFIG -= embed_manifest_exe

# При операции clean должны удаляться все сгенерённые файлы
QMAKE_CLEAN += generated/*
