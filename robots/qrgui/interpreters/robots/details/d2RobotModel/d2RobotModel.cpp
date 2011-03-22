#include "d2RobotModel.h"

using namespace qReal::interpreters::robots;
using namespace details::d2Model;

D2RobotModel::D2RobotModel(QObject *parent)
	: QObject(parent)
{
	mDrawer = new RobotDrawer();
	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(nextFragment()));
	init();
}

D2RobotModel::~D2RobotModel()
{
	delete mDrawer;
}

void D2RobotModel::init()
{
	mMotorA = initMotor(5, 0, 0, 0);
	mMotorB = initMotor(5, 0, 0, 1);
	mMotorC = initMotor(5, 0, 0, 2);
	setBeep(0, 0);
	mAngle = 0;
	mPos = QPointF(0, 0);
	mRotatePoint  = QPointF(0, 0);
}

D2RobotModel::Motor* D2RobotModel::initMotor(int radius, int speed, long unsigned int degrees, int port)
{
	Motor *motor = new Motor();
	motor->radius = radius;
	motor->speed = speed;
	motor->degrees = degrees;
	mMotors[port] = motor;
	return motor;
}

void D2RobotModel::setBeep(unsigned freq, unsigned time)
{
	mBeep.freq = freq;
	mBeep.time = time;
}

void D2RobotModel::setNewMotor(int speed, unsigned long degrees, const int port)
{
	mMotors[port]->speed = speed;
	mMotors[port]->degrees = degrees;
}

void D2RobotModel::startInit()
{
	init();
	mDrawer->init();
	mTimer->start(timeInterval);
}

void D2RobotModel::stopRobot()
{
	mTimer->stop();
	mDrawer->close();
}

void D2RobotModel::countBeep()
{
	if (mBeep.time > 0) {
		mDrawer->drawBeep(QColor(Qt::red));
		mBeep.time -= timeInterval;
	} else
		mDrawer->drawBeep(QColor(Qt::green));
}

void D2RobotModel::countNewCoord()
{
	qreal vSpeed = mMotorA->speed * 2 * M_PI * mMotorA->radius * 1.0 / 120000;
	qreal uSpeed = mMotorB->speed * 2 * M_PI * mMotorB->radius * 1.0 / 120000;
	qreal gamma = 0;
	qreal deltaY = 0;
	qreal deltaX = 0;
	qreal averangeSpeed = (vSpeed + uSpeed) / 2;
	if (vSpeed != uSpeed) {
		qreal vRadius = vSpeed * robotHeight / (vSpeed - uSpeed);
		qreal averangeRadius = vRadius - robotHeight / 2;
		qreal angularSpeed = 0;
		if (vSpeed == -uSpeed)
			angularSpeed = vSpeed / vRadius;
		else
			angularSpeed = averangeSpeed / averangeRadius;
		gamma = timeInterval * angularSpeed;//����� ���� � ��������
		qreal const gammaDegrees = gamma * 180 / M_PI;

		QTransform map;
		map.rotate(mAngle);
		map.translate(robotWidth, vRadius);
		map.rotate(gammaDegrees);
		map.translate(-robotWidth, -vRadius);

		QPointF newStart = map.map(QPointF(0, 0));
		deltaX = newStart.x();
		deltaY = newStart.y();

		mAngle += gammaDegrees;//����� ���� � ��������
	} else {
		deltaY = averangeSpeed * timeInterval * sin(mAngle * M_PI / 180);
		deltaX = averangeSpeed * timeInterval * cos(mAngle * M_PI / 180);
	}
	mPos.setX(mPos.x() + deltaX);
	mPos.setY(mPos.y() + deltaY);
	if(mAngle > 360)
		mAngle -= 360;
}

void D2RobotModel::nextFragment()
{
	mDrawer->draw(mPos, mAngle, mRotatePoint);
	countNewCoord();
	countBeep();
}
