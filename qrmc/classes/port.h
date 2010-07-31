#pragma once

#include <QDomElement>

namespace utils {
	class OutFile;
}

class Port
{
public:
	virtual bool init(QDomElement const &element, int width, int height) = 0;
	virtual Port *clone() const = 0;
};
