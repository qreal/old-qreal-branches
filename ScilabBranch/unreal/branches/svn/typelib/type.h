#pragma once

#include <QString>
#include "value.h"
#include "typefactory.h"
#include "constraint.h"

namespace qRealType {
	class QRealTypeFactory;

	// Basic OCL types + enum
	enum QREAL_METATYPE {
		INTEGER,
		REAL,
		BOOLEAN,
		STRING,
		ENUM,
	};

	class QRealType {
		QREAL_METATYPE mType;
		QString mName;
		QRealValue *mDefaultValue;
		QConstraintList mConstraints;

		QRealType(QREAL_METATYPE type);
	public:
		~QRealType();

		QString toString() const;
		QRealValue* newValue();

		QRealType *clone() const;
		QRealType* newSubType(QString const &name, QConstraintList const &constr, QRealValue const *def = NULL);

		// Used by QRealValue class only
		QString toStringValue(QRealValue const *var);
		void fromStringValue(QRealValue *var, QString const &val);
		int toIntegerValue(QRealValue const *var);
		void fromIntegerValue(QRealValue *var, int val);
		bool toBooleanValue(QRealValue const *var);
		void fromBooleanValue(QRealValue *var, bool val);
		double toRealValue(QRealValue const *var);
		void fromRealValue(QRealValue *var, double val);

		friend class QRealTypeFactory;
	};
};
