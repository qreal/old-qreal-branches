/** @file editorscene.cpp
 * 	@brief Сцена для отрисовки объектов
 * */
#include "editorscene.h"
#include "editorview.h"
#include "uml_nodeelement.h"
#include "uml_guiobjectfactory.h"
#include "realreporoles.h"
#include "mainwindow.h"

#include <QGraphicsTextItem>
#include <QtGui>
#include "../common/classes.h"

extern MainWindow *window;

EditorScene::EditorScene(QObject *parent) : QGraphicsScene(parent)
{
	setItemIndexMethod(NoIndex);
	mv_iface = new EditorScenePrivate::EditorViewMViface(this);
}

EditorScene::~EditorScene()
{
	delete mv_iface;
}

void EditorScene::dragEnterEvent ( QGraphicsSceneDragDropEvent * event )
{
	const QMimeData *mimeData = event->mimeData();
	if (mimeData->hasFormat("application/x-real-uml-data"))
		QGraphicsScene::dragEnterEvent(event);
	else
		event->ignore();
}

void EditorScene::dragMoveEvent ( QGraphicsSceneDragDropEvent * event )
{
	Q_UNUSED(event);
}

void EditorScene::dragLeaveEvent ( QGraphicsSceneDragDropEvent * event )
{
	Q_UNUSED(event);
}

void EditorScene::dropEvent ( QGraphicsSceneDragDropEvent * event )
{
	// Transform mime data to include coordinates.
	const QMimeData *mimeData = event->mimeData();
	QByteArray itemData = mimeData->data("application/x-real-uml-data");

	QDataStream in_stream(&itemData, QIODevice::ReadOnly);

	IdType uuid = "", oldparent = "";
	TypeIdType type_id = "";
	QString name;
	QPointF pos;

	in_stream >> uuid;
	in_stream >> type_id;
	in_stream >> oldparent;
	in_stream >> name;
	in_stream >> pos;

	QByteArray newItemData;
	QDataStream stream(&newItemData, QIODevice::WriteOnly);

	UML::Element *newParent = 0;

	UML::Element *e = UML::GUIObjectFactory(type_id);

	if (dynamic_cast<UML::NodeElement *>(e)) {
		// Do not care if casting fails
		newParent = dynamic_cast<UML::NodeElement *>(itemAt(event->scenePos()));
	}

	if (e) {
		delete e;
	}

	stream << uuid;				// uuid
	stream << type_id;			// type
	stream << oldparent;
	stream << name;

	if (!newParent) {
		stream << event->scenePos();
	} else {
		stream << newParent->mapToItem(newParent, newParent->mapFromScene(event->scenePos()));
	}

	QMimeData *newMimeData = new QMimeData;
	newMimeData->setData("application/x-real-uml-data", newItemData);

	if (newParent) {
		mv_iface->model()->dropMimeData( newMimeData, event->dropAction(),
						 mv_iface->model()->rowCount(newParent->index()), 0, newParent->index() );
	} else {
		mv_iface->model()->dropMimeData( newMimeData, event->dropAction(),
						 mv_iface->model()->rowCount(mv_iface->rootIndex()), 0, mv_iface->rootIndex() );
	}
	delete newMimeData;
	updateLinks();
}

void EditorScene::keyPressEvent( QKeyEvent * event )
{
	if (dynamic_cast<QGraphicsTextItem*>(this->focusItem())) {
		// Forward event to text editor
		QGraphicsScene::keyPressEvent(event);
	} else if (event->key() == Qt::Key_Delete) {
		// Delete selected elements from scene
		deleteCurrentSelection();
	} else
		QGraphicsScene::keyPressEvent(event);
}

void EditorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	// Let scene update selection and perform other operations
	QGraphicsScene::mousePressEvent(event);

	if (event->button() != Qt::RightButton)
		return;

	// Did we click on element?
	UML::Element *e = dynamic_cast<UML::Element*>(itemAt(event->scenePos()));
	if (!e) return;

	// Skip edges
	if (dynamic_cast<UML::EdgeElement*>(e))
		return;

	// Menu belongs to scene handler because it can delete elements.
	// We cannot not allow elements to commit suicide.
	QMenu menu;
	QAction *delAction = menu.addAction("Delete");
	connect(delAction, SIGNAL(triggered()), this, SLOT(deleteCurrentSelection()));
	menu.exec(event->screenPos());
	delete delAction;
}

void EditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	UML::ElementTitle *t = dynamic_cast<UML::ElementTitle*>(itemAt(event->scenePos()));
	// Double click on title activates it
	if (event->button() == Qt::LeftButton && t)
		t->setTextInteractionFlags(Qt::TextEditorInteraction);
	QGraphicsScene::mouseDoubleClickEvent(event);
}

void EditorScene::updateLinks()
{
	foreach (QGraphicsItem *item, items())
	{
		UML::EdgeElement *e = dynamic_cast<UML::EdgeElement *>(item);
		if (e)
			e->makeRightAngle();
	}
}

void EditorScene::setModel(QAbstractItemModel *model)
{
	mv_iface->setModel(model);
}

void EditorScene::setRootIndex(const QModelIndex &index)
{
	mv_iface->setRootIndex(index);
}

void EditorScene::deleteCurrentSelection()
{
	foreach (QGraphicsItem *item, selectedItems()) {
		if (UML::Element * elem = dynamic_cast < UML::Element * >(item))
			if (elem->index().isValid())
			{
			try{
				mv_iface->model()->removeRow(elem->index().row(), elem->index().parent());
			}
			catch (QString e)
			{
				QMessageBox::warning(window, tr("Operation aborted"),
					 tr("Repository can not delete this element."));
			}
			}
	}
}

/* {{{ EditorViewMViface*/
namespace EditorScenePrivate {
EditorViewMViface::EditorViewMViface(EditorScene *scene)
	: QAbstractItemView(0)
{
	this->scene = scene;
}

void EditorViewMViface::reset()
{
	items.clear();
	scene->clear();

	// so that our diagram be nicer
	QGraphicsRectItem *rect = scene->addRect(QRect(-1000,-1000,2000,2000));
	scene->removeItem(rect);
	delete rect;

	if ( model() )
		rowsInserted(rootIndex(), 0, model()->rowCount(rootIndex()) - 1 );
}

void EditorViewMViface::setRootIndex(const QModelIndex &index)
{
	QAbstractItemView::setRootIndex(index);
	reset();
}

void EditorViewMViface::rowsInserted(const QModelIndex &parent, int start, int end)
{
	qDebug() << "========== rowsInserted" << parent << start << end;

	if (parent == QModelIndex() || parent.parent() == QModelIndex())
		return;

	qDebug() << "rowsInserted: adding items" << parent;
	for (int row = start; row <= end; ++row) {
		QPersistentModelIndex current = model()->index(row, 0, parent);
		IdType uuid = current.data(Unreal::IdRole).toString();
		TypeIdType type = current.data(Unreal::TypeRole).toString();

		IdType parent_uuid = "";
		if (parent != rootIndex())
			parent_uuid = parent.data(Unreal::IdRole).toString();

		qDebug() << uuid << type;

		if (uuid == "")
			continue;

		if (UML::Element *e = UML::GUIObjectFactory(type)) {
			e->setIndex(current);
			if (parent_uuid != "")
				e->setParentItem(items[parent]);
			else
				scene->addItem(e);
			items[current] = e;
			e->updateData();
			e->connectToPort();
		}

		if (model()->hasChildren(current)) {
			rowsInserted(current, 0, model()->rowCount(current) - 1);
		}
	}

	QAbstractItemView::rowsInserted(parent, start, end);
}

void EditorViewMViface::rowsAboutToBeRemoved ( const QModelIndex & parent, int start, int end )
{
	for (int row = start; row <= end; ++row) {
		QModelIndex curr = model()->index(row, 0, parent);
		scene->removeItem(items[curr]);
		delete items[curr];
		items.remove(curr);
	}

	QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

void EditorViewMViface::dataChanged(const QModelIndex &topLeft,
	const QModelIndex &bottomRight)
{
	for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
		QModelIndex curr = topLeft.sibling(row, 0);
		if (items.contains(curr)) {
			Q_ASSERT(items[curr] != NULL);
			items[curr]->updateData();
		}
		else
			rowsInserted(topLeft.parent(),row,row);
	}
}
}
/* }}} */
