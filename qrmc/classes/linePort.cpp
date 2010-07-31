#include "linePort.h"
#include "../utils/outFile.h"

using namespace utils;

bool LinePort::init(QDomElement const &element, int width, int height)
{
	QDomElement portStartElement = element.firstChildElement("start");
	QDomElement portEndElement = element.firstChildElement("end");

	initCoordinate(mStartX, portStartElement.attribute("startx"), width);
	initCoordinate(mStartY, portStartElement.attribute("starty"), height);
	initCoordinate(mEndX, portEndElement.attribute("endx"), width);
	initCoordinate(mEndY, portEndElement.attribute("endy"), height);
	return true;
}

void LinePort::initCoordinate(ScalableCoordinate &field, QString coordinate, int maxValue)
{
	if (coordinate.endsWith("a"))
	{
		coordinate.remove(coordinate.length() - 1, 1);
		field = ScalableCoordinate(((qreal) coordinate.toInt()) / maxValue, true);
	}
	else if (coordinate.endsWith("%"))
	{
		coordinate.remove(coordinate.length() - 1, 1);
		field = ScalableCoordinate(((qreal) coordinate.toInt()) / 100, false);
	}
	else
	{
		field = ScalableCoordinate(((qreal) coordinate.toInt()) / maxValue, false);
	}
}

Port *LinePort::clone() const
{
	LinePort *result = new LinePort();
	result->mEndX = mEndX;
	result->mEndY = mEndY;
	result->mStartX = mStartX;
	result->mStartY = mStartY;
	return result;
}

