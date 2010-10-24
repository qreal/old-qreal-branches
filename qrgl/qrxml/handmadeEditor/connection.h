#pragma once

#include "../../qrgui/umllib/uml_edgeelement.h"

#include <QDebug>

namespace UML {

	class Connection: public EdgeElement {
	public:
		Connection() {
			mPenStyle = Qt::SolidLine;
		}

		virtual ~Connection() {}
	protected:
		virtual void drawStartArrow(QPainter * p) const {}
		virtual void drawEndArrow(QPainter * p) const {}
	};
}

