#pragma once

#include "type.h"

namespace utils {
	class OutFile;
}

class NonGraphicType : public Type
{
public:
	virtual bool resolve();

	virtual void print();

	virtual bool isGraphicalType() const;

	virtual QString generateProperties(QString const &namesTemplate) const;
	virtual QString generatePropertyDefaults(QString const &namesTemplate) const;
	virtual QString generateContainers(QString const &lineTemplate) const;
	virtual QString generateConnections(QString const &lineTemplate) const;
	virtual QString generateUsages(QString const &lineTemplate) const;

protected:
	NonGraphicType();
};
