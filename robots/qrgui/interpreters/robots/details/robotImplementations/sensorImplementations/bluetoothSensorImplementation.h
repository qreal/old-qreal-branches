#pragma once
#include "abstractSensorImplementation.h"
#include "../../../robotCommunicationInterface.h"
#include "../../robotCommandConstants.h"

namespace qReal {
namespace interpreters {
namespace robots {
namespace details {
namespace robotImplementations {
namespace sensorImplementations {

class BluetoothSensorImplementation : public AbstractSensorImplementation
{
	Q_OBJECT
public:
	BluetoothSensorImplementation(RobotCommunicationInterface *robotCommunicationInterface
		, lowLevelSensorType::SensorTypeEnum const &lowLevelSensorType
		, sensorMode::SensorModeEnum const &sensorMode
		, inputPort::InputPortEnum const &port
		);
	virtual ~BluetoothSensorImplementation();
	void configure();
	virtual void read() = 0;

signals:
	void configured();
protected slots:
	void readingDone(QObject *addressee, QByteArray const &reading);

protected:
	RobotCommunicationInterface *mRobotCommunicationInterface;
	lowLevelSensorType::SensorTypeEnum mSensorType;
	sensorMode::SensorModeEnum mSensorMode;
	bool mIsConfigured;
	bool mResetDone;

	virtual void processResponse(QByteArray const &reading);
	virtual void sensorSpecificProcessResponse(QByteArray const &reading) = 0;
};

}
}
}
}
}
}

