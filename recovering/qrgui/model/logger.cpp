#include "model.h"
#include "logger.h"

#include <QDebug>

using namespace qReal;
using namespace model;

Logger::Logger(Model *model)
	: mModel(model), enabled(false)
{
	flagsEnabled[editors] = true;
	flagsEnabled[diagrams] = true;
	flagsEnabled[uselessMessages] = false;
	flagsEnabled[invalidMessages] = false;
}

Logger::~Logger()
{
	foreach(Id diagram, cleanDiagrams)
		remove(diagram);
}

void Logger::enable()
{
	enabled = true;
}

void Logger::disable()
{
	enabled = false;
}

void Logger::setFlag(flag name, bool arg)
{
	flagsEnabled[name] = arg;
}

void Logger::log(action performed,
							const Id scene)
{
	if ((performed != createDiagram) && (performed != destroyDiagram)) {
		write("#InvalidMessage\n",scene);
		if (flagsEnabled[invalidMessages])
			log(performed, scene, Id(), QVariant(), QString());
	} else
		log(performed, scene, Id(), QVariant(), QString());
}

void Logger::log(action performed,
					const Id scene, const Id target)
{
	if ((performed != addElement) && (performed != removeElement)) {
		write("#InvalidMessage\n",scene);
		if (flagsEnabled[invalidMessages])
			log(performed, scene, target, QVariant(), QString());
	} else
		log(performed, scene, target, QVariant(), QString());
}

void Logger::log(action performed,
				 const Id scene, const Id target,
				 const QVariant data, const QString additional)
{
	if (!pass(scene))
		return;

	QString message;
	switch (performed) {
		case setData:
			if ((!flagsEnabled[uselessMessages]) &&
			//may be, "name" messages are useless too
				((additional == QString("position")) ||
				 (additional == QString("configuration"))))
				return;
			cleanDiagrams.remove(scene);
			message += QString("SetData");
			break;
		case addElement:
			cleanDiagrams.remove(scene);
			message += QString("AddElement");
			break;
		case removeElement:
			cleanDiagrams.remove(scene);
			message += QString("RemoveElement");
			break;
		case createDiagram:
			cleanDiagrams.insert(scene);
			message += QString("CreateDiagram");
			break;
		case destroyDiagram:
			if (cleanDiagrams.contains(scene)) {
				cleanDiagrams.remove(scene);
				remove(scene);
				return;
			}
			cleanDiagrams.remove(scene);
			message += QString("DestroyDiagram");
			break;
	}

	message += "\n";
	if (target.idSize() > 1)
		message += target.toString() + "\n";
	if (!additional.isNull())
		message += additional + "\n";
	if (!data.isNull())
		message += getDataString(data) + "\n";

	write(message, scene);
}

bool Logger::pass(const Id scene)
{
	const bool editor = scene.editor() == QString("Meta_editor");
	return
		((!editor && flagsEnabled[diagrams])
			|| (editor && flagsEnabled[editors]));
}

void Logger::remove(const Id scene)
{
	mModel->mutableApi().logRemove(scene);
}

void Logger::write(const QString message, const Id scene)
{
	if (!enabled)
		return;
	mModel->mutableApi().log(message, scene);
}

QString Logger::getDataString(const QVariant data) const
{
	QString output;
	QDebug qD = QDebug(&output);
	qD << data;
	return output;
}
