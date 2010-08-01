#include "label.h"
#include "../utils/outFile.h"
#include "metaCompiler.h"

#include <QDebug>

using namespace utils;

bool Label::init(QDomElement const &element, int index, bool nodeLabel)
{
	mX = element.attribute("x", "0");
	mY = element.attribute("y", "0");
	mCenter = element.attribute("center", "false");
	mText = element.attribute("text");
	mTextBinded = element.attribute("textBinded");
	mReadOnly = element.attribute("readOnly", "false");
	mIndex = index;
	mBackground = element.attribute("background", nodeLabel ? "transparent" : "white");
	if ((mText.isEmpty() && mTextBinded.isEmpty()) || (mReadOnly != "true" && mReadOnly != "false")) {
		qDebug() << "ERROR: can't parse label";
		return false;
	}
	return true;
}

QString Label::generateInit(MetaCompiler *compiler) const
{
	QString result = compiler->getTemplateUtils(nodeInitTag);
	QString name = mText.isEmpty() ? mTextBinded : mText;

	result.replace(labelXTag, mX)
			.replace(labelYTag, mY)
			.replace(labelReadonlyTag, mReadOnly)
			.replace(labelIndexTag, QString::number(mIndex))
			.replace(labelNameTag, name);

	return result;
}
