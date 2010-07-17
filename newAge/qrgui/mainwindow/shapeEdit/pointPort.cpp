#include "pointPort.h"

PointPort::PointPort(qreal x, qreal y, Item *parent) : Item(parent)
{
	mRadius = 4;
	mRect = QRectF(x - mRadius / 2, y - mRadius / 2, mRadius, mRadius);
	mX1 = x - mRadius * 0.8;
	mY1 = y - mRadius * 0.8;
	mX2 = x + mRadius * 0.8;
	mY2 = y + mRadius * 0.8;
	mPen = QPen(Qt::blue);
	mBrush = QBrush(Qt::SolidPattern);
	mBrush.setColor(Qt::blue);
	mDomElementType = portType;
}

QRectF PointPort::boundingRect() const
{
	return mRect;
}

void PointPort::drawItem(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->setPen(mPen);
	painter->setBrush(mBrush);
	painter->drawEllipse(mRect);
}

void PointPort::drawExtractionForItem(QPainter* painter)
{
	QPen pen(Qt::red);
	pen.setWidth(mRadius / 2.3);
	painter->setPen(pen);
	Item::drawExtractionForItem(painter);
	drawFieldForResizeItem(painter);
}

void PointPort::drawFieldForResizeItem(QPainter* painter)
{
	Q_UNUSED(painter);
}

void PointPort::changeDragState(qreal x, qreal y)
{
	Q_UNUSED(x);
	Q_UNUSED(y);
}

void PointPort::resizeItem(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event);
}

QPair<QDomElement, Item::DomElementTypes> PointPort::generateItem(QDomDocument &document, QPointF const &topLeftPicture)
{
	QDomElement pointPort = document.createElement("pointPort");
	qreal const x = scenePos().x() + boundingRect().x() + mRadius / 2 - topLeftPicture.x();
	qreal const y = scenePos().y() + boundingRect().y() + mRadius / 2 - topLeftPicture.y();
	pointPort.setAttribute("y", y);
	pointPort.setAttribute("x", x);

	return QPair<QDomElement, Item::DomElementTypes>(pointPort, mDomElementType);
}
