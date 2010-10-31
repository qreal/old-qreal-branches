#include "model.h"
#include <QtGui/QIcon>
#include <QtGui/QPolygon>
#include <QtCore/QDebug>
#include <QtCore/QUuid>

using namespace qReal;
using namespace model;
using namespace details;

Model::Model(EditorManager const &editorManager, QString const &workingDir)
	:
	mApi(workingDir), mEditorManager(editorManager),
	mAssistApi(*this, editorManager),
	mLogger(mutableApi().getWorkingDir()), mRepairer(mEditorManager)
{
	mRootItem = new ModelTreeItem(ROOT_ID, NULL);
	init();
}

Model::~Model()
{
	cleanupTree(mRootItem);
	mTreeItems.clear();
}

bool Model::isDiagram(Id const &id) const
{
	return ((id != ROOT_ID) && (id.element() == assistApi().editorManager().getEditorInterface(id.editor())->diagramNodeName(id.diagram())));
}

details::ModelTreeItem* Model::isSituatedOn(details::ModelTreeItem *element) const
{
	while ((element->parent() != mRootItem)
			&& (element->parent()->id().element() != "MetamodelDiagram"))
		element = element->parent();

	return element;
}

void Model::init()
{
	mTreeItems.insert(ROOT_ID, mRootItem);
	mApi.setName(ROOT_ID, ROOT_ID.toString());
	// Turn off view notification while loading. Model can be inconsistent during a process,
	// so views shall not update themselves before time. It is important for
	// scene, where adding edge before adding nodes may lead to disconnected edge.
	blockSignals(true);

	qDebug() << "";
	if (!checkElements(mRootItem->id())) {
		repairElements();
		return;
	}

	loadSubtreeFromClient(mRootItem);
	blockSignals(false);
	mApi.resetChangedDiagrams();
	mLogger.enable();
}

Qt::ItemFlags Model::flags(QModelIndex const &index) const
{
	if (index.isValid()) {
		return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled
			| Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
	} else {
	// root item has invalid index, but we should still be able to drop elements into it
		return Qt::ItemIsDropEnabled;
	}
}

QVariant Model::data(QModelIndex const &index, int role) const
{
	if (index.isValid()) {
		ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
		Q_ASSERT(item);
		switch (role) {
			case Qt::DisplayRole:
			case Qt::EditRole:
				return mApi.name(item->id());
			case Qt::DecorationRole:
				return mEditorManager.icon(item->id());
			case roles::idRole:
				return item->id().toVariant();
			case roles::positionRole:
				return mApi.property(item->id(), "position");
			case roles::fromRole:
				return mApi.from(item->id()).toVariant();
			case roles::toRole:
				return mApi.to(item->id()).toVariant();
			case roles::fromPortRole:
				return mApi.fromPort(item->id());
			case roles::toPortRole:
				return mApi.toPort(item->id());
			case roles::configurationRole:
				return mApi.property(item->id(), "configuration");
		}
		if (role >= roles::customPropertiesBeginRole) {
			QString selectedProperty = findPropertyName(item->id(), role);
			return mApi.property(item->id(), selectedProperty);
		}
		Q_ASSERT(role < Qt::UserRole);
		return QVariant();
	} else {
		return QVariant();
	}
}

bool Model::setData(QModelIndex const &index, QVariant const &newValue, int role)
{
	if (index.isValid()) {
		ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
		Id id = item->id();
		QString message;
		QVariant prevValue;
		switch (role) {
		case Qt::DisplayRole:
		case Qt::EditRole:
			prevValue = mApi.name(id);
			mApi.setName(id, newValue.toString());
			message = "name";
			emit nameChanged(index);
			break;
		case roles::positionRole:
			prevValue = mApi.property(id, "position");
			mApi.setProperty(id, "position", newValue);
			message = "position";
			break;
		case roles::configurationRole:
			prevValue = mApi.property(id, "configuration");
			mApi.setProperty(id, "configuration", newValue);
			message = "configuration";
			break;
		case roles::fromRole:
			prevValue = mApi.property(id, "from");
			mApi.setFrom(id, newValue.value<Id>());
			message = "from";
			break;
		case roles::toRole:
			prevValue = mApi.property(id, "to");
			mApi.setTo(id, newValue.value<Id>());
			message = "to";
			break;
		case roles::fromPortRole:
			prevValue = mApi.property(id, "fromPort");
			mApi.setFromPort(id, newValue.toDouble());
			message = "fromPort";
			break;
		case roles::toPortRole:
			prevValue = mApi.property(id, "toPort");
			mApi.setToPort(id, newValue.toDouble());
			message = "toPort";
			break;
		default:
			if (role >= roles::customPropertiesBeginRole) {
				QString selectedProperty = findPropertyName(id, role);
				prevValue = mApi.property(id, selectedProperty);
				mApi.setProperty(id, selectedProperty, newValue);
				message = selectedProperty;
				break;
			}
			Q_ASSERT(role < Qt::UserRole);
			return false;
		}

		mLogger.rememberNameOfScene(isSituatedOn(item)->id(), mApi.name(isSituatedOn(item)->id()));
		mLogger.log(actSetData, isSituatedOn(item)->id(), id, prevValue, newValue, message);

		emit dataChanged(index, index);
		return true;
	}
	return false;
}

QString Model::findPropertyName(Id const &id, int const role) const
{
	// В случае свойства, описанного в самом элементе, роль - просто
	// порядковый номер свойства в списке свойств. Этого соглашения
	// надо всюду придерживаться, а то роли "поедут".
	QStringList properties = mEditorManager.getPropertyNames(id.type());
	Q_ASSERT(role - roles::customPropertiesBeginRole < properties.count());
	return properties[role - roles::customPropertiesBeginRole];
}

QStringList Model::getEnumValues(QModelIndex const &index, int const role) const
{
	do {
		if (!index.isValid())
			break;
		ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
		if (!item)
			break;
		QString selectedProperty = findPropertyName(item->id(), role);
		return mEditorManager.getEnumValues(item->id(), selectedProperty);
	} while (false);

	return QStringList();
}

QString Model::getTypeName(QModelIndex const &index, int const role) const
{
	do {
		if (!index.isValid())
			break;
		ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
		if (!item)
			break;
		QString selectedProperty = findPropertyName(item->id(), role);
		return mEditorManager.getTypeName(item->id(), selectedProperty);
	} while (false);

	return QString();
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
		return QVariant("name");
	else
		return QVariant();
}

int Model::rowCount(QModelIndex const &parent) const
{
	return parentTreeItem(parent)->children().size();
}

int Model::columnCount(QModelIndex const &parent) const
{
	Q_UNUSED(parent)
	return 1;
}

bool Model::removeRows(int row, int count, QModelIndex const &parent)
{
	ModelTreeItem *parentItem = parentTreeItem(parent);

	if (parentItem->children().size() < row + count)
		return false;
	else {
		for (int i = row; i < row + count; ++i) {
			ModelTreeItem *child = parentItem->children().at(i);
			mApi.removeElement(child->id());
			removeModelItems(child);

			// TODO: Убрать копипасту.
			int childRow = child->row();
			beginRemoveRows(parent, childRow, childRow);
			child->parent()->removeChild(child);
			mTreeItems.remove(child->id(), child);
			if (mTreeItems.count(child->id()) == 0)
				mApi.removeChild(parentItem->id(), child->id());
			delete child;
			endRemoveRows();
		}
		return true;
	}
}

QString Model::pathToItem(ModelTreeItem const *item) const
{
	if (item != mRootItem) {
		QString path;
		do {
			item = item->parent();
			path = item->id().toString() + ID_PATH_DIVIDER + path;
		} while (item != mRootItem);
		return path;
	}
	else
		return ROOT_ID.toString();
}

void Model::removeConfigurationInClient(ModelTreeItem const * const item)
{
	mApi.removeProperty(item->id(), "position");
	mApi.removeProperty(item->id(), "configuration");
}

QModelIndex Model::index(ModelTreeItem const * const item) const
{
	QList<int> rowCoords;

	for (ModelTreeItem const *curItem = item;
		curItem != mRootItem; curItem = curItem->parent())
	{
		rowCoords.append(const_cast<ModelTreeItem *>(curItem)->row());
	}

	QModelIndex result;

	for (int i = rowCoords.size() - 1; i >= 0; --i) {
		result = index(rowCoords[i], 0, result);
	}

	return result;
}

void Model::removeModelItems(ModelTreeItem *root)
{
	foreach (ModelTreeItem *child, root->children()) {
		mApi.removeElement(child->id());
		removeModelItems(child);
		int childRow = child->row();
		beginRemoveRows(index(root),childRow,childRow);
		removeConfigurationInClient(child);
		child->parent()->removeChild(child);
		mTreeItems.remove(child->id(),child);
		if (mTreeItems.count(child->id())==0) {
			mApi.removeChild(root->id(),child->id());
		}
		delete child;
		endRemoveRows();
	}
}

QModelIndex Model::index(int row, int column, QModelIndex const &parent) const
{
	ModelTreeItem *parentItem = parentTreeItem(parent);
	if (parentItem->children().size()<=row)
		return QModelIndex();
	ModelTreeItem *item = parentItem->children().at(row);
	return createIndex(row,column,item);
}

QModelIndex Model::parent(QModelIndex const &index) const
{
	if (index.isValid()) {
		ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
		ModelTreeItem *parentItem = item->parent();
		if (parentItem == mRootItem || parentItem == NULL)
			return QModelIndex();
		else
			return createIndex(parentItem->row(), 0, parentItem);
	} else
		return QModelIndex();
}

Qt::DropActions Model::supportedDropActions() const
{
	return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

QStringList Model::mimeTypes() const
{
	QStringList types;
	types.append(DEFAULT_MIME_TYPE);
	return types;
}

QMimeData* Model::mimeData(QModelIndexList const &indexes) const
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	foreach (QModelIndex index, indexes) {
		if (index.isValid()) {
			ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
			stream << item->id().toString();
			stream << pathToItem(item);
			stream << mApi.property(item->id(), "name");
			stream << mApi.property(item->id(), "position").toPointF();
		} else {
			stream << ROOT_ID.toString();
			stream << QString();
			stream << ROOT_ID.toString();
			stream << QPointF();
		}
	}
	QMimeData *mimeData = new QMimeData();
	mimeData->setData(DEFAULT_MIME_TYPE, data);
	return mimeData;
}

bool Model::dropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent)
{
	Q_UNUSED(row)
	Q_UNUSED(column)
	if (action == Qt::IgnoreAction)
		return true;
	else {
		ModelTreeItem *parentItem = parentTreeItem(parent);

		QByteArray dragData = data->data(DEFAULT_MIME_TYPE);
		QDataStream stream(&dragData, QIODevice::ReadOnly);
		QString idString;
		QString pathToItem;
		QString name;
		QPointF position;
		stream >> idString;
		stream >> pathToItem;
		stream >> name;
		stream >> position;
		Id id = Id::loadFromString(idString);
		Q_ASSERT(id.idSize() == 4);  // Бросать в модель мы можем только конкретные элементы.

		if (mTreeItems.contains(id))  // Пока на диаграмме не может быть больше одного экземпляра
			// одной и той же сущности, бросать существующие элементы нельзя.
			return false;

		return addElementToModel(parentItem, id, pathToItem, name, position, action) != NULL;
	}
}

ModelTreeItem *Model::addElementToModel(ModelTreeItem *parentItem, Id const &id,
	QString const &oldPathToItem, QString const &name, QPointF const &position, Qt::DropAction action)
{
	Q_UNUSED(oldPathToItem)
	Q_UNUSED(action)

	if (isDiagram(id)) {
		if (!isDiagram(parentItem->id()) && parentItem != mRootItem) {
			qDebug() << "Diagram cannot be placed into element.";
			return NULL;
		}
		if (parentItem == mRootItem)
			mLogger.log(actCreateDiagram, id);
		else
			mLogger.log(actAddElement, isSituatedOn(parentItem)->id(), id);
	}
	else {
		if (parentItem == mRootItem) {
			qDebug() << "Element can be placed only on diagram.";
			return NULL;
		}
		mLogger.log(actAddElement, isSituatedOn(parentItem)->id(), id);
	}

	int newRow = parentItem->children().size();
	beginInsertRows(index(parentItem), newRow, newRow);
		ModelTreeItem *item = new ModelTreeItem(id, parentItem);
		parentItem->addChild(item);
		mTreeItems.insert(id, item);
		mApi.addChild(parentItem->id(), id);
		mApi.setProperty(id, "name", name);
		mApi.setProperty(id, "from", ROOT_ID.toVariant());
		mApi.setProperty(id, "to", ROOT_ID.toVariant());
		mApi.setProperty(id, "fromPort", 0.0);
		mApi.setProperty(id, "toPort", 0.0);
		mApi.setProperty(id, "links", IdListHelper::toVariant(IdList()));
		mApi.setProperty(id, "outgoingConnections", IdListHelper::toVariant(IdList()));
		mApi.setProperty(id, "incomingConnections", IdListHelper::toVariant(IdList()));
		mApi.setProperty(id, "outgoingUsages", IdListHelper::toVariant(IdList()));
		mApi.setProperty(id, "incomingUsages", IdListHelper::toVariant(IdList()));
		mApi.setProperty(id, "position", position);
		mApi.setProperty(id, "configuration", QVariant(QPolygon()));

		QStringList properties = mEditorManager.getPropertyNames(id.type());
		foreach (QString property, properties)
		// for those properties that doesn't have default values, plugin will return empty string
			mApi.setProperty(id, property, mEditorManager.getDefaultPropertyValue(id, property));

		endInsertRows();
	return item;
}

bool Model::addElementToModel(Id const &parent, Id const &id, QString const &name, QPointF const &position)
{
	ModelTreeItem *parentItem = parent == ROOT_ID ? mRootItem
		: static_cast<ModelTreeItem*>(indexById(parent).internalPointer());
	Q_ASSERT(parentItem != NULL);
	return addElementToModel(parentItem, id, "", name, position, Qt::CopyAction) != NULL;
}

void Model::changeParent(QModelIndex const &element, QModelIndex const &parent, QPointF const &position)
{
	if (!parent.isValid() || element.parent() == parent)
		return;

	int destinationRow = parentTreeItem(parent)->children().size();

	if (beginMoveRows(element.parent(), element.row(), element.row(), parent, destinationRow)) {
		ModelTreeItem *elementItem = static_cast<ModelTreeItem*>(element.internalPointer());
		QVariant configuration = mApi.property(elementItem->id(), "configuration");
		elementItem->parent()->removeChild(elementItem);
		ModelTreeItem *parentItem = parentTreeItem(parent);

		mApi.addParent(elementItem->id(), parentItem->id());
		mApi.removeParent(elementItem->id(), elementItem->parent()->id());

		elementItem->setParent(parentItem);
		parentItem->addChild(elementItem);

		mApi.setProperty(elementItem->id(), "position", position);
		mApi.setProperty(elementItem->id(), "configuration", configuration);
		endMoveRows();
	}
}

void Model::loadSubtreeFromClient(ModelTreeItem * const parent)
{
	foreach (Id childId, mApi.children(parent->id())) {
		ModelTreeItem *child = loadElement(parent, childId);
		loadSubtreeFromClient(child);
	}
}

ModelTreeItem *Model::loadElement(ModelTreeItem *parentItem, Id const &id)
{
	if (isDiagram(id))
			mApi.addOpenedDiagram(id);

	int newRow = parentItem->children().size();
	beginInsertRows(index(parentItem), newRow, newRow);
	ModelTreeItem *item = new ModelTreeItem(id, parentItem);
	checkProperties(id);
	parentItem->addChild(item);
	mTreeItems.insert(id, item);
	endInsertRows();
	return item;
}

void Model::checkProperties(Id const &id)
{
	if (!mEditorManager.hasElement(id.type()))
		return;
	QStringList propertiesThatShallBe = mEditorManager.getPropertyNames(id.type());
	foreach (QString property, propertiesThatShallBe)
		if (!api().hasProperty(id, property))
			mApi.setProperty(id, property, "");  // Типа значение по умолчанию.
	if (!api().hasProperty(id, "outgoingUsages"))
		mApi.setProperty(id, "outgoingUsages", IdListHelper::toVariant(IdList()));
	if (!api().hasProperty(id, "incomingUsages"))
		mApi.setProperty(id, "incomingUsages", IdListHelper::toVariant(IdList()));
}

QPersistentModelIndex Model::rootIndex() const
{
	return index(mRootItem);
}

void Model::open(QString const &workingDir)
{
	mApi.open(workingDir);
	mLogger.setWorkingDir(workingDir);
	reinit();
}

void Model::saveTo(QString const &workingDir)
{
	mApi.saveTo(workingDir);
	mLogger.setWorkingDir(workingDir);
	mLogger.output();
}

void Model::save()
{
	mApi.saveAll();
	mLogger.output();
	mApi.resetChangedDiagrams();
}

void Model::removeByIndex(QModelIndex const &index)
{
	Id id = idByIndex(index);
	ModelTreeItem *treeItem = mTreeItems.value(id);
	if (treeItem->parent() == mRootItem)
		mLogger.log(actDestroyDiagram, isSituatedOn(treeItem)->id());
	else
		mLogger.log(actRemoveElement, isSituatedOn(treeItem)->id(), id);

	removeRow(index.row(), index.parent());
}

void Model::clean()
{
	cleanupTree(mRootItem);
	mTreeItems.clear();
	delete mRootItem;
	mRootItem = new ModelTreeItem(ROOT_ID, NULL);
	reset();
}

void Model::reinit()
{
	clean();
	init();
}

void Model::exterminate()
{
	mApi.exterminate();
	reinit();
}

void Model::addDiagram(Id const &id)
{
	mApi.addChild(ROOT_ID, id);
}

void Model::resetChangedDiagrams()
{
	mApi.resetChangedDiagrams();
}

void Model::resetChangedDiagrams(const IdList &list)
{
	mApi.resetChangedDiagrams(list);
}


void Model::cleanupTree(ModelTreeItem *root)
{
	foreach (ModelTreeItem *childItem, root->children()) {
		cleanupTree(childItem);
		delete childItem;
	}
	root->clearChildren();
}

qrRepo::RepoApi const & Model::api() const
{
	return mApi;
}

ModelTreeItem *Model::parentTreeItem(QModelIndex const &parent) const
{
	return parent.isValid()
		? static_cast<ModelTreeItem*>(parent.internalPointer())
		: mRootItem
	;
}

qrRepo::RepoApi& Model::mutableApi()
{
	return mApi;
}

QModelIndex Model::indexById(Id const &id) const
{
	if (mTreeItems.keys().contains(id)) {
		return index(mTreeItems.find(id).value());
	}
	return QModelIndex();
}

Id Model::idByIndex(QModelIndex const &index) const
{
	ModelTreeItem *item = static_cast<ModelTreeItem*>(index.internalPointer());
	return mTreeItems.key(item);
}

bool Model::isCorrect(Id target) const
{
	return (mEditorManager.elements(target.diagramId()).contains(target.type()));
}

void Model::repairElements()
{
	qDebug() << "\nModel::repairElements()\n";
	//тут надо повесить какое-то окошко, сообщающее пользователю о том, что восстанавливается диаграмма и предлагающее отменить и т.п.
	repairElements(mRootItem->id());
//	reinit();	//пока что выключил, иначе риал зацикливается на починке элементов, т.к. на самом деле ничего не чинится
}

void Model::repairElements(const Id target)
{
	foreach(Id child, mApi.children(target)) {
		//лучше это оптимизировать со временем, чтобы не проверять по 2 раза элементы
		if (!isCorrect(child))
			mRepairer.getCorrectId(child);	//тут надо что-то с этим сделать будет
		repairElements(child);
	}
}

bool Model::checkElements(Id const target) const
{
	qDebug() << "Model::checkElements(" << target.toString() << ")";
	//критерии проверки надо будет изменить и вынести в отдельные методы
	if ((target != mRootItem->id()) && (!isCorrect(target))) {
		qDebug() << "---incorrect";
		return false;
	}
	foreach(Id child, mApi.children(target)) {
		if (!checkElements(child))
			return false;
	}
	return true;
}

ModelAssistApi &Model::assistApi()
{
	return mAssistApi;
}

ModelAssistApi const &Model::assistApi() const
{
	return mAssistApi;
}

Id Model::getRootDiagram()
{
	return mRootIndex.data(roles::idRole).value<Id>();
}

void Model::setRootIndex(const QModelIndex &index)
{
	mRootIndex = index;
}

bool Model::isChanged()
{
	return (mApi.getChangedDiagrams().size() > 0);
}
