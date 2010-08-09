#pragma once
#include <GeometricForms.h>
#include <QList>
#include <QString>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>

// ������ xml - ���������� ����� �������� � �� ����������� ��������
class Serializer
{

public:
	Serializer(QString const & pathToFile);
	void serialize(const Objects &objects);
	EntityVector deserialize();

private:
	QDomElement getFirstDomElement();
	Entity parseNode(QDomElement const & domElement);
	QList<QPoint> getEllipsePath(QPoint const &point1, QPoint const &point2);
	QString mPathToFile;
	QDomElement mDomElement;
	QList<QPoint> getPoints(QDomElement const & domElement);
	QList<QPoint> getRectanglePath(QDomElement const & domElement);
};
