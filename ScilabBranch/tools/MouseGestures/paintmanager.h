#pragma once
#include <QPainter>
#include <QList>
#include <QVector>
#include <QPoint>
#include <QString>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QObject>

class PaintManager : public QObject
{
    Q_OBJECT
public:
    PaintManager(QGraphicsView *gestureView);
    ~PaintManager();
    static void drawPath(QPainter *painter, QList<QPoint> const & path);
    void drawGesture(QString const & polygonString);

private:
    QGraphicsScene *mGestureScene;
    QTimer *mTimer;
    QVector<QPoint> mPath;
    static const int pointsAtSegment = 5;
    int mCurrentPointNumber;
    static int coord(int previous, int next, int part);

private slots:
    void draw();
};
