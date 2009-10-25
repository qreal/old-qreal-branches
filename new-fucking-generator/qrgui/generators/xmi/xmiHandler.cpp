#include "xmiHandler.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtCore/QDebug>

#include "../../kernel/definitions.h"
#include "../../client/repoApi.h"

using namespace qReal;
using namespace generators;

XmiHandler::XmiHandler(client::RepoApi const &api)
	: mApi(api)
{
}

QString XmiHandler::exportToXmi(QString const &pathToFile)
{
	mErrorText = "";

	QFile file(pathToFile);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return "";

	QTextStream out(&file);

	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << "\n";
	out << "<xmi:XMI xmlns:xmi=\"http://schema.omg.org/spec/XMI/2.1\" xmlns:uml=\"http://schema.omg.org/spec/UML/2.0\" xmi:version=\"2.1\">" << "\n";

	out << "\n";
	out << "\n";

	//  --------------  header --------------- //
	out << "<xmi:Documentation exporter=\"QReal\" />" << "\n";

	out << "\n";
	out << "\n";

	//  --------------  end of header --------------- //

	//  --------------  content --------------- //
	Id repoId = ROOT_ID;

	out << "<uml:Package xmi:id=\"" + repoId.toString() + "\" xmi:uuid=\""
		+ repoId.toString() + "\" name=\"RootDiagram\">" << "\n";

	out << initPrimitiveTypes();

	out << serializeChildren(repoId);

	/* Это нам потребуется, когда к корню можно будет цеплять только диаграммы.
	IdList rootDiagrams = mApi.children(repoId);

	foreach (Id const typeDiagram, rootDiagrams) {
		out << serializeChildren(typeDiagram);
	}
	*/

	out << "</uml:Package>" << "\n";
	//  --------------  end of content  --------------- //

	out << "</xmi:XMI>" << "\n";
	qDebug() << "Done.";
	return mErrorText;
}

QString XmiHandler::serializeChildren(Id const &idParent)
{
	QString result = "";
	IdList childElems = mApi.children(idParent);

	foreach (Id const id, childElems) {
		result += serializeObject(id, idParent);
	}

	if (idParent != ROOT_ID) {
		IdList linksOut = mApi.outcomingLinks(idParent);

		foreach (Id const id, linksOut) {
			result += serializeOutcomingLink(id);
		}

		IdList linksIn = mApi.incomingLinks(idParent);

		foreach (Id const id, linksIn) {
			result += serializeIncomingLink(id);
		}
	}

	return result;
}

QString XmiHandler::serializeObject(Id const &id, Id const &parentId)
{
	QString result = "";

	QString typeOfElem = "";
	QString typeOfTag = "";
	QString additionalParams = "";

	QString const objectType = mApi.typeName(id);
	QString const parentType = mApi.typeName(parentId);

	if (objectType == "krnnDiagram") {
		typeOfTag = "ownedMember";
		typeOfElem = "uml:Package";
	}

	// class diagramm

	else if (objectType == "cnClass") {
		typeOfTag = "ownedMember";
		typeOfElem = "uml:Class";
	} else if (objectType == "cnClassView") {
		typeOfTag = "ownedMember";
		typeOfElem = "uml:Package";
	}else if (objectType == "cnClassMethod") {
		if (parentType == "cnClass") {
			typeOfTag = "ownedOperation";
			typeOfElem = "uml:Operation";
		} else {
			this->addError("unable to serrialize object " + objectType + " with id: " + id.toString() + ". Move it inside some cnClass");
		}
	} else if (objectType == "cnClassField") {
		if (parentType == "cnClass"){
			typeOfTag = "ownedAttribute";
			typeOfElem = "uml:Property";
		} else {
			addError("unable to serialize object " + objectType + " with id: " + id.toString() + ". Move it inside some cnClass");
		}
	}

	//use case diagram

	else if (objectType == "uscnActor") {
		typeOfTag = "ownedMember";
		typeOfElem = "uml:Actor";
	} else if (objectType == "uscnUseCase") {
		typeOfTag = "ownedMember";
		typeOfElem = "uml:UseCase";
	}

	// sequence diagramm

	else if (objectType == "sqnnInteraction") {
		typeOfTag = "ownedMember";
		typeOfElem = "uml:Interaction";
	} else if (objectType == "sqnnSLifeline") {
		typeOfTag = "lifeline";
		typeOfElem = "uml:Lifeline";
	}

	// additional params
	if (mApi.hasProperty(id, "visibility")) {
		QString visibility = mApi.stringProperty(id, "visibility");

		if (!visibility.isEmpty()) {

			if (visibility == "+")
				visibility = "public";
			else if (visibility == "-")
				visibility = "private";
			else if (visibility == "#")
				visibility = "protected";
			else if (visibility == "~")
				visibility = "package";

			if (isVisibilitySuitable(visibility)) {
				additionalParams = additionalParams + "visibility=\"" + visibility + "\"";
			} else {
				addError("object " + objectType + " with id  " + id.toString() + " has invalid visibility property: " + visibility);
			}
		}
	}

	if (mApi.hasProperty(id, "type")) {
		QString type = mApi.stringProperty(id, "type");

		if (!type.isEmpty()) {
			if (isTypeSuitable(type)) {
				additionalParams = additionalParams + "type=\"" + type + "\"";
			} else {
				addError("object " + objectType + " with id " + id.toString() + " has invalid type: " + type);
			}
		}
	}

	if (!typeOfElem.isEmpty() && !typeOfTag.isEmpty()) {
		result += "<" + typeOfTag + " xmi:type=\"" + typeOfElem + "\" xmi:id=\""
			+ id.toString() + "\" xmi:uuid=\"" + id.toString() + "\" name=\"" + mApi.name(id)
			+ "\" " + additionalParams + ">" + "\n";
		result += serializeChildren(id);
		result += "</" + typeOfTag + ">" + "\n";
		result += serializeLinkBodies(id);
	}
	return result;
}

QString XmiHandler::serializeOutcomingLink(Id const &id)
{
	QString result = "";
	QString linkType = mApi.typeName(id);

	// kernel diagram
	// TODO: Кошмарная копипаста с идшниками
	if (linkType == "krnePackageImport") {
		result += "<packageImport xmi:type=\"uml:PackageImport\" xmi:id=\""
			+ id.toString() + "\" xmi:uuid=\"" + id.toString()
			+ "\"  importedPackage=\"" + mApi.to(id).toString() + "\" />";
	} else if (linkType == "krneElementImport") {
		result += "<elementImport xmi:type=\"uml:ElementImport\" xmi:id=\""
			+ id.toString() + "\" xmi:uuid=\"" + id.toString()
			+ "\"  importedElement=\"" + mApi.to(id).toString() + "\" />";
	} else if (linkType == "krneGeneralization") {
		result += "<generalization xmi:type=\"uml:Generalization\" xmi:id=\""
			+ id.toString() + "\" xmi:uuid=\"" + id.toString()
			+ "\" general=\"" + mApi.to(id).toString() +  "\"/>";
	} else if (linkType == "krneDirRelationship") {
		result = result + "<ownedAttribute xmi:type=\"uml:Property\" xmi:id=\""
			+ "ToEnd" + id.toString() + "\" xmi:uuid=\"" + "ToEnd" + id.toString()
			+ "\" visibility=\"protected\" type=\"" + mApi.to(id).toString() + "\">" + "\n";

		QString toMult = mApi.stringProperty(id, "toMultiplicity");
		result = result + serializeMultiplicity(id, toMult);

		result = result + "<association xmi:idref=\"" + id.toString() +  "\"/>" + "\n";
		result = result + "</ownedAttribute>" + "\n";
	}

	// class diagram

	else if (linkType == "ceDependency") {
		result += "<clientDependency xmi:idref=\"" + id.toString() + "\"/>" + "\n";
	}

	// use case diagram

	else if (linkType == "uscaExtend") {
		result += "<extend xmi:type=\"uml:Extend\" xmi:id=\"" + id.toString()
			+ "\" xmi:uuid=\"" + id.toString() + "\" extendedCase=\""
			+ mApi.to(id).toString() + "\">" + "\n";
		result += "<extension xmi:idref=\"" + mApi.from(id).toString() + "\"/>" + "\n";
		result += "</extend>\n";
	} else if (linkType == "uscaInclude") {
		result += "<include xmi:type=\"uml:Include\" xmi:id=\"" + id.toString() +
			"\" xmi:uuid=\"" + id.toString() + "\" addition=\"" + mApi.to(id).toString() + "\"/>" + "\n";
	}

	return result;
}

QString XmiHandler::serializeIncomingLink(Id const &id)
{
	QString result = "";

	if (mApi.typeName(id) == "ceDependency") {
		result += "<supplierDependency xmi:idref=\"" + id.toString()  + "\"/>";
	}

	return result;
}

QString XmiHandler::serializeLink(Id const &id)
{
	QString result = "";
	QString additionalParams = "";
	QString linkType = mApi.typeName(id);

	if (!mApi.stringProperty(id, "visibility").isEmpty()) {
		additionalParams += "visibility=\"" + mApi.property(id, "visibility").toString() + "\"";
	}

	if (linkType == "ceComposition" || linkType == "ceAggregation"
		|| linkType == "krneRelationship" || linkType == "ceRelation"
		|| linkType == "krneDirRelationship")
	{
		QString aggregation = "";

		if (linkType == "ceComposition") {
			aggregation = "aggregation=\"composite\"";
		} else if (linkType == "ceAggregation") {
			aggregation = "aggregation=\"shared\"";
		}

		result += "<ownedMember xmi:type=\"uml:Association\" xmi:id=\""
			+ id.toString() + "\" xmi:uuid=\"" + id.toString() + "\" " + additionalParams + ">" + "\n";

		// FromEnd

		result += "<memberEnd xmi:idref=\"FromEnd" + id.toString() + "\"/>\n";
		result += "<ownedEnd xmi:type=\"uml:Property\" xmi:id=\""
			+ QString("FromEnd") + id.toString() + "\" xmi:uuid=\"FromEnd" + id.toString()
			+ "\" visibility=\"protected\" type=\"" + mApi.from(id).toString() + "\">\n";
		result += "<association xmi:idref=\"" + id.toString() +  "\"/>\n";

		QString fromMult = mApi.stringProperty(id, "fromMultiplicity");
		result += serializeMultiplicity(id, fromMult);

		result += "<association xmi:idref=\"" + id.toString() +  "\"/>\n";
		result += "</ownedEnd>\n";

		// ToEnd

		result += "<memberEnd xmi:idref=\"ToEnd" + id.toString() + "\"/>\n";

		if (linkType != "krneDirRelationship") {

			result += "<ownedEnd xmi:type=\"uml:Property\" xmi:id=\"ToEnd"
				+ id.toString() + "\" xmi:uuid=\"ToEnd" + id.toString()
				+ "\" visibility=\"protected\" " + aggregation + " type=\""
				+ mApi.to(id).toString() + "\">" + "\n";

			QString toMult = mApi.stringProperty(id, "toMultiplicity");
			result += serializeMultiplicity(id, toMult);

			result += "<association xmi:idref=\"" + id.toString() +  "\"/>" + "\n";
			result += "</ownedEnd>\n";
		}

		result += "</ownedMember>\n";
	} else if (linkType == "ceDependency"){
		result += "<ownedMember xmi:type=\"uml:Dependency\" xmi:id=\""
			+ id.toString() + "\" xmi:uuid=\"" + id.toString() + "\" "
			+ additionalParams + ">" + "\n";
		result = "<supplier xmi:idref=\"" + mApi.to(id).toString() + "\"/>\n";
		result = "<client xmi:idref=\"" + mApi.from(id).toString() + "\"/>\n";
		result = "</ownedMember>\n";
	}

	return result;
}

QString XmiHandler::serializeLinkBodies(Id const &id)
{
	QString result = "";

	foreach (Id const id, mApi.incomingLinks(id)) {
		result += serializeLink(id);
	}

	return result;
}

QString XmiHandler::serializeMultiplicity(Id const &id, QString const &multiplicity) const
{
	QString result = "";
	if (!multiplicity.isEmpty()) {
		QString valueLower;
		QString valueUpper;

		if (multiplicity == "1..*") {
			valueLower = "1";
			valueUpper = "*";
		} else if (multiplicity == "0..1") {
			valueLower = "0";
			valueUpper = "1";
		} else if (multiplicity == "1") {
			valueLower = "1";
			valueUpper = "1";
		} else if (multiplicity == "*") {
			valueLower = "*";
			valueUpper = "*";
		}

		if (!valueLower.isEmpty() && !valueUpper.isEmpty()) {
			result += QString("<lowerValue xmi:type=\"uml:LiteralString\" xmi:id=\"")
					 + "loverValueTo" + id.toString() + "\" xmi:uuid=\"loverValueTo" + id.toString()
					 + "\" visibility=\"public\" value=\"" + valueLower + "\"/>\n";
			result = QString("<upperValue xmi:type=\"uml:LiteralString\" xmi:id=\"")
					 + "upperValueTo" + id.toString() + "\" xmi:uuid=\"" + "upperValueTo" + id.toString()
					 + "\" visibility=\"public\" value=\"" + valueUpper + "\"/>\n";
		}
	}

	return result;
}

QString XmiHandler::initPrimitiveTypes() const
{
	QString result = "";

	result += "<ownedMember xmi:type=\"uml:PrimitiveType\" xmi:id=\"int\" xmi:uuid=\"int\" name=\"int\" visibility=\"public\"/>\n";
	result += "<ownedMember xmi:type=\"uml:PrimitiveType\" xmi:id=\"float\" xmi:uuid=\"float\" name=\"float\" visibility=\"public\"/>\n";
	result += "<ownedMember xmi:type=\"uml:PrimitiveType\" xmi:id=\"double\" xmi:uuid=\"double\" name=\"double\" visibility=\"public\"/>\n";

	result += "<ownedMember xmi:type=\"uml:PrimitiveType\" xmi:id=\"char\" xmi:uuid=\"char\" name=\"char\" visibility=\"public\"/>\n";
	result += "<ownedMember xmi:type=\"uml:PrimitiveType\" xmi:id=\"boolean\" xmi:uuid=\"boolean\" name=\"boolean\" visibility=\"public\"/>\n";
	result += "<ownedMember xmi:type=\"uml:PrimitiveType\" xmi:id=\"byte\" xmi:uuid=\"byte\" name=\"byte\" visibility=\"public\"/>\n";

	return result;
}

bool XmiHandler::isTypeSuitable(QString const &type) const
{
	return type == "int" || type == "float" || type == "double" || type == "boolean" || type == "char" || type == "byte";
}

bool XmiHandler::isVisibilitySuitable(QString const &visibility) const
{
	return visibility == "public" || visibility == "private" || visibility == "protected" || visibility == "package";
}

void XmiHandler::addError(QString const &errorText)
{
	mErrorText += errorText + "\n";
}
