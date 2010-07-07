#include "ellipse.h"

Ellipse::Ellipse(qreal x1, qreal y1, qreal x2, qreal y2, Item* parent)
	:Item(parent)
{
	mPen.setColor(Qt::blue);
	mBrush.setStyle(Qt::NoBrush);
	mX1 = x1;
	mY1 = y1;
	mX2 = x2;
	mY2 = y2;
}

QRectF Ellipse::boundingRect() const
{
	return QRectF(qMin(mX1, mX2), qMin(mY1, mY2), abs(mX2 - mX1), abs(mY2 - mY1));
}

void Ellipse::drawItem(QPainter* painter)
{
	if(mX2 > mX1) {
		if (mY2 > mY1)
			painter->drawEllipse(mX1, mY1, mX2 - mX1, mY2 - mY1);
		else
			painter->drawEllipse(mX1, mY2, mX2 - mX1, mY1 - mY2);
	} else {
		if (mY2 > mY1)
			painter->drawEllipse(mX2, mY1, mX1 - mX2, mY2 - mY1);
		else
			painter->drawEllipse(mX2, mY2, mX1 - mX2, mY1 - mY2);
	}
}


