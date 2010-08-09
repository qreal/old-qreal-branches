/** @file realrepomodel.cpp
 *	@brief Основная модель данных
 * */
#include <QtGui>
#include <QtSql>
#include <QMessageBox>

#include "realrepomodel.h"
#include "realreporoles.h"
//#define _LONG_DEBUG
#include "dbg.h"

using namespace qRealTypes;

////------------------------------------///
RealRepoModel::RealRepoModel( const QString &addr, const int port, QObject *parent )
	: QAbstractItemModel(parent)
{
dbg;
	m_error = -1;
	qDebug() << addr << port;
	repoClient = new RealRepoClient(addr, port, this);

	rootItem = new RepoTreeItem;
	rootItem->parent = NULL;
	rootItem->id = "root";
	rootItem->is_avatar = false;
	rootItem->orphan_avatar = false;

	if (!readRootTable())
		exit(1);

	// Uncomment this when needed. Make sure that splashscreen turned off :)
	// runTestQueries();

	undoStack = new QUndoStack();
	undoView = new QUndoView(undoStack);
	undoView->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
	undoView->setWindowTitle(tr("Command List"));
	undoView->setAttribute(Qt::WA_QuitOnClose, false);

	addToStack = true;

	readItems();

		// перенаправляем сигналы из undoStack к View. Напрямую это не сделать т.к. undoStack - private
		connect(undoStack, SIGNAL( canUndoChanged(bool) ), this, SIGNAL( canUndoChanged(bool) ));
		connect(undoStack, SIGNAL( canRedoChanged(bool) ), this, SIGNAL( canRedoChanged(bool) ));
}

RealRepoModel::~RealRepoModel()
{
dbg;
	cleanupTree(rootItem);
	delete rootItem;
	undoView->close();
	delete undoView;
	delete undoStack;
	delete repoClient;
}

QVariant RealRepoModel::data(const QModelIndex &index, int role) const
{
dbg;
	if (!index.isValid())
		return QVariant();

	if (index.column() != 0)
		return QVariant();

	RepoTreeItem *item = static_cast<RepoTreeItem *>(index.internalPointer());

	switch (role) {
	case Qt::DisplayRole:
	case Qt::EditRole:
	case Unreal::krnnNamedElement::nameRole:
		return hashNames[item->id];
	case Qt::DecorationRole:
		if ( hashTypes.contains(item->id) )
			return info.objectIcon(hashTypes.value(item->id));
		else
//			return info.objectIcon(item->id);
			return QIcon();
	case Unreal::IdRole:
		return item->id;
	case Unreal::TypeRole:
		return hashTypes[item->id];
	case Unreal::PositionRole:
		if ( type( item->parent ) == Container )
			if ( hashDiagramElements[item->parent->id].contains(item->id) )
				return hashDiagramElements[item->parent->id][item->id].position;

		return QVariant();
	case Unreal::ConfigurationRole:
		if ( type( item->parent ) == Container )
			if ( hashDiagramElements[item->parent->id].contains(item->id) )
				return hashDiagramElements[item->parent->id][item->id].configuration;

		return QVariant();
	default:
		if ( role > Unreal::UserRole ) {
			RealRepoInfo info;
			QString name = info.getColumnName(hashTypes[item->id], role);
//			qDebug() << "requested role:" << role << name;
			QString val = repoClient->getPropValue(item->id, name);
			return (val == "") ? QVariant() : val;
/*
			if ( hashElementProps.contains(item->id) ) {
				if ( hashElementProps[item->id].contains(role) ){
					return hashElementProps[item->id][role];
				}
				else
					return QVariant();
			} else {
				const_cast<RealRepoModel *>(this)->updateProperties(item->id);
				qDebug() << "returning" << hashElementProps[item->id][role];
				return hashElementProps[item->id][role];
			}	*/
		} else
			return QVariant(); //for now
	}

	return QVariant();
}

bool RealRepoModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
dbg;
	if (!index.isValid())
		return false;

	if (index.column() != 0)
		return false;

	RepoTreeItem *item = static_cast<RepoTreeItem*>(index.internalPointer());

	switch (role) {
	case Qt::DisplayRole:
	case Unreal::krnnNamedElement::nameRole:
	case Qt::EditRole:
		{
			repoClient->setName(item->id, value.toString());
			hashNames[item->id] = value.toString();
		}
		break;
	case Unreal::PositionRole:
		{
			repoClient->setPosition(item->id, item->parent->id, value.toPoint().x(), value.toPoint().y());
			hashDiagramElements[item->parent->id][item->id].position = value.toPoint();
		}
		break;
	case Unreal::ConfigurationRole:
		{
			QPolygon poly(value.value<QPolygon>());
			QString result;
			foreach ( QPoint point, poly ) {
				result += QString("(%1,%2);").arg(point.x()).arg(point.y());
			}
			result.chop(1);
			repoClient->setConfiguration(item->id, item->parent->id, result);
			hashDiagramElements[item->parent->id][item->id].configuration = poly;
		}
		break;
	default:
		if ( type(item->parent) == Container ) {
			QString column_name = info.getColumnName(hashTypes[item->id],role);
			QVariant old_value = QVariant(repoClient->getPropValue(item->id, column_name));
			if (!info.isPropertyRef(hashTypes[item->id], column_name))
			{
				repoClient->setPropValue(item->id,info.getColumnName(hashTypes[item->id],role), value.toString());
			}
			else
			{
				// try-catch Хак. Пусть поживет.
				try{
					qDebug() << "removing ref" << item->id << "from" << old_value.toString();
					repoClient->decReferral(old_value.toString(), item->id);
				} catch (QString e)
				{
					qDebug() << "error removing referrals";
					if (old_value.toString() != "")
						break; // Serious error, breaking
				}
				try{
					qDebug() << "adding ref" << item->id << "to" << value.toString();
					repoClient->incReferral(value.toString(), item->id);
				} catch (QString e)
				{
					qDebug() << "error adding referrals";
					if (value.toString() != "")
						break; // Serious error, breaking
				}
				repoClient->setPropValue(item->id,info.getColumnName(hashTypes[item->id],role), value.toString());
			}
		}
		break;
	}

	foreach(RepoTreeItem *item, hashTreeItems[item->id]) {
		//		item->updateData();
		QModelIndex index = this->index(item);
		emit dataChanged(index,index);
	}

	return true;
}


//--------------------View Interface------------------//

bool RealRepoModel::changeRole(const QModelIndex & index, const QVariant & value, int role)
{
dbg;
	qDebug() << "New role in changeRole:  " << role;
	if (!index.isValid())
		return false;

	if (index.column() != 0)
		return false;

	RepoTreeItem *item = static_cast<RepoTreeItem*>(index.internalPointer());

	switch (role) {
	case Qt::DisplayRole:
		//qDebug() << "DisplayRole";
		//break;
	case Unreal::krnnNamedElement::nameRole:
	case Qt::EditRole:
		{
		QVariant old_value = QVariant(repoClient->getName(item->id));
		undoStack->push(new ChangeEditRoleCommand(this, index, old_value, value, role));
		}
		break;
	case Unreal::PositionRole:
		if ( type(item->parent) == Container ) {
			QVariant old_value = QVariant(hashDiagramElements[item->parent->id][item->id].position);
			if(old_value.toPoint().x() != value.toPoint().x() || old_value.toPoint().x() != value.toPoint().x()){
				undoStack->push(new ChangePositionCommand(this, index, old_value, value, role));
			}
		}
		break;
	case Unreal::ConfigurationRole:
		if ( type(item->parent) == Container ) {
			QVariant old_value = QVariant(hashDiagramElements[item->parent->id][item->id].configuration);
			QPolygon old_poly(old_value.value<QPolygon>());
			QPolygon poly(value.value<QPolygon>());
			if(poly != old_poly){
				QString result;
				foreach ( QPoint point, poly ) {
					result += QString("(%1,%2);").arg(point.x()).arg(point.y());
				}
				result.chop(1);
				undoStack->push(new ChangeConfigurationCommand(this, index, old_value, value, role));
				}
		}
		break;
	default:
		qDebug() << "New role in changeRole:  " << role;
		// Нужен ли здесь этот if ?
		if ( type(item->parent) == Container ) {
			QString column_name = info.getColumnName(hashTypes[item->id],role);
			QVariant old_value = QVariant(repoClient->getPropValue(item->id, column_name));
			undoStack->push(new ChangeUserRoleCommand(this, index, old_value, value, role));
		}
		break;
	}

	foreach(RepoTreeItem *item, hashTreeItems[item->id]) {
		//		item->updateData();
		QModelIndex index = this->index(item);
		emit dataChanged(index,index);
	}

	return true;
}

void RealRepoModel::deleteElement(QModelIndex index)
{
	undoStack->beginMacro("Delete Element");

	internalDeleteElement(index);

	undoStack->endMacro();
}

void RealRepoModel::internalDeleteElement(QModelIndex ind)
{
	RepoTreeItem *item = static_cast<RepoTreeItem*>(ind.internalPointer());
	foreach (RepoTreeItem *cur, item->children){
		internalDeleteElement(index(cur));
	}
	undoStack->push(new DeleteElementCommand(this,ind));
}






QModelIndex RealRepoModel::createDefaultTopLevelItem() {
	RepoTreeItem *diagramCategory = hashTreeItems["krnnDiagram"].first();
	if (diagramCategory->children.empty()) {
		addElementToModel(diagramCategory, index(diagramCategory), "", "",
			"krnnDiagram", "Root diagram", QPointF(), Qt::CopyAction, -1);
	}

	if (!diagramCategory->children.empty()) {
		return index(diagramCategory->children[0]);
	}
	else
		return QModelIndex();
}

void RealRepoModel::readItems()
{
	RepoTreeItem *diagramCategory = hashTreeItems["krnnDiagram"].first();
	readCategoryTable(diagramCategory);
	foreach (RepoTreeItem *diagram, diagramCategory->children) {
		readItemsRecurse(diagram);
	}
}

void RealRepoModel::readItemsRecurse( RepoTreeItem *parent )
{
	readContainerTable(parent);
	foreach (RepoTreeItem *childItem,parent->children) {
		readItemsRecurse(childItem);
	}
}

Qt::ItemFlags RealRepoModel::flags(const QModelIndex &index) const
{
	switch ( type(index) ) {
	case Container:
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable |
		       Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
		       Qt::ItemIsDropEnabled;
	case Category:
		return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
	case Root:
		return Qt::ItemIsEnabled;
	default:
		return 0;
	}
}

QVariant RealRepoModel::headerData(int section, Qt::Orientation orientation,
		int role) const
{
dbg;
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0 )
		return tr("Name");

	return QVariant();
}

QModelIndex RealRepoModel::index(const RepoTreeItem *item) const
{
dbg;
	QList <int> rowCoords;

	for ( RepoTreeItem *curItem = const_cast<RepoTreeItem *>(item);
			curItem != rootItem; curItem = curItem->parent ) {
	// WT* is this if?
//		if( curItem && curItem->row != 0){
			rowCoords.append(curItem->row);
//		}
	}

	QModelIndex result;

	for ( int i = rowCoords.size() - 1; i >= 0; i-- ) {
		result = index(rowCoords[i],0,result);
	}

	return result;
}

QModelIndex RealRepoModel::index(int row, int column, const QModelIndex &parent)
	const
{
dbg;
	RepoTreeItem *parentItem = NULL;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<RepoTreeItem*>(parent.internalPointer());

//	qDebug() << "[INDEX]: id: " << parentItem->id << "row:" << row << "column:" << column << "children:" << parentItem->children.size();

	if ( parentItem->children.isEmpty() ) {
		if (type(parentItem) == Container ) {
			const_cast<RealRepoModel *>(this)->readContainerTable(parentItem);
		} else {
			const_cast<RealRepoModel *>(this)->readCategoryTable(parentItem);
		}
	}

	RepoTreeItem *childItem = 0;
	if( parentItem && parentItem->children.size() > row && row >= 0 )
		childItem = parentItem->children[row];

//	qDebug() << parentItem->children.size() << row << parentItem->id;
//	RepoTreeItem *childItem = parentItem->children[row];

	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex RealRepoModel::parent(const QModelIndex &child) const
{
dbg;
	if (!child.isValid())
		return QModelIndex();

	RepoTreeItem *childItem = static_cast<RepoTreeItem*>(child.internalPointer());
	RepoTreeItem *parentItem = 0;
	if( childItem )
		parentItem = childItem->parent;

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row, 0, parentItem);
}

int RealRepoModel::rowCount(const QModelIndex &parent) const
{
dbg;
	RepoTreeItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<RepoTreeItem*>(parent.internalPointer());

	return parentItem->children.size();
}

int RealRepoModel::columnCount(const QModelIndex &/*parent*/) const
{
dbg;
	return 1;
}

bool RealRepoModel::canBeDeleted(const QModelIndex &ind) const
{
	RepoTreeItem *item = static_cast<RepoTreeItem*>(ind.internalPointer());
	// Check for avatar
	if (item->is_avatar && !item->orphan_avatar)
		return false;

	// Check for referrals
	IdTypeList l = repoClient->getReferrals(item->id);
	if (!l.empty())
		return false;

	// Has avatar. Then we should check avatar's children, not this
	if (item->has_avatar)
		item = item->avatar;

	// Ask children, whether they are ready to be removed
	foreach (RepoTreeItem *cur, item->children)
	{
		if (!canBeDeleted(index(cur)))
			return false;
	}
	return true;
}

void RealRepoModel::deleteElementSafe(QModelIndex ind)
{
	deleteElement(ind);
}
/*{
	RepoTreeItem *item = static_cast<RepoTreeItem*>(ind.internalPointer());
	RepoTreeItem *parentItem = static_cast<RepoTreeItem*>(ind.parent().internalPointer());
	QModelIndex *i, ava;

	i = &ind;
	if (item->has_avatar)
	{
		// This is diagram inv avatar. Remove it IMMEDIATELY
		beginRemoveRows(ind.parent(), ind.row(), ind.row());
		ava = index(item->avatar);
		item->avatar->orphan_avatar = true;
		endRemoveRows();
		i = &ava;
		item = static_cast<RepoTreeItem*>(ava.internalPointer());
		parentItem = static_cast<RepoTreeItem*>(ava.parent().internalPointer());
	}

//	if (item->orphan_avatar)
//		repoClient->deleteObject(parentItem->children[i]->id, curItem->inv_avatar->id);

	// Delete children
	foreach (RepoTreeItem *cur, item->children)
		deleteElementSafe(index(cur));

	// Delete this element
	beginRemoveRows(i->parent(), i->row(), i->row());

	TypeIdType t = hashTypes[item->id];
	QStringList l = info.getColumnNames(t);

	qDebug() << "Processing referrals";
	foreach(QString p, l)
	{
		// When deleting object, process refs first
		if (info.isPropertyRef(t, p))
			setData(*i, "", info.roleByColumnName(t, p));
	}

	qDebug() << "id:" << item->id << "parent id:" << parentItem->id << "row1: " << i->row() << "row2:" << item->row;
	repoClient->deleteObject(item->id, parentItem->id);

	hashTreeItems[item->id].removeAll(parentItem->children.at(i->row()));

	delete parentItem->children.at(i->row());
	parentItem->children.removeAt(i->row());

	for ( int j = 0; j < parentItem->children.size(); j++ )
		parentItem->children[j]->row = j;

	endRemoveRows();
}*/

bool RealRepoModel::removeRows ( int row, int count, const QModelIndex & parent )
{
	RepoTreeItem *parentItem;
	int i;

	if ( parent.isValid() )
		parentItem = static_cast<RepoTreeItem*>(parent.internalPointer());
	else {
		return false;
	}

	qDebug() << "attempt to remove objects" << row << count;

	// Check preconditions.
	for (i = row; i < row+count; i++)
	{
		if (!canBeDeleted(index(i, 0, parent)))
			return false;
	}

	// Perform deletion
	for (i = row; i < row+count; i++)
		deleteElementSafe(index(row, 0, parent));
	return true;
}

void RealRepoModel::removeChildrenRows( QPersistentModelIndex parent, RepoTreeItem* parentItem, int row, int count )
{
	beginRemoveRows(parent,row,row+count-1);

	for ( int i = row; i < row+count; i++ ) {
		hashTreeItems[parentItem->children.at(row)->id].removeAll(parentItem->children.at(row));

		cleanupTree(parentItem->children.at(row));
		delete parentItem->children.at(row);

		parentItem->children.removeAt(row);
	}

	for ( int i = 0; i < parentItem->children.size(); i++ )
		parentItem->children[i]->row = i;

	endRemoveRows();
}

QStringList RealRepoModel::mimeTypes () const
{
	return QStringList() << "application/x-real-uml-data";
}

QMimeData * RealRepoModel::mimeData ( const QModelIndexList & indexes ) const
{
dbg;
	qDebug() << "++++++++++++++++++!!!!!!!!!!!!!!!!++++++++++++++++++";
	qDebug() << "index list size: " << indexes.size();

	RepoTreeItem *item;
	if ( indexes.at(0).isValid() )
		item = static_cast<RepoTreeItem *>(indexes.at(0).internalPointer());
	else{
		qDebug() << "bad item dragged!";
		return 0;
	}

	QByteArray itemData;
	QDataStream stream(&itemData, QIODevice::WriteOnly);
	stream << item->id;
	stream << hashTypes[item->id];
	stream << item->parent->id;
	stream << hashNames[item->id];
	stream << hashDiagramElements[item->parent->id][item->id].position;
	stream << item->row;

	qDebug() << "ID: 	 " << item->id;
	qDebug() << "TYPE: 	 " << hashTypes[item->id];
	qDebug() << "PARENT: " << item->parent->id;

	QMimeData *mimeData = new QMimeData;
	mimeData->setData("application/x-real-uml-data", itemData);
	return mimeData;
}

Qt::DropActions RealRepoModel::supportedDropActions () const
{
dbg;
	return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

bool RealRepoModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
		int row, int column, const QModelIndex &parent)
{
dbg;
	Q_UNUSED(row);

	if (action == Qt::IgnoreAction)
		return true;
	if (!data->hasFormat("application/x-real-uml-data"))
		return false;
	if (column > 0)
		return false;

//	qDebug() << "parent is valid" << parent.isValid();
	RepoTreeItem *parentItem = rootItem;
	if (parent.isValid())
		parentItem = static_cast<RepoTreeItem *>(parent.internalPointer());

	QByteArray dragData = data->data("application/x-real-uml-data");
	QDataStream stream(&dragData, QIODevice::ReadOnly);

	QString name;
	IdType newtype = "", oldParent = "";
	TypeIdType newid = "";
	QPointF newPos;
	int oldRow;

	stream >> newid;
	stream >> newtype;
	stream >> oldParent;
	stream >> name;
	stream >> newPos;
	stream >> oldRow;

//	qDebug() << "dropped" << newid << newtype << name << newPos;

	qDebug() << "dropMimeData" << parentItem->id << newtype << newid;
	return addElementToModel(parentItem, parent, oldParent, newid, newtype, name, newPos,
		action,oldRow);
}

bool RealRepoModel::addElementToModel( RepoTreeItem *const parentItem, const QModelIndex &parent,
									  IdType const &oldParent, IdType const &newid,
									  TypeIdType const &newtype, QString const &name,
									  QPointF const &newPos, Qt::DropAction action, int oldRow )
{
	switch (type(parentItem)) {
		case Category:
			{
//				qDebug() << parentItem->id << newtype;
				qDebug() << newid << newtype << name << newPos;

				if ( parentItem->id != newtype) {
					qDebug() << "Object dragged into the wrong category";
					return false;
				}

				beginInsertRows(parent, parentItem->children.size(), parentItem->children.size());
				// FIXME
				IdType id = repoClient->createObject(newtype, "anonymous");
//				qDebug() << "\tcreating new item3" << parentItem->id << id << newtype;
				createItem(parentItem, id, newtype);
				endInsertRows();

				return true;
			}
			break;
		case Container:
			{
				// In the beginning check validity
				if (hashTypes[parentItem->id] == "krnnDiagram")
				{
					if (type(parentItem->parent) != Category)
					{
						// FIXME: should put them into avatar
						qDebug() << "cannot put objects here";
						return false;
					}
				}
				qDebug() << "adding to container, action is " << action;
				IdType id = newid;
				qDebug() << "newid: " << newid;

				// drag'n'drop из эксплорера, создаем ссылку на текущий элемент
				bool newElement = (id == "");
				qDebug() << newElement << name;
				CopyType copyType = SYM_LINK_TYPE;

				if (!newElement) {

					QMenu menu;

					QAction *copyAction = menu.addAction("Perform full copy");
					QAction *symlinkAction = menu.addAction("Add symlink");

					if ( QAction *selectedAction = menu.exec(QCursor::pos()) ) {
						// hack with coords (model knows nothing about the GUI event)
						if (selectedAction == copyAction) {
							copyType = FULL_COPY_TYPE;
						} else if (selectedAction == symlinkAction) {
							copyType = SYM_LINK_TYPE;
						} else
							return false;
					} else
						return false;
				}

				RepoTreeItem *newTypedItem;

				// drag'n'drop из палитры, создаем новый элемент
				if (action == Qt::CopyAction && newElement) { // дерево инспектора объектов
					qDebug() << "Qt::CopyAction";

					int typeIndex = findIndex(newtype);
					QModelIndex newTypeIndex = index(typeIndex, 0, QModelIndex());
					RepoTreeItem *typeItem = static_cast<RepoTreeItem*>(newTypeIndex.internalPointer());
					beginInsertRows(newTypeIndex, typeItem->children.size(), typeItem->children.size());
					id = repoClient->createObjectWithParent(newtype,"anonymous", parentItem->id);
					repoClient->setPosition(id, parentItem->id, (int)newPos.x(), (int)newPos.y());
					qDebug() << "\tcreating new item" << rootItem->children.at(typeIndex)->id << id << newtype;
					newTypedItem = createItem(rootItem->children.at(typeIndex), id, newtype);
					if (newtype == "krnnDiagram")
					{
						// inv_avatar will be later
						newTypedItem->is_avatar = true;
					}
					endInsertRows();
				}

				qDebug() << "\tcreating new item2" << parentItem->id << id << newtype;

				// TODO: What the hell?
				Q_ASSERT(action == Qt::CopyAction);
				if (parentItem->id == id) {
					QMessageBox::warning(NULL, tr("Cognitive hazard"),
						 tr("This is a diagram element, not a Klein bottle!"));
					return false;
				}
				foreach (RepoTreeItem *child, parentItem->children) {
					if (child->id == id && id != "") {
						QMessageBox::warning(NULL, tr("Warning!"),
							tr("Making two copies of one element with the same parent is not allowed, use containers instead."));
						return false;
					}
				}
				// дерево инспектора диаграмм
				if (newElement) {
					beginInsertRows(parent,  parentItem->children.size(), parentItem->children.size());
					RepoTreeItem *qq = createItem(parentItem, id, newtype, name);
					if (action == Qt::CopyAction && newtype == "krnnDiagram")
					{
						newTypedItem->inv_avatar = qq;
						qq->has_avatar = true;
						qq->avatar = newTypedItem;
					}
					hashDiagramElements[parentItem->id][id].position = newPos.toPoint();
					endInsertRows();
				}
				else {
					// Если объект подвесили и в корень, и как сына другого объекта,
					// надо дать об этом знать репозиторию, иначе будет #95.
					// TODO: Subject to refactoring.
					if (action == Qt::CopyAction) {
						if (copyType == SYM_LINK_TYPE){
							qDebug() << "SYM_LINK_TYPE";

							Q_ASSERT(oldRow!=-1);
							RepoTreeItem *sourceItem;
							foreach (RepoTreeItem *mbOldParent,hashTreeItems[oldParent]) {
								foreach (RepoTreeItem *mbItem, mbOldParent->children) {
									if ((mbItem->id == id) && (mbItem->row==oldRow)) {
										sourceItem = mbItem;
									}
								}
							}
							Q_ASSERT(sourceItem);

							id = repoClient->copyEntity(newtype, id, parentItem->id, oldParent);
							RepoTreeItem *newItem = copyElement(sourceItem,parent,parentItem);
							copyChildren(sourceItem,index(newItem),newItem);
						}
						else if (copyType == FULL_COPY_TYPE) {
							qDebug() << "FULL_COPY_TYPE";
							beginInsertRows(parent,  parentItem->children.size(), parentItem->children.size());

							id = repoClient->copyEntity(newtype, id, parentItem->id, oldParent, true);

							RepoTreeItem *newItem = createItem(parentItem, id, newtype, name);
							if (hashDiagramElements[oldParent].contains(newid)) {
								hashDiagramElements[parentItem->id][id]=
									 hashDiagramElements[oldParent][newid];
							}
							endInsertRows();
							readChildren(newItem);
						}

					} else {
						beginInsertRows(parent,  parentItem->children.size(), parentItem->children.size());
						createItem(parentItem, id, newtype);
						endInsertRows();
					}
				}
//				foreach( RepoTreeItem *item, hashTreeItems[id])
//					emit dataChanged(index(item),index(item));
			}
			break;
		default:
			return false;
	}

	return false;
}

void RealRepoModel::readChildren( RepoTreeItem *item )
{
	readContainerTable(item);
	beginInsertRows(index(item),0,item->children.size());
	endInsertRows();
	foreach (RepoTreeItem *child, item->children) {
		readChildren(child);
	}
}

RealRepoModel::RepoTreeItem* RealRepoModel::copyElement( RepoTreeItem *item, QPersistentModelIndex newParent, RepoTreeItem *parentItem )
{
	IdType oldParent = item->parent->id;
	IdType id = item->id;

	beginInsertRows(newParent, parentItem->children.size(), parentItem->children.size());

	RepoTreeItem *newItem = createItem(parentItem, id, hashTypes[id], hashNames[id]);
	if (hashDiagramElements[oldParent].contains(id)) {
		hashDiagramElements[parentItem->id][id] = hashDiagramElements[oldParent][id];
	}
	endInsertRows();

	return newItem;
}


void RealRepoModel::copyChildren( RepoTreeItem *item, QPersistentModelIndex newElem, RepoTreeItem *newItem )
{
	foreach (RepoTreeItem *childItem, item->children) {
		IdTypeList newChildren;
		foreach(RepoTreeItem *newChild,newItem->children) {
			newChildren.append(newChild->id);
		}
		if (!newChildren.contains(childItem->id)) {
			RepoTreeItem *newChildItem = copyElement(childItem,newElem,newItem);
			copyChildren(childItem,index(newChildItem),newChildItem);
		}
	}
}


void RealRepoModel::removeChildren( QPersistentModelIndex elem,RepoTreeItem* item )
{
	foreach (RepoTreeItem *childItem, item->children) {
		removeChildren(index(childItem),childItem);
		if (childItem->children.size()) {
			removeChildrenRows(index(childItem),childItem,0,childItem->children.size());
		}
	}
	if (item->children.size()) {
		removeChildrenRows(elem,item,0,item->children.size());
	}
}

void RealRepoModel::changeParent(QPersistentModelIndex elem,QPersistentModelIndex newParent, QPointF newPos)
{
	if (newParent!=parent(elem)) {
		setData(elem, newPos, Unreal::PositionRole);

		RepoTreeItem *item = static_cast<RepoTreeItem*>(elem.internalPointer());
		IdType oldParent = item->parent->id;
		RepoTreeItem *parentItem = static_cast<RepoTreeItem*>(newParent.internalPointer());
		IdType id = item->id;

		id = repoClient->copyEntity(hashTypes[id], id, parentItem->id, oldParent);

		RepoTreeItem *newItem = copyElement(item,newParent,parentItem);
		if (item->has_avatar)
		{
			item->avatar->inv_avatar = newItem;
			newItem->has_avatar = true;
			newItem->avatar = item->avatar;
			item->has_avatar = false;
		}

		copyChildren(item,index(newItem),newItem);

		removeChildren(elem,item);

		removeRows(item->row,1,parent(elem));
	}
}

RealRepoModel::ElementType RealRepoModel::type(const RepoTreeItem *item) const
{
dbg;
	// TODO: Грязный хак. Задача - уметь отличать тип от объекта. У типов строковые
	// ид-шники совпадают с именами типов, у объектов это просто номера.
	if (item->id.toInt() != 0)
		return Container;
	else
	if (item->id != "root")
		return Category;
	else
		return Root;
}

RealRepoModel::ElementType RealRepoModel::type(const QModelIndex &index) const
{
dbg;
	if (index.isValid())
		return type(static_cast<RepoTreeItem *>(index.internalPointer()));
	else
		return Root;
}

void RealRepoModel::cleanupTree(RepoTreeItem *root)
{
dbg;
	foreach (RepoTreeItem *childItem, root->children) {
		cleanupTree(childItem);
		delete childItem;
	}
	root->children.clear();
}

RealRepoModel::RepoTreeItem* RealRepoModel::commonCreateItem( RepoTreeItem *parentItem, IdType const &id, TypeIdType const &type )
{
	//	qDebug() << "\n\n====================";
	RepoTreeItem *item = new RepoTreeItem;
	item->parent = parentItem;
	item->id = id;
	item->is_avatar = false;
	item->has_avatar = false;
	item->orphan_avatar = false;
	item->row = parentItem->children.size();

	// 	qDebug() << "++ id: " << id << ", children: " << item->row+1;
	parentItem->children.append(item);
	hashTreeItems[id].append(item);

	hashTypes[id] = type;

	//	qDebug() << "parent: " << parentItem->id << ", id: " << id;

	return item;
}

RealRepoModel::RepoTreeItem* RealRepoModel::createItem(RepoTreeItem *parentItem, IdType const &id, TypeIdType const &type)
{
dbg;
	RepoTreeItem *item = commonCreateItem(parentItem,id,type);
	hashNames[id] = "anonymous";
	return item;
}

RealRepoModel::RepoTreeItem* RealRepoModel::createItem(RepoTreeItem *parentItem, IdType const &id, TypeIdType const &type, QString name){
dbg;
	RepoTreeItem *item = commonCreateItem(parentItem,id,type);
	hashNames[id] = name;
	return item;
}

void RealRepoModel::updateProperties(IdType const & /*id*/)
{
dbg;
}

void RealRepoModel::updateRootTable()
{
dbg;

/*
	// FIXME: call signals!!!! or rewrite the other way

	QList< QPair<int,int> > list = repoClient->getDiagramsChildCount();
	int i=0;

	while ( i<list.size()  ) {
		hashChildCount[list[i].first] = list[i].second;
		i++;
	}*/
}

bool RealRepoModel::readRootTable()
{
dbg;
	unsigned count = 0;
	qRealTypes::TypeIdTypeList types = repoClient->getAllTypes();
	foreach (TypeIdType type, types) {
		RealType info = repoClient->getTypeById(type);
		RepoTreeItem *item = new RepoTreeItem;
		// qDebug() << "root table: " << info.getId() << count << info.getName() << info.getDescription() << item;
		item->parent = rootItem;
		item->row = count;
		item->is_avatar = false;
		item->has_avatar = false;
		item->orphan_avatar = false;
		item->id = info.getId();

		hashNames[item->id] = info.getName();

		hashTreeItems[item->id].append(item);

		rootItem->children.append(item);
		++count;
	}

//	qDebug() << "root children" << rootItem->children.size();
	return true;
}

void RealRepoModel::readCategoryTable(RepoTreeItem * parent)
{
dbg;
	// Select all elements of the same type as the parent

	IdTypeList ids = repoClient->getObjectsListByType(parent->id);
	qDebug() << ids;

//	qDebug() << "searching for type " << parent->id << ", found " << ids.size() << "elements" << ids;
	unsigned count = 0;
	foreach (IdType id, ids) {
		// TODO: не должно по идее быть так.
		if (id == "")
			continue;
		QString data = repoClient->getObjectData(id);
//		qDebug() << "element" << i << data;
		RepoTreeItem *item = new RepoTreeItem;
		item->parent = parent;
		item->id = id;
		item->row = count;
		item->is_avatar = false;
		item->has_avatar = false;
		item->orphan_avatar = false;
		parent->children.append(item);
		hashTreeItems[item->id].append(item);

		// Some elements may be already added here - user may open diagrams before us.
		if (!hashNames.contains(item->id)) {
			hashNames[item->id] = data.section("\t", 1, 1);
			hashTypes[item->id] = parent->id;
		}
		++count;
	}
}

void RealRepoModel::readContainerTable(RepoTreeItem * root)
{
dbg;
//	qDebug() << "================ READING DIAGRAM =======================";
	// If it is not a non-top-level avatar of diagram, do not read children
	RepoTreeItem *diagramCategory = hashTreeItems["krnnDiagram"].first();
	foreach (RepoTreeItem *idItem, diagramCategory->children)
	{
		if (idItem->id == root->id && root != idItem)
		{
			idItem->is_avatar = true;
			idItem->inv_avatar = root;
			root->avatar = idItem;
			root->has_avatar = true;
			return;
		}
	}

	IdTypeList idChildren;
	foreach (RepoTreeItem *idItem, hashTreeItems[root->id]) {
		foreach(RepoTreeItem *idChild,idItem->children) {
			if (!idChildren.contains(idChild->id)) {
				idChildren.append(idChild->id);
			}
		}
	}

	if (idChildren.size()) {
		int i = 0;
		foreach (IdType childId, idChildren) {
			RepoTreeItem *item = new RepoTreeItem;
			item->parent = root;
			item->row = i++;
			item->id = childId;
			item->is_avatar = false;
			item->has_avatar = false;
			item->orphan_avatar = false;

			root->children.append(item);
			hashTreeItems[item->id].append(item);
		}
	} else {
		QStringList children = repoClient->getChildren(root->id).split("\t", QString::SkipEmptyParts);
//		qDebug() << children.size() << children;
		for (int i = 0; i < children.size(); ++i) {
			IdType id = children[i];
			QString data = repoClient->getObjectData(id);
//			int _type = data.section("\t",2,2).toInt();
			QString coordinates = repoClient->getPosition(id, root->id);

			RepoTreeItem *item = new RepoTreeItem;
			item->parent = root;
			item->row = i;
			item->id = id;
			item->is_avatar = false;
			item->has_avatar = false;
			item->orphan_avatar = false;

			hashNames[item->id] = data.section("\t", 1, 1);
			hashTypes[item->id] = data.section("\t", 2, 2);

			root->children.append(item);
			hashTreeItems[item->id].append(item);

			hashDiagramElements[root->id][item->id].position =
				QPoint(coordinates.section(";", 0, 0).toInt(), coordinates.section(";", 1, 1).toInt());

			// FIXME: parse some better way
			QPolygon newConfig;
			QStringList pointList = repoClient->getConfiguration(id, root->id).split(';', QString::SkipEmptyParts);
			foreach (QString pointData, pointList) {
				QStringList coords = pointData.split(QRegExp("\\(|\\)|,"), QString::SkipEmptyParts);
				newConfig << QPoint(coords.at(0).toInt(), coords.at(1).toInt());
			}
			hashDiagramElements[root->id][item->id].configuration = newConfig;
		}

	}
}

void RealRepoModel::undo()
{
	qDebug() << "undo";
	undoStack->undo();
}

void RealRepoModel::redo()
{
	qDebug() << "redo";
	undoStack->redo();
}

void RealRepoModel::showCommandList()
{
	undoView->show();
}

unsigned RealRepoModel::findIndex(TypeIdType const &id) const
{
	unsigned count = 0;
	foreach (RepoTreeItem *item, rootItem->children) {
		if (item->id == id)
			return count;
		++count;
	}
	Q_ASSERT(!"Element not found in the root table");
	return count;
}

QModelIndex RealRepoModel::getDiagramCategoryIndex() const
{
	// Выглядит как хак, но сама по себе эта функция - хак и не должна пережить
	// введение плагинов.
	return index(findIndex("krnnDiagram"), 0, QModelIndex());
}

QModelIndex RealRepoModel::getAvatarIndex(const QModelIndex &ind) const
{
	RepoTreeItem *item = static_cast<RepoTreeItem *>(ind.internalPointer());
	if ((!item) || (!item->has_avatar)) return QModelIndex();

	return index(item->avatar);
}

void RealRepoModel::runTestQueries()
{

	qDebug() << "Getting types by metatype 'object'";
	TypeIdTypeList result = repoClient->getTypesByMetaType(object);
	qDebug() << "Result: " << result;

	qDebug() << "Getting types by metatype 'link'";
	result = repoClient->getTypesByMetaType(qRealTypes::link);
	qDebug() << "Result: " << result;

	qDebug() << "Getting types by metatype 'dataType'";
	result = repoClient->getTypesByMetaType(dataType);
	qDebug() << "Result: " << result;

	qDebug() << "Getting types by metatype 'rawType'";
	result = repoClient->getTypesByMetaType(rawType);
	qDebug() << "Result: " << result;

	qDebug() << "Getting type ID by name for non-existing (for now!) type 'Cthulhu fhtagn!'";
	TypeIdType typeIdTypeResult = repoClient->getTypeIdByName("Cthulhu fhtagn!");
	qDebug() << "Result: " << typeIdTypeResult;

	qDebug() << "Done";
}
