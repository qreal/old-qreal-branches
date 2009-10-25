#include "javaHandler.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtCore/QDebug>

#include "../../kernel/definitions.h"
#include "../../client/repoApi.h"

using namespace qReal;
using namespace generators;

JavaHandler::JavaHandler(client::RepoApi const &api)
		: mApi(api)
{
}

QString JavaHandler::generateToJava(QString const &pathToFile)
{
	mErrorText = "";

	QFile file(pathToFile);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return "";

	QTextStream out(&file);

	Id repoId = ROOT_ID;

	out << serializeChildren(repoId);

	IdList rootDiagrams = mApi.children(repoId);

	foreach (Id const typeDiagram, rootDiagrams) {
		out << serializeChildren(typeDiagram);
	}

	qDebug() << "Done.";
	return mErrorText;
}

QString JavaHandler::serializeChildren(Id const &idParent)
{
	QString result = "";
	IdList childElems = mApi.children(idParent);

	foreach (Id const id, childElems) {
		result += serializeObject(id, idParent);
	}

	return result;
}

QString JavaHandler::serializeObject(Id const &id, Id const &parentId)
{
	QString result = "";

	QString additionalParams = "";

	QString const objectType = mApi.typeName(id);
	QString const parentType = mApi.typeName(parentId);

	if (objectType == "krnnDiagram") {
	}

	// class diagramm

	else if (objectType == "cnClass") {
		QString visibility = getVisibility(id);
		QString isFinalField = hasModifier(id, "final");
		QString isAbstractField = hasModifier(id, "abstract");
		QString parents = getParents(id);
		if (isAbstractField == "abstract " && isFinalField == "final "){
			addError("unable to serialize object " + objectType + " with id: " + id.toString() + ". \"abstract final\" declaration doesn't make sence");
		}
		result += isAbstractField + isFinalField + visibility + "class " + mApi.name(id) + parents +  " {" + "\n";
		result += serializeChildren(id);
		result += "}\n";
	} else if (objectType == "cnClassView") {
		//	    to do someting
	} else if (objectType == "cnClassMethod") {
		if (parentType == "cnClass") {
			QString visibility = getVisibility(id);
			QString type = getType(id);
			QString operationFactors = getOperationFactors(id);
			QString isFinalField = hasModifier(id, "final");
			QString isAbstractField = hasModifier(id, "abstract");
			QString isStaticField = hasModifier(id, "static");
			QString isSynchronizedField = hasModifier(id, "synchronized");
			QString isNativeField = hasModifier(id, "native");
			if ( (isAbstractField == "abstract " && isFinalField == "final ")
				|| (isAbstractField == "abstract " && isStaticField == "static ") ){
				addError("unable to serialize object " + objectType + " with id: " + id.toString() + ". \"abstract static\" or \"abstract final\" declaration doesn't make sence");
			}
			result += isAbstractField + isFinalField + isStaticField + isSynchronizedField + isNativeField +
					  visibility + type  + mApi.name(id) + "(" + operationFactors + "){};" + "\n";
		} else {
			this->addError("unable to serrialize object " + objectType + " with id: " + id.toString() + ". Move it inside some cnClass");
		}
	} else if (objectType == "cnClassField") {
		if (parentType == "cnClass"){
			QString visibility = getVisibility(id);
			QString type = getType(id);
			QString defaultValue = getDefaultValue(id);
			QString isFinalField = hasModifier(id, "final");
			QString isStaticField = hasModifier(id, "static");
			QString isVolatileField = hasModifier(id, "volatile");
			QString isTransientField = hasModifier(id, "transient");
			if (isVolatileField == "volatile " && isFinalField == "final "){
				addError("unable to serialize object " + objectType + " with id: " + id.toString() + ". \"final volatile\" declaration doesn't make sence");
			}
			result += isFinalField + visibility + isStaticField + isVolatileField + isTransientField + type + mApi.name(id);
			if (defaultValue != ""){
				result += " " + defaultValue;
			}
			result += ";\n";
		} else {
			addError("unable to serialize object " + objectType + " with id: " + id.toString() + ". Move it inside some cnClass");
		}
	}

	return result;
}

QString JavaHandler::getVisibility(Id const &id)
{
	QString result = "";

	QString const objectType = mApi.typeName(id);

	if (mApi.hasProperty(id, "visibility")) {
		QString visibility = mApi.stringProperty(id, "visibility");

		if (visibility == "+")
			visibility = "public";
		else if (visibility == "-")
			visibility = "private";
		else if (visibility == "#")
			visibility = "protected";
		else if (visibility == "~")
			visibility = "";

		if (isVisibilitySuitable(visibility)) {
			result = visibility;
			if (result != "")
				result += " ";
		} else {
			addError("object " + objectType + " with id  " + id.toString() + " has invalid visibility property: " + visibility);
		}
	}

	return result;
}

QString JavaHandler::getType(Id const &id)
{
	QString result = "";

	QString const objectType = mApi.typeName(id);

	if (mApi.hasProperty(id, "type")) {
		QString type = mApi.stringProperty(id, "type");

		if (isTypeSuitable(type) || (objectType == "cnClassMethod" && type == "void")) {
			result = type;
			if (result != "")
				result += " ";
		} else {
			addError("object " + objectType + " with id " + id.toString() + " has invalid type: " + type);
		}
	}

	return result;
}

QString JavaHandler::getParents(Id const &id)
{
	QString result = "";
	bool hasParentClass = false;

	QString const objectType = mApi.typeName(id);

	if (!mApi.links(id).isEmpty()) {
		IdList links = mApi.outcomingLinks(id);

		foreach (Id const aLink, links){
			if (aLink.element() == "krneGeneralization") {
				if (hasParentClass == false) {
					hasParentClass = true;
					if (id == mApi.otherEntityFromLink(aLink, id)) {
						addError("object " + objectType + " with id " + id.toString() + " can not be his own superclass");
					} else {
						QString parentClass = mApi.name(mApi.otherEntityFromLink(aLink, id));
						result += " extends " + parentClass;
					}
				} else {
					addError("object " + objectType + " with id " + id.toString() + " has too much superclasses");
				}
			}
		}

	}

	return result;
}

QString JavaHandler::hasModifier(Id const &id, QString const &modifier)
{
	QString result = "";

	QString const objectType = mApi.typeName(id);

	QString isModifier = "";

	if (modifier == "final") {
		isModifier = "isLeaf";
	} else {
		isModifier = "is" + modifier.left(1).toUpper() + modifier.mid(1, modifier.length());
	}

	if (mApi.hasProperty(id, isModifier)) {
		QString hasModifier = mApi.stringProperty(id, isModifier);

		if (hasModifier == "true") {
			result = modifier + " ";
		}else if (hasModifier != "false" && hasModifier != "") {
			addError("object " + objectType + " with id " + id.toString() + " has invalid " + isModifier + " value: " + hasModifier);
		}
	}

	return result;
}

QString JavaHandler::getOperationFactors(Id const &id)
{
	QString result = "";

	QString const objectType = mApi.typeName(id);

	if (mApi.hasProperty(id, "type")) {
		QString operationFactors = mApi.stringProperty(id, "operationFactors");

		//	to check for the corract data
		//	if (isTypeSuitable(type) || (objectType == "cnClassMethod" && type == "void")) {
		result = operationFactors;
		//	} else {
		//		addError("object " + objectType + " with id " + id.toString() + " has invalid type: " + type);
		//    	}
	}

	return result;
}

QString JavaHandler::getDefaultValue(Id const &id)
{
	QString result = "";

	QString const objectType = mApi.typeName(id);

	if (mApi.hasProperty(id, "defaultValue")) {
		QString defaultValue = mApi.stringProperty(id, "defaultValue");

		//	if (isTypeSuitable(defaultValue)) {
		//	to check for the corract data
		result = defaultValue;
		//	} else {
		//		addError("object " + objectType + " with id " + id.toString() + " has invalid default value: " + defaultValue);
		//    	}
	}

	return result;
}

bool JavaHandler::isTypeSuitable(QString const &type) const
{
	//  todo: check class-type for correctness
	return type == "int" || type == "float" || type == "double" || type == "boolean"
			|| type == "char" || type == "byte" || type == "long" || type == "short";
}

bool JavaHandler::isVisibilitySuitable(QString const &visibility) const
{
	return visibility == "public" || visibility == "private" || visibility == "protected" || visibility == "";
}

void JavaHandler::addError(QString const &errorText)
{
	mErrorText += errorText + "\n";
}
