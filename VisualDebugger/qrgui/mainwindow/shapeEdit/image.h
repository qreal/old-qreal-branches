#pragma once
#include "item.h"

class Image : public Item
{
public:
	Image(QString fileName, qreal x, qreal y, Item* parent = 0);
	Image(Image const &other);
	virtual Item* clone();
	virtual QRectF boundingRect() const;
	virtual void drawItem(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

	virtual QPair<QDomElement, Item::DomElementTypes> generateItem(QDomDocument &document, QPoint const &topLeftPicture);
private:
	QGraphicsPixmapItem* mPixmapItem;
	QString mFileName;
};
