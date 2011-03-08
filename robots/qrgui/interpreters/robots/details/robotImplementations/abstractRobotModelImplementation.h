#pragma once
#include <QtCore/QObject>
#include <QtCore/QVector>
#include "brickImplementations/abstractBrickImplementation.h"
#include "motorImplementations/abstractMotorImplementation.h"
#include "sensorImplementations/abstractSensorImplementation.h"
#include "../../sensorConstants.h"

namespace qReal {
namespace interpreters {
namespace robots {
namespace details {
namespace robotImplementations {

class AbstractRobotModelImplementation : public QObject
{
	Q_OBJECT
public:
	AbstractRobotModelImplementation();
	virtual ~AbstractRobotModelImplementation();

	virtual void init();
	virtual void clear();
	virtual void stopRobot() = 0;

	virtual brickImplementations::AbstractBrickImplementation &brick() = 0;
	virtual sensorImplementations::AbstractSensorImplementation *touchSensor(inputPort::InputPortEnum const &port) const = 0;
	virtual sensorImplementations::AbstractSensorImplementation *sonarSensor(inputPort::InputPortEnum const &port) const = 0;

	virtual motorImplementations::AbstractMotorImplementation &motorA() = 0;
	virtual motorImplementations::AbstractMotorImplementation &motorB() = 0;
	virtual motorImplementations::AbstractMotorImplementation &motorC() = 0;

	virtual void configureSensor(sensorType::SensorTypeEnum const &sensorType
			, inputPort::InputPortEnum const &port);
	virtual QVector<sensorImplementations::AbstractSensorImplementation *> sensors();
signals:
	void connected(bool success);
protected:
	int mSensorsToConfigure;
	QVector<sensorImplementations::AbstractSensorImplementation *> mSensors;
	virtual void addTouchSensor(inputPort::InputPortEnum const &port) = 0;
	virtual void addSonarSensor(inputPort::InputPortEnum const &port) = 0;
};

}
}
}
}
}

