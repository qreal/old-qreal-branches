#include "line.h"
#include <QPainter>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

Line::Line(qreal x1, qreal y1, qreal x2, qreal y2, Item* parent)
	:Item(parent)
{
	mPen.setColor(Qt::green);
	mX1 = x1;
	mY1 = y1;
	mX2 = x2;
	mY2 = y2;
}

QRectF Line::boundingRect() const
{
	return QRectF(qMin(mX1, mX2), qMin(mY1, mY2), abs(mX2 - mX1), abs(mY2 - mY1));
}

void Line::drawItem(QPainter* painter)
{
	painter->drawLine(mX1, mY1, mX2, mY2);
}

void Line::drawExtractionForItem(QPainter* painter)
{
	painter->drawPoint(mX1, mY1);
	painter->drawPoint(mX2, mY2);
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
	ps.setWidth(5);

	path.moveTo(mX1, mY1);
	path.lineTo(mX2, mY2);
	path = ps.createStroke(path);

	return path;
}
