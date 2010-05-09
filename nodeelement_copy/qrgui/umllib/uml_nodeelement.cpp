﻿/** @file uml_nodeelement.cpp
 * 	@brief Класс, представляющий объект на диаграмме
 * */
#include "uml_nodeelement.h"

#include <QtGui/QStyle>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtGui/QMessageBox>
#include <QtGui/QTextCursor>
#include <QtGui/QToolTip>
#include <QtCore/QDebug>
#include <QtCore/QUuid>

#include "../model/model.h"
#include "../view/editorviewscene.h"

#include <math.h>

using namespace UML;
using namespace qReal;

NodeElement::NodeElement(ElementImpl* impl)
	: mSwitchGrid(false), mSwitchGridAction("Switch on/off grid", this), mPortsVisible(false), mDragState(None),mEmbeddedLinker(NULL), mElementImpl(impl)
{
	setAcceptHoverEvents(true);
	setFlag(ItemClipsChildrenToShape, false);
	mPortRenderer = new SdfRenderer();
	mElementImpl->init(mContents, mPointPorts, mLinePorts, mTitles, mPortRenderer);
	foreach (ElementTitle *title, mTitles)
		title->setParentItem(this);
	connect(&mSwitchGridAction, SIGNAL(triggered()), this, SLOT(switchGrid()));
}

NodeElement::~NodeElement()
{
	foreach (EdgeElement *edge, mEdgeList)
		edge->removeLink(this);

	foreach (ElementTitle *title, mTitles)
		delete title;

	delete mPortRenderer;
	delete mElementImpl;
}

void NodeElement::setName(QString value)
{
	QAbstractItemModel *im = const_cast<QAbstractItemModel *>(mDataIndex.model());
	im->setData(mDataIndex, value, Qt::DisplayRole);
}

void NodeElement::setGeometry(QRectF const &geom)
{
	prepareGeometryChange();
	setPos(geom.topLeft());
	if (geom.isValid())
		mContents = geom.translated(-geom.topLeft());
	mTransform.reset();
	mTransform.scale(mContents.width(), mContents.height());
	adjustLinks();

	foreach (ElementTitle *title, mTitles)
		title->setTextWidth(mContents.width() - title->pos().x());
}

void NodeElement::adjustLinks()
{
	foreach (EdgeElement *edge, mEdgeList)
		edge->adjustLink();

	foreach (QGraphicsItem *child, childItems()) {
		NodeElement *element = dynamic_cast<NodeElement*>(child);
		if (element)
			element->adjustLinks();
	}
}

void NodeElement::storeGeometry()
{
	QRectF tmp = mContents;
	model::Model *itemModel = const_cast<model::Model*>(static_cast<model::Model const *>(mDataIndex.model()));
	itemModel->setData(mDataIndex, pos(), roles::positionRole);
	itemModel->setData(mDataIndex, QPolygon(tmp.toAlignedRect()), roles::configurationRole);
}

QList<ContextMenuAction*> NodeElement::contextMenuActions()
{
	QList<ContextMenuAction*> result;
	result.push_back(&mSwitchGridAction);
	return result;
}

/*Удаляем все лишние прямые*/
void NodeElement::delUnusedLines()
{
	for (int i = mLines.size() - 1; i >= 0; i--) {
		mLines[i]->hide();
		scene()->removeItem(mLines[i]);
		mLines.pop_back();
	}
}

/*Рисуем горизонтальную линию*/
void NodeElement::drawLineY(qreal pointY)
{
	bool lineIsFound = false;
	QLineF line(this->scene()->sceneRect().x(), pointY, this->scene()->sceneRect().x() + this->scene()->sceneRect().width(), pointY);
	foreach (QGraphicsLineItem* lineItem, mLines) {
		if (lineItem->line().y1() == line.y1() && lineItem->line().y2() == line.y2())
			lineIsFound = true;
	}
	if (!lineIsFound)
		mLines.push_back(scene()->addLine(line, QPen(Qt::black, 0.25, Qt::DashLine)));
}

/*Рисуем вертикальную линию*/
void NodeElement::drawLineX(qreal pointX)
{
	bool lineIsFound = false;
	QLineF line(pointX, this->scene()->sceneRect().y(), pointX, this->scene()->sceneRect().y() + this->scene()->sceneRect().height());
	foreach (QGraphicsLineItem* lineItem, mLines) {
		if (lineItem->line().x1() == line.x1() && lineItem->line().x2() == line.x2())
			lineIsFound = true;
	}
	if (!lineIsFound)
		mLines.push_back(scene()->addLine(line, QPen(Qt::black, 0.25, Qt::DashLine)));
}

/*Проверяем, надо ли делать "прыжок" к вертикальной линии. Если да, то делаем его*/
bool NodeElement::makeJumpX(qreal deltaX, qreal radiusJump, qreal pointX)
{
	if (deltaX <= radiusJump) {
		setX(pointX - boundingRect().x());
		return true;
	}
	return false;
}

/*Проверяем, надо ли делать "прыжок" к горизонтальной линии. Если да, то делаем его*/
bool NodeElement::makeJumpY(qreal deltaY, qreal radiusJump, qreal pointY)
{
	if (deltaY <= radiusJump) {
		setY(pointY - boundingRect().y());
		return true;
	}
	return false;
}

/*"Строим" вертикальную прямую, т.е. рисуем её и проверяем, нужен ли "прыжок" объекта к ней*/
void NodeElement::buildLineX(qreal deltaX, qreal radius, bool doAlways, qreal radiusJump, qreal pointX, qreal correctionX, qreal &myX1, qreal &myX2)
{
	if (deltaX <= radius || doAlways) {
		drawLineX(pointX);
		if (makeJumpX(deltaX, radiusJump, pointX - correctionX)) {
			myX1 = recountX1();
			myX2 = recountX2(myX1);
		}
	}
}

/*"Строим" горизонтальную прямую, т.е. рисуем её и проверяем, нужен ли "прыжок" объекта к ней*/
void NodeElement::buildLineY(qreal deltaY, qreal radius, bool doAlways, qreal radiusJump, qreal pointY, qreal correctionY, qreal &myY1, qreal &myY2)
{
	if (deltaY <= radius || doAlways) {
		drawLineY(pointY);
		if (makeJumpY(deltaY, radiusJump, pointY - correctionY)) {
			myY1 = recountY1();
			myY2 = recountY2(myY1);
		}
	}
}

/*Пересчитываем X1*/
qreal NodeElement::recountX1()
{
	return scenePos().x() + boundingRect().x();
}

/*Пересчитываем X2*/
qreal NodeElement::recountX2(qreal myX1)
{
	return myX1 + boundingRect().width();
}

/*Пересчитываем Y1*/
qreal NodeElement::recountY1()
{
	return scenePos().y() + boundingRect().y();
}

/*Пересчитываем Y2*/
qreal NodeElement::recountY2(qreal myY1)
{
	return myY1 + boundingRect().height();
}

/*Осуществляем вертикальное движение объекта по заданной сетке*/
void NodeElement::makeGridMovingX(qreal myX, int koef, int indexGrid)
{
	int oneKoef = 0;
	if (koef != 0)
		oneKoef = koef / fabs(koef);
	if(fabs(fabs(myX) - fabs(koef) * indexGrid) <= indexGrid / 2)
		setX(koef * indexGrid);
	else if(fabs(fabs(myX) - (fabs(koef) + 1)* indexGrid) < indexGrid / 2)
		setX((koef + oneKoef) * indexGrid);
}

/*Осуществляем горизонтальное движение объекта по заданной сетке*/
void NodeElement::makeGridMovingY(qreal myY, int koef, int indexGrid)
{
	int oneKoef = 0;
	if (koef != 0)
		oneKoef = koef / fabs(koef);
	if(fabs(fabs(myY) - fabs(koef) * indexGrid) <= indexGrid / 2)
		setY(koef * indexGrid);
	else if(fabs(fabs(myY) - (fabs(koef) + 1)* indexGrid) < indexGrid / 2)
		setY((koef + oneKoef) * indexGrid);
}

//события мышки

void NodeElement::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
	if (mEmbeddedLinker == NULL)
	{
		mEmbeddedLinker = new EmbeddedLinker();
		mEmbeddedLinker->setMaster(this);
		scene()->addItem(mEmbeddedLinker);
	}
	mEmbeddedLinker->moveTo(event->pos());
	mEmbeddedLinker->setCovered(true);

	if (isSelected())
	{
		if (QRectF(mContents.topLeft(), QSizeF(4, 4)).contains(event->pos()))
			mDragState = TopLeft;
		else if (QRectF(mContents.topRight(), QSizeF(-4, 4)).contains(event->pos()))
			mDragState = TopRight;
		else if (QRectF(mContents.bottomRight(), QSizeF(-12, -12)).contains(event->pos()))
			mDragState = BottomRight;
		else if (QRectF(mContents.bottomLeft(), QSizeF(4, -4)).contains(event->pos()))
			mDragState = BottomLeft;
		else
			Element::mousePressEvent(event);
	}
	else
		Element::mousePressEvent(event);

	if (event->button() == Qt::RightButton)
		event->accept();

	setZValue(1);

}

void NodeElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	mEmbeddedLinker->setCovered(false);
	if (mDragState == None)
	{
		Element::mouseMoveEvent(event);
	} else {
		QRectF newContents = mContents;
		switch (mDragState)
		{
		case TopLeft:
			newContents.setTopLeft(event->pos());
			break;
		case Top:
			newContents.setTop(event->pos().y());
			break;
		case TopRight:
			newContents.setTopRight(event->pos());
			break;
		case Left:
			newContents.setLeft(event->pos().x());
			break;
		case Right:
			newContents.setRight(event->pos().x());
			break;
		case BottomLeft:
			newContents.setBottomLeft(event->pos());
			break;
		case Bottom:
			newContents.setBottom(event->pos().y());
			break;
		case BottomRight:
			newContents.setBottomRight(event->pos());
			break;
		case None:
			break;
		}

		if (event->modifiers() & Qt::ShiftModifier)
		{
			qreal size = std::max(newContents.width(), newContents.height());
			newContents.setWidth(size);
			newContents.setHeight(size);
		}

		newContents.translate(pos());
		if (!((newContents.width() < 10) || (newContents.height() < 10)))
			setGeometry(newContents);
	}

	qreal myX1 = scenePos().x() + boundingRect().x();
	qreal myY1 = scenePos().y() + boundingRect().y();

	if (mSwitchGrid) {
		int indexGrid = 10; //ширина м/д линиями решетки сцены
		int koefX = ((int) myX1) / indexGrid;
		int koefY = ((int) myY1) / indexGrid;

		makeGridMovingX(myX1, koefX, indexGrid);
		makeGridMovingY(myY1, koefY, indexGrid);

		myX1 = scenePos().x() + boundingRect().x();
		myY1 = scenePos().y() + boundingRect().y();
	}

	qreal myX2 = myX1 + boundingRect().width();
	qreal myY2 = myY1 + boundingRect().height();

	qreal radius = 20;
	qreal radiusJump = 10;

	QList<QGraphicsItem *> list = scene()->items();
	delUnusedLines();
	foreach (QGraphicsItem *graphicsItem, list) {
		NodeElement* item = dynamic_cast<NodeElement*>(graphicsItem);
		if (item == NULL)
			continue;
		QPointF point = item->scenePos();
		qreal pointX1 = point.x() + item->boundingRect().x();
		qreal pointY1  = point.y() + item->boundingRect().y();
		qreal pointX2 = pointX1  + item->boundingRect().width();
		qreal pointY2  = pointY1 + item->boundingRect().height();

		if (pointX1 != myX1 || pointY1 != myY1) {
			qreal deltaY1 = fabs(pointY1 - myY1);
			qreal deltaY2 = fabs(pointY2 - myY2);
			qreal deltaX1 = fabs(pointX1 - myX1);
			qreal deltaX2 = fabs(pointX2 - myX2);
			if (deltaY1 <= radius || deltaY2 <= radius) {
				buildLineY(deltaY1, radius, true, radiusJump, pointY1, 0, myY1, myY2);
				buildLineY(deltaY2, radius, true, radiusJump, pointY2, boundingRect().height(), myY1, myY2);
			}
			if (deltaX1 <= radius || deltaX2 <= radius) {
				buildLineX(deltaX1, radius, true, radiusJump, pointX1, 0, myX1, myX2);
				buildLineX(deltaX2, radius, true, radiusJump, pointX2, boundingRect().width(), myX1, myX2);
			}
			buildLineY(fabs(pointY1 - myY2), radius, false, radiusJump, pointY1, boundingRect().height(), myY1, myY2);
			buildLineX(fabs(pointX1 - myX2), radius, false, radiusJump, pointX1, boundingRect().width(), myX1, myX2);
			buildLineY(fabs(pointY2 - myY1), radius, false, radiusJump, pointY2, 0, myY1, myY2);
			buildLineX(fabs(pointX2 - myX1), radius, false, radiusJump, pointX2, 0, myX1, myX2);
		}
	}
}

void NodeElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	delUnusedLines();
	mContents = mContents.normalized();
	storeGeometry();
	mEmbeddedLinker->setCovered(true);

	if (mDragState == None)
		Element::mouseReleaseEvent(event);

	NodeElement *newParent = getNodeAt(event->scenePos());
	EditorViewScene *evScene = dynamic_cast<EditorViewScene *>(scene());
	model::Model *itemModel = const_cast<model::Model*>(static_cast<const model::Model*>(mDataIndex.model()));
	if (newParent) {
		itemModel->changeParent(mDataIndex, newParent->mDataIndex,
								mapToItem(evScene->getElemByModelIndex(newParent->mDataIndex), mapFromScene(scenePos())));
	} else
		itemModel->changeParent(mDataIndex, evScene->rootItem(), scenePos());

	mDragState = None;
	setZValue(0);
}

//события наведения мыши

void NodeElement::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	if (!isSelected())
		return;
	if (mEmbeddedLinker == NULL)
	{
		mEmbeddedLinker = new EmbeddedLinker();
		mEmbeddedLinker->setMaster(this);
		scene()->addItem(mEmbeddedLinker);
	}
	mEmbeddedLinker->moveTo(event->pos());
	mEmbeddedLinker->setCovered(true);
}

void NodeElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	if (!isSelected())
		return;
	mEmbeddedLinker->moveTo(event->pos());
}

void NodeElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event);
	if (!isSelected())
		return;
	mEmbeddedLinker->setCovered(false);
}

QVariant NodeElement::itemChange(GraphicsItemChange change, const QVariant &value)
{
	switch (change) {
	case ItemPositionHasChanged:
		adjustLinks();
		return value;
	default:
		return QGraphicsItem::itemChange(change, value);
	}
}

QRectF NodeElement::contentsRect() const
{
	return mContents;
}

QRectF NodeElement::boundingRect() const
{
	return mContents.adjusted(-kvadratik, -kvadratik, kvadratik, kvadratik);
}

void NodeElement::updateData()
{
	Element::updateData();
	if (mMoving == 0) {
		QPointF newpos = mDataIndex.data(roles::positionRole).toPointF();
		QPolygon newpoly = mDataIndex.data(roles::configurationRole).value<QPolygon>();
		QRectF newRect; // Use default ((0,0)-(0,0))
		// QPolygon::boundingRect is buggy :-(
		if (!newpoly.isEmpty()) {
			int minx = newpoly[0].x();
			int miny = newpoly[0].y();
			int maxx = newpoly[0].x();
			int maxy = newpoly[0].y();;
			for (int i = 1; i < newpoly.size(); ++i) {
				if (minx > newpoly[i].x())
					minx = newpoly[i].x();
				if (maxx < newpoly[i].x())
					maxx = newpoly[i].x();
				if (miny > newpoly[i].y())
					miny = newpoly[i].y();
				if (maxy < newpoly[i].y())
					maxy = newpoly[i].y();
			}
			newRect = QRectF(QPoint(minx, miny), QSize(maxx - minx, maxy - miny));
		}
		setGeometry(newRect.translated(newpos));
	}

	mElementImpl->updateData(this);
	update();
}

static int portId(qreal id)
{
	int iid = qRound(id);
	if (id < 1.0 * iid)
		return iid - 1;
	else
		return iid;
}

const QPointF NodeElement::getPortPos(qreal id) const
{
	int iid = portId(id);

	if (id < 0.0)
		return QPointF(0, 0);
	if (id < mPointPorts.size())
		return mTransform.map(mPointPorts[iid]);
	if (id < mPointPorts.size() + mLinePorts.size())
		return newTransform(mLinePorts.at(iid - mPointPorts.size())).pointAt(id - 1.0 * iid);
	else
		return QPointF(0, 0);
}

QLineF NodeElement::newTransform(const StatLine& port) const
{
	float x1 = 0.0;
	float x2 = 0.0;
	float y1 = 0.0;
	float y2 = 0.0;

	if (port.prop_x1)
		x1 = port.line.x1() * 100;
	else
		x1 = port.line.x1() * contentsRect().width();

	if (port.prop_y1)
		y1 = port.line.y1() * 100;
	else
		y1 = port.line.y1() * contentsRect().height();

	if (port.prop_x2)
		x2 = port.line.x2() * 100;
	else
		x2 = port.line.x2() * contentsRect().width();

	if (port.prop_y2)
		y2 = port.line.y2() * 100;
	else
		y2 = port.line.y2() * contentsRect().height();

	return QLineF(x1, y1, x2, y2);
}

qreal NodeElement::minDistanceFromLinePort(int linePortNumber, const QPointF &location) const
{
	QLineF linePort = newTransform(mLinePorts[linePortNumber]);
	qreal a = linePort.length();
	qreal b = QLineF(linePort.p1(), location).length();
	qreal c = QLineF(linePort.p2(), location).length();

	qreal nearestPointOfLinePort = getNearestPointOfLinePort(linePortNumber, location);
	if ((nearestPointOfLinePort < 0) || (nearestPointOfLinePort > 0.9999))
		return qMin(b, c);
	else {
		qreal p = (a + b + c) / 2;
		qreal triangleSquare = sqrt(p*(p-a)*(p-b)*(p-c));
		qreal minDistance = 2 * triangleSquare / a;
		return minDistance;
	}
}

qreal NodeElement::distanceFromPointPort(int pointPortNumber, const QPointF &location) const
{
	return QLineF(mTransform.map(mPointPorts[pointPortNumber]), location).length();
}

qreal NodeElement::getNearestPointOfLinePort(int linePortNumber, const QPointF &location) const
{
	qreal nearestPointOfLinePort = 0;
	QLineF nearestLinePort = newTransform(mLinePorts[linePortNumber]);
	if (nearestLinePort.x1() == nearestLinePort.x2()) {
		nearestPointOfLinePort = (location.y() - nearestLinePort.y1()) / (nearestLinePort.y2() - nearestLinePort.y1());
	} else if (nearestLinePort.y1() == nearestLinePort.y2()) {
		nearestPointOfLinePort = (location.x() - nearestLinePort.x1()) / (nearestLinePort.x2() - nearestLinePort.x1());
	} else {
		qreal k = (nearestLinePort.y2() - nearestLinePort.y1()) / (nearestLinePort.x2() - nearestLinePort.x1());
		qreal b2 = location.y() + 1 / k * location.x();
		qreal b = nearestLinePort.y1() - k * nearestLinePort.x1();
		qreal x3 = k / (1 + k * k) * (b2 - b);
		nearestPointOfLinePort = (x3 - nearestLinePort.x1()) / (nearestLinePort.x2() - nearestLinePort.x1());
	}
	return nearestPointOfLinePort;
}

qreal NodeElement::getPortId(const QPointF &location) const
{
	for (int i = 0; i < mPointPorts.size(); ++i) {
		if (QRectF(mTransform.map(mPointPorts[i]) - QPointF(kvadratik, kvadratik),
				   QSizeF(kvadratik * 2, kvadratik * 2)).contains(location))
		{
			return 1.0 * i;
		}
	}

	for (int i = 0; i < mLinePorts.size(); i++) {
		QPainterPathStroker ps;
		ps.setWidth(kvadratik);

		QPainterPath path;
		path.moveTo(newTransform(mLinePorts[i]).p1());
		path.lineTo(newTransform(mLinePorts[i]).p2());

		path = ps.createStroke(path);
		if (path.contains(location))
			return (1.0 * (i + mPointPorts.size()) + qMin(0.9999,
														  QLineF(QLineF(newTransform(mLinePorts[i])).p1(), location).length()
														  / newTransform(mLinePorts[i]).length() ) );
	}

	qreal minDistance = 0;
	int numMinDistance = -1;
	if (mPointPorts.size() != 0)
	{
		numMinDistance = 0;
		minDistance = distanceFromPointPort(0, location);
		for(int i = 1; i < mPointPorts.size(); i++)
		{
			qreal currentDistance = distanceFromPointPort(i, location);
			if (currentDistance < minDistance)
			{
				numMinDistance = i;
				minDistance = currentDistance;
			}
		}
	}

	if (mLinePorts.size() != 0)
	{
		bool linePort = false;
		if (numMinDistance == -1)
		{
			numMinDistance = 0;
			minDistance = minDistanceFromLinePort(0, location);
			linePort = true;
		}
		for(int i = 0; i < mLinePorts.size(); i++)
		{
			qreal currentDistance = minDistanceFromLinePort(i, location);
			if (currentDistance < minDistance)
			{
				numMinDistance = i;
				minDistance = currentDistance;
				linePort = true;
			}
		}
		if (linePort)
		{
			qreal nearestPointOfLinePort = getNearestPointOfLinePort(numMinDistance, location);
			if (nearestPointOfLinePort < 0)
			{
				nearestPointOfLinePort = 0;
			} else if (nearestPointOfLinePort > 0.9999)
			{
				nearestPointOfLinePort = 0.9999;
			}
			return 1.0 * (numMinDistance + nearestPointOfLinePort + mPointPorts.size());
		} else
		{
			return 1.0 * numMinDistance;
		}
	} else if (numMinDistance >= 0)
	{
		return 1.0 * numMinDistance;
	}
	return -1.0;
}

void NodeElement::setPortsVisible(bool value)
{
	prepareGeometryChange();
	mPortsVisible = value;
}

NodeElement *NodeElement::getNodeAt( const QPointF &position )
{
	foreach( QGraphicsItem *item, scene()->items(position) ) {
		NodeElement *e = dynamic_cast<NodeElement *>(item);
		if (( e )&&(item!=this))
			return e;
	}
	return 0;
}

void NodeElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *style, QWidget *widget)
{
	if (mElementImpl->hasPorts())
		paint(painter, style, widget, mPortRenderer);
	else
		paint(painter, style, widget, 0);
	mElementImpl->paint(painter, mContents);
}

void NodeElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget*, SdfRenderer* portRenderer)
{
	if (option->levelOfDetail >= 0.5)
	{
		if (option->state & QStyle::State_Selected)
		{
			painter->save();

			QBrush b;
			b.setColor(Qt::blue);
			b.setStyle(Qt::SolidPattern);
			painter->setBrush(b);
			painter->setPen(Qt::blue);

			painter->drawRect(QRectF(mContents.topLeft(),QSizeF(4,4)));
			painter->drawRect(QRectF(mContents.topRight(),QSizeF(-4,4)));
			painter->drawRect(QRectF(mContents.bottomLeft(),QSizeF(4,-4)));

			painter->translate(mContents.bottomRight());
			painter->drawLine(QLineF(-4,0,0,-4));
			painter->drawLine(QLineF(-8,0,0,-8));
			painter->drawLine(QLineF(-12,0,0,-12));

			painter->restore();
		}
		if (((option->state & QStyle::State_MouseOver) || mPortsVisible) && portRenderer)
		{
			painter->save();
			painter->setOpacity(0.7);
			portRenderer->render(painter,mContents);
			painter->restore();
		}
	}
}

void NodeElement::addEdge(EdgeElement *edge)
{
	mEdgeList << edge;
}

void NodeElement::delEdge(EdgeElement *edge)
{
	mEdgeList.removeAt(mEdgeList.indexOf(edge));
}

void NodeElement::switchGrid()
{
	mSwitchGrid = !mSwitchGrid;
}
