#pragma once

#include "nonGraphicType.h"

#include <QDomElement>

enum BaseType
{
	IntType,
	FloatType
};

class NumericType : public NonGraphicType
{
public:
	virtual bool init(QDomElement const &element, QString const &context);
	virtual Type* clone() const;

private:
	BaseType mBaseType;
};
