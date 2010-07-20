#include "pointPort.h"

const int step = 3;
PointPort::PointPort(qreal x, qreal y, Item *parent) : Item(parent)
{
	mNeedScalingRect = true;
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
	mBoundingRect = boundingRect().adjusted(scalingDrift, scalingDrift, -scalingDrift, -scalingDrift);
}

QRectF PointPort::boundingRect() const
{
	return mRect.adjusted(-scalingDrift, -scalingDrift, scalingDrift, scalingDrift);
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

	painter->setPen(QPen(Qt::red));
	painter->setBrush(QBrush(Qt::SolidPattern));
	drawScalingRects(painter);
}

void PointPort::drawFieldForResizeItem(QPainter* painter)
{
	Q_UNUSED(painter);
}

void PointPort::drawScalingRects(QPainter* painter)
{
	QBrush brush(Qt::SolidPattern);
	mBoundingRect = boundingRect().adjusted(scalingDrift, scalingDrift, -scalingDrift, -scalingDrift);
	qreal x1= mBoundingRect.left();
	qreal y1 = mBoundingRect.top();
	int scalingPoint = scalingRect;

	brush.setColor(mListScalePoint.at(4).second);
	painter->setBrush(brush);
	painter->drawRect(x1 - scalingPoint - step, y1 - step, scalingPoint, scalingPoint);

	brush.setColor(mListScalePoint.at(0).second);
	painter->setBrush(brush);
	painter->drawRect(x1 - step, y1 - scalingPoint - step, scalingPoint, scalingPoint);
}

void PointPort::changeDragState(qreal x, qreal y)
{
	Q_UNUSED(x);
	Q_UNUSED(y);
}

void PointPort::changeScalingPointState(qreal x, qreal y)
{
	mBoundingRect = boundingRect().adjusted(scalingDrift, scalingDrift, -scalingDrift, -scalingDrift);
	qreal x1= mBoundingRect.left();
	qreal x2 = mBoundingRect.right();
	qreal y1 = mBoundingRect.top();
	qreal y2 = mBoundingRect.bottom();
	int correction = step;
	calcForChangeScalingState(QPointF(x, y), QPointF(x1, y1), QPointF(x2, y2), correction);
}

void PointPort::resizeItem(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event);
}

QPair<QDomElement, Item::DomElementTypes> PointPort::generateItem(QDomDocument &document, QPointF const &topLeftPicture)
{
	mBoundingRect = boundingRect().adjusted(scalingDrift, scalingDrift, -scalingDrift, -scalingDrift);
	QDomElement pointPort = document.createElement("pointPort");
	qreal const x = scenePos().x() + mBoundingRect.x() + mRadius / 2 - topLeftPicture.x();
	qreal const y = scenePos().y() + mBoundingRect.y() + mRadius / 2 - topLeftPicture.y();
	pointPort.setAttribute("y", setSingleScaleForDoc(4, x, y));
	pointPort.setAttribute("x", setSingleScaleForDoc(0, x, y));

	return QPair<QDomElement, Item::DomElementTypes>(pointPort, mDomElementType);
}
