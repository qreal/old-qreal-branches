#include "nullRobotModelImplementation.h"
using namespace qReal::interpreters::robots;
using namespace details::robotImplementations;

NullRobotModelImplementation::NullRobotModelImplementation()
	: AbstractRobotModelImplementation()
	, mBrick()
	, mMotorA(0)
	, mMotorB(1)
	, mMotorC(2)
{
	mActiveWaitingTimer.setInterval(100);
	connect(&mActiveWaitingTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

brickImplementations::NullBrickImplementation &NullRobotModelImplementation::brick()
{
	return mBrick;
}

sensorImplementations::NullTouchSensorImplementation *NullRobotModelImplementation::touchSensor(inputPort::InputPortEnum const &port) const
{
	return dynamic_cast<sensorImplementations::NullTouchSensorImplementation *>(mSensors[port]);
}

sensorImplementations::NullSonarSensorImplementation *NullRobotModelImplementation::sonarSensor(inputPort::InputPortEnum const &port) const
{
	return dynamic_cast<sensorImplementations::NullSonarSensorImplementation *>(mSensors[port]);
}

void NullRobotModelImplementation::addTouchSensor(inputPort::InputPortEnum const &port)
{
//	lowLevelInputPort::InputPortEnum const lowLevelPort = static_cast<lowLevelInputPort::InputPortEnum>(port);
	mSensors[port] = new sensorImplementations::NullTouchSensorImplementation(port);
}

void NullRobotModelImplementation::addSonarSensor(inputPort::InputPortEnum const &port)
{
//	lowLevelInputPort::InputPortEnum const lowLevelPort = static_cast<lowLevelInputPort::InputPortEnum>(port);
	mSensors[port] = new sensorImplementations::NullSonarSensorImplementation(port);
}

void NullRobotModelImplementation::init()
{
	AbstractRobotModelImplementation::init();
	mActiveWaitingTimer.start();
}

void NullRobotModelImplementation::timerTimeout()
{
	emit connected(true);
}

void NullRobotModelImplementation::stopRobot()
{
	mMotorA.off();
	mMotorB.off();
	mMotorC.off();
}

motorImplementations::NullMotorImplementation &NullRobotModelImplementation::motorA()
{
	return mMotorA;
}

motorImplementations::NullMotorImplementation &NullRobotModelImplementation::motorB()
{
	return mMotorB;
}

motorImplementations::NullMotorImplementation &NullRobotModelImplementation::motorC()
{
	return mMotorC;
}

