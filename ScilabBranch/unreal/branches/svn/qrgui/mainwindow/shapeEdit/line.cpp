#include "line.h"
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionGraphicsItem>

Line::Line(qreal x1, qreal y1, qreal x2, qreal y2, Item* parent)
	:Item(parent)
{
	mPen.setColor(Qt::green);
	mDomElementType = pictureType;
	mX1 = x1;
	mY1 = y1;
	mX2 = x2;
	mY2 = y2;
}

QRectF Line::boundingRect() const
{
	return QRectF(qMin(mX1, mX2) - mPen.width(), qMin(mY1, mY2) - mPen.width(), abs(mX2 - mX1) + mPen.width(), abs(mY2 - mY1) + mPen.width()).adjusted(-drift, -drift, drift, drift);
}

void Line::drawItem(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->drawLine(mX1, mY1, mX2, mY2);
}

void Line::drawExtractionForItem(QPainter* painter)
{
	painter->drawPoint(mX1, mY1);
	painter->drawPoint(mX2, mY2);

	setPenBrushDriftRect(painter);
	painter->drawPath(shape());
	drawFieldForResizeItem(painter);
}

void Line::drawFieldForResizeItem(QPainter* painter)
{
	painter->drawEllipse(QPointF(mX1, mY1), resizeDrift, resizeDrift);
	painter->drawEllipse(QPointF(mX2, mY2), resizeDrift, resizeDrift);
}

QLineF Line::line() const
{
	return QLineF(mX1, mY1, mX2, mY2);
}

QPainterPath Line::shape() const
{
	QPainterPath path;
	path.setFillRule(Qt::WindingFill);

	QPainterPathStroker ps;
	ps.setWidth(drift);

	path.moveTo(mX1, mY1);
	path.lineTo(mX2, mY2);
	path = ps.createStroke(path);

	return path;
}

void Line::resizeItem(QGraphicsSceneMouseEvent *event)
{
	if (mDragState == TopLeft || mDragState == BottomRight)
		Item::resizeItem(event);
}

QPair<QDomElement, Item::DomElementTypes> Line::generateItem(QDomDocument &document, QPointF const &topLeftPicture)
{
	qreal const x1 = scenePos().x() + line().x1() - topLeftPicture.x();
	qreal const y1 = scenePos().y() + line().y1() - topLeftPicture.y();
	qreal const x2 = scenePos().x() + line().x2() - topLeftPicture.x();
	qreal const y2 = scenePos().y() + line().y2() - topLeftPicture.y();

	QDomElement line = setPenBrushToDoc(document, "line");
	setXandY(line, QRectF(x1, y1, x2 - x1, y2 - y1));

	return QPair<QDomElement, Item::DomElementTypes>(line, mDomElementType);
}
