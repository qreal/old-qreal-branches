#pragma once
#include <QList>
#include <QPoint>
#include <QString>

//todo:: � ���� ����� ���� �������� ��� �� ������, �� �������� ��� ���� ������� � ����� (������ �� 2 ������)

class Adopter
{
public:
    static QString pathToString(QList<QPoint> const &path);
    static QList<QPoint> stringToPath(QString const &str);

private:
    static QPoint parsePoint(QString const &str);
    static QString pointToString(QPoint const &p);
};
