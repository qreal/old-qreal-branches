#include "logicalModelAssistApi.h"
#include <QtCore/QUuid>

using namespace qReal;
using namespace models;
using namespace details;

LogicalModelAssistApi::LogicalModelAssistApi(LogicalModel &logicalModel, EditorManager const &editorManager)
	: ModelsAssistApi(logicalModel, editorManager), mLogicalModel(logicalModel)
{
}

qrRepo::LogicalRepoApi const &LogicalModelAssistApi::logicalRepoApi() const
{
	return mLogicalModel.api();
}

Id LogicalModelAssistApi::createElement(Id const &parent, NewType const &type)
{
	Q_ASSERT(type.typeSize() == 3);
	//Q_ASSERT(parent.idSize() == 4);

	Id const newElementId(QUuid::createUuid().toString());
	QString const elementFriendlyName = mEditorManager.friendlyName(type);
	mLogicalModel.addElementToModel(parent, newElementId, newElementId, type, "(" + elementFriendlyName + ")", QPointF(0, 0));
	return newElementId;
}

Id LogicalModelAssistApi::createElement(Id const &parent, Id const &id, NewType const &type, bool isFromLogicalModel, QString const &name, QPointF const &position)
{
	return ModelsAssistApi::createElement(parent, id, type, isFromLogicalModel, name, position);
}

IdList LogicalModelAssistApi::children(Id const &element) const
{
	return mLogicalModel.api().children(element);
}

void LogicalModelAssistApi::changeParent(Id const &element, Id const &parent, QPointF const &position)
{
	Q_UNUSED(position);
	mLogicalModel.changeParent(mModel.indexById(element), mModel.indexById(parent), QPointF());
}

NewType LogicalModelAssistApi::type(Id const &id) const
{
	return mLogicalModel.api().type(id);
}

void LogicalModelAssistApi::connect(Id const &source, Id const &destination)
{
	mLogicalModel.mutableApi().connect(source, destination);
}

void LogicalModelAssistApi::disconnect(Id const &source, Id const &destination)
{
	mLogicalModel.mutableApi().disconnect(source, destination);
}

void LogicalModelAssistApi::addUsage(Id const &source, Id const &destination)
{
	mLogicalModel.mutableApi().addUsage(source, destination);
}

void LogicalModelAssistApi::deleteUsage(Id const &source, Id const &destination)
{
	mLogicalModel.mutableApi().deleteUsage(source, destination);
}

Id LogicalModelAssistApi::createConnectedElement(Id const &source, NewType const &elementType)
{
	Id element = createElement(ROOT_ID, elementType);
	QString sourceName = mLogicalModel.data(mLogicalModel.indexById(source), Qt::DisplayRole).toString();
	QString typeName = editorManager().friendlyName(elementType);
	mLogicalModel.setData(mLogicalModel.indexById(element), sourceName + " " + typeName, Qt::DisplayRole);
	return element;
}

void LogicalModelAssistApi::createConnected(Id const &sourceElement, NewType const &elementType)
{
	Id element = createConnectedElement(sourceElement, elementType);
	connect(sourceElement, element);
}

void LogicalModelAssistApi::createUsed(Id const &sourceElement, NewType const &elementType)
{
	Id element = createConnectedElement(sourceElement, elementType);
	addUsage(sourceElement, element);
}

TypeList LogicalModelAssistApi::diagramsFromList(TypeList const &list) const
{
	// TODO: diagrams are kinda special, so we need the editor to be able to
	// tell us whether this particular element is a diagram or not
	TypeList result;
	foreach (NewType type, list) {
		if (type.element().split("_").back().contains("Diagram", Qt::CaseInsensitive)) {
			if (!result.contains(type))
				result.append(type);
		}
	}
	return result;
}

TypeList LogicalModelAssistApi::diagramsAbleToBeConnectedTo(NewType const &element) const
{
	return diagramsFromList(editorManager().getConnectedTypes(element));
}

TypeList LogicalModelAssistApi::diagramsAbleToBeUsedIn(NewType const &element) const
{
	return diagramsFromList(editorManager().getUsedTypes(element));
}

void LogicalModelAssistApi::setPropertyByRoleName(Id const &elem, QVariant const &newValue, QString const &roleName)
{
	int roleIndex = roleIndexByName(elem, roleName);
	if (roleIndex < roles::customPropertiesBeginRole)
		return;
	setProperty(elem, newValue, roleIndex);
}

QVariant LogicalModelAssistApi::propertyByRoleName(Id const &elem, QString const &roleName) const
{
	int roleIndex = roleIndexByName(elem, roleName);
	if (roleIndex < roles::customPropertiesBeginRole)
		return "";
	return property(elem, roleIndex);
}

bool LogicalModelAssistApi::isLogicalId(Id const &id) const
{
	return indexById(id) != QModelIndex();
}
