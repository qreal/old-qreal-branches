#pragma once

#include <QtGui/QGraphicsTextItem>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtGui/QPainter>
#include <QtGui/QFont>

#include "item.h"

class Text : public Item
{
public:
	Text(qreal x, qreal y, QString const &text = "text", bool isDynamic = false);
	bool isDynamicText();
	void setIsDynamicText(bool isDynamic);
	virtual QRectF boundingRect() const;
	virtual void drawItem(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
	virtual void drawExtractionForItem(QPainter* painter);
	virtual void drawFieldForResizeItem(QPainter* painter);
	virtual void drawScalingRects(QPainter* painter);
	virtual void changeScalingPointState(qreal x, qreal y);
	QGraphicsTextItem const& getText();
	void setZValueAll(int const &index);

	virtual QPair<QDomElement, Item::DomElementTypes> generateItem(QDomDocument &document, QPointF const &topLeftPicture);

private:
	QGraphicsTextItem mText;
	bool mIsDynamicText;
	QRectF mRect;
	QFont mFont;
	void drawForDynamicText(QPainter* painter);
};
