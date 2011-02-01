#include "waitForSonarDistanceBlock.h"

#include "../../sensorConstants.h"

using namespace qReal;
using namespace interpreters::robots;
using namespace details::blocks;

WaitForSonarDistanceBlock::WaitForSonarDistanceBlock(RobotModel const * const robotModel)
	: mSonarSensor(NULL)
	, mRobotModel(robotModel)
{
	// There is about 30 ms latency within robot bluetooth chip, so it is useless to
	// read sensor too frequently.
	mActiveWaitingTimer.setInterval(100);

	connect(&mActiveWaitingTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

void WaitForSonarDistanceBlock::run()
{
	inputPort::InputPortEnum const port = static_cast<inputPort::InputPortEnum>(intProperty("Port") - 1);
	mSonarSensor = mRobotModel->sonarSensor(port);

	if (!mSonarSensor) {
		mActiveWaitingTimer.stop();
		error(tr("Sonar sensor is not configured on this port"));
		return;
	}

	connect(mSonarSensor, SIGNAL(response(int)), this, SLOT(response(int)));
	connect(mSonarSensor, SIGNAL(failure()), this, SLOT(failure()));

	mSonarSensor->read();
	mActiveWaitingTimer.start();
}

void WaitForSonarDistanceBlock::response(int reading)
{
	int const targetDistance = intProperty("Distance");
	if (reading < targetDistance) {
		mActiveWaitingTimer.stop();
		emit done(mNextBlock);
	}
}

void WaitForSonarDistanceBlock::failure()
{
	mActiveWaitingTimer.stop();
	emit failure();
}

void WaitForSonarDistanceBlock::timerTimeout()
{
	mSonarSensor->read();
}

QList<Block::SensorPortPair> WaitForSonarDistanceBlock::usedSensors() const
{
	inputPort::InputPortEnum const port = static_cast<inputPort::InputPortEnum>(intProperty("Port") - 1);
	return QList<SensorPortPair>() << qMakePair(sensorType::sonar, static_cast<int>(port));
}
