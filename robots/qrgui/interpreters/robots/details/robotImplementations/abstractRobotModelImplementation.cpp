#include "abstractRobotModelImplementation.h"

#include <QtCore/QDebug>

#include "../../../../kernel/exception/exception.h"

#include "nullRobotModelImplementation.h"
#include "realRobotModelImplementation.h"
#include "unrealRobotModelImplementation.h"
#include <QtCore/QDebug>
using namespace qReal::interpreters::robots;
using namespace details::robotImplementations;

NullRobotModelImplementation *AbstractRobotModelImplementation::mNullRobotModel = NULL;
RealRobotModelImplementation *AbstractRobotModelImplementation::mRealRobotModel = NULL;
UnrealRobotModelImplementation *AbstractRobotModelImplementation::mUnrealRobotModel = NULL;

AbstractRobotModelImplementation::AbstractRobotModelImplementation()
	: mSensorsToConfigure(0)
{
	mSensors.resize(4);
}

AbstractRobotModelImplementation::~AbstractRobotModelImplementation()
{
	foreach (sensorImplementations::AbstractSensorImplementation *sensor, mSensors) {
		delete sensor;
	}
}

NullRobotModelImplementation *AbstractRobotModelImplementation::nullRobotModel()
{
	if (mNullRobotModel == NULL)
		mNullRobotModel = new NullRobotModelImplementation();
	return mNullRobotModel;
}

RealRobotModelImplementation *AbstractRobotModelImplementation::realRobotModel(RobotCommunicationInterface * const robotCommunicationInterface)
{
	if (mRealRobotModel == NULL)
		mRealRobotModel = new RealRobotModelImplementation(robotCommunicationInterface);
	return mRealRobotModel;
}

UnrealRobotModelImplementation *AbstractRobotModelImplementation::unrealRobotModel(d2Model::D2RobotModel *d2RobotModel)
{
	if (mUnrealRobotModel == NULL)
		mUnrealRobotModel = new UnrealRobotModelImplementation(d2RobotModel);
	return mUnrealRobotModel;
}

AbstractRobotModelImplementation *AbstractRobotModelImplementation::robotModel(robotModelType::robotModelTypeEnum type, RobotCommunicationInterface * const robotCommunicationInterface, d2Model::D2RobotModel *d2RobotModel)
{
	if (type == robotModelType::null)
		return nullRobotModel();
	else if (type == robotModelType::unreal)
		return unrealRobotModel(d2RobotModel);
	else if (type == robotModelType::real)
		return realRobotModel(robotCommunicationInterface);
	else
		throw Exception("AbstractRobotModelImplementation::robotModel tried to create unknown robot model");
}

QVector<sensorImplementations::AbstractSensorImplementation *> AbstractRobotModelImplementation::sensors()
{
	return mSensors;
}

void AbstractRobotModelImplementation::init()
{
	qDebug() << "Initializing robot model...";
	qDebug() << "Connecting to robot...";
}

void AbstractRobotModelImplementation::clear()
{
}

void AbstractRobotModelImplementation::configureSensor(sensorType::SensorTypeEnum const &sensorType
		, inputPort::InputPortEnum const &port)
{
	delete mSensors[port];
	mSensors[port] = NULL;
	switch (sensorType) {
	case sensorType::unused:
		break;
	case sensorType::touchBoolean:
		addTouchSensor(port);
		break;
	case sensorType::touchRaw:
		break;
	case sensorType::sonar:
		addSonarSensor(port);
		break;
	case sensorType::colorFull:
		addColorSensor(port, lowLevelSensorType::COLORFULL);
		break;
	case sensorType::colorRed:
		addColorSensor(port, lowLevelSensorType::COLORRED);
		break;
	case sensorType::colorGreen:
		addColorSensor(port, lowLevelSensorType::COLORGREEN);
		break;
	case sensorType::colorBlue:
		addColorSensor(port, lowLevelSensorType::COLORBLUE);
		break;
	case sensorType::colorNone:
		addColorSensor(port, lowLevelSensorType::COLORNONE);
		break;
	default:
		// TODO: Throw an exception
		break;
	}
}
