#pragma once

#include <QWidget>
#include <QList>
#include <QRectF>
#include <QPointF>
#include <QPainter>
#include "elementTitle.h"
#include "elementRepoInterface.h"
#include "sdfrenderer.h"

namespace UML{

/** @brief Описание линейного порта, реагирующего на абсолютные координаты */
	struct StatLine
	{
		QLineF line;
		bool prop_x1;
		bool prop_y1;
		bool prop_x2;
		bool prop_y2;
	
		StatLine() : line(QLineF(0, 0, 0, 0)), prop_x1(false), prop_y1(false),
			prop_x2(false), prop_y2(false) {}
	
		operator QLineF () const
		{
			return line;
		}
	
		void operator = (QLineF const &l)
		{
			line = l;
			prop_x1 = false;
			prop_x2 = false;
			prop_y1 = false;
			prop_y2 = false;
		}
	};
	
	/** @class ElementImpl
	 *	@brief Базовый класс для генерящейся в плагины специфики
	 *	TODO: поделить на два
	 * */	
	class ElementImpl {
		public:
			virtual void init(QRectF &contents, QList<QPointF> &pointPorts, 
							QList<StatLine> &linePorts, QList<ElementTitle*> &titles, SdfRenderer *portRenderer) = 0;
			virtual void init(QList<ElementTitle*> &titles) = 0;
			virtual void paint(QPainter *painter, QRectF &contents) = 0;
			virtual void updateData(ElementRepoInterface *repo) const = 0;
			virtual bool isNode() = 0;
			virtual bool hasPorts() = 0;
			virtual Qt::PenStyle getPenStyle() = 0;
			virtual void drawStartArrow(QPainter *painter) const = 0;
			virtual void drawEndArrow(QPainter *painter) const = 0;
	};
}
