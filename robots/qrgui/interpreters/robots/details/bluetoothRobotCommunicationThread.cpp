#include "bluetoothRobotCommunicationThread.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaType>
#include <time.h>

#include "../../thirdparty/qextserialport-1.2win-alpha/qextserialport.h"

using namespace qReal::interpreters::robots::details;

BluetoothRobotCommunicationThread::BluetoothRobotCommunicationThread()
	: mPort(NULL)
{
	qRegisterMetaType<details::lowLevelInputPort::InputPortEnum>("details::lowLevelInputPort::InputPortEnum");
}

BluetoothRobotCommunicationThread::~BluetoothRobotCommunicationThread()
{
	disconnect();
}

void BluetoothRobotCommunicationThread::send(QObject *addressee
		, QByteArray const &buffer, unsigned const responseSize)
{
	if (!mPort) {
		emit response(addressee, QByteArray());
		return;
	}

	send(buffer);
	if (buffer.size() >= 3 && buffer[2] == 0x00) {
		QByteArray const result = receive(responseSize);
		emit response(addressee, result);
	} else {
		emit response(addressee, QByteArray());
	}
}

void BluetoothRobotCommunicationThread::connect(QString const &portName)
{
	if (mPort != NULL) {
		disconnect();
		SleeperThread::msleep(1000);  // Give port some time to close
	}

	mPort = new QextSerialPort(portName);
	mPort->setBaudRate(BAUD9600);
	mPort->setFlowControl(FLOW_OFF);
	mPort->setParity(PAR_NONE);
	mPort->setDataBits(DATA_8);
	mPort->setStopBits(STOP_2);
	mPort->setTimeout(3000);

	mPort->open(QIODevice::ReadWrite | QIODevice::Unbuffered);

	qDebug() << "Port" << mPort->portName() << "is open:" << mPort->isOpen();

	QByteArray command(4, 0);
	command[0] = 0x02;  //command length
	command[1] = 0x00;
	command[2] = 0x01;
	command[3] = 0x88;

	send(command);
	QByteArray const response = receive(9);

	emit connected(response != QByteArray());
}

void BluetoothRobotCommunicationThread::reconnect(QString const &portName)
{
	connect(portName);
}

void BluetoothRobotCommunicationThread::disconnect()
{
	if (mPort) {
		mPort->close();
		delete mPort;
		mPort = NULL;
	}
	emit disconnected();
}

void BluetoothRobotCommunicationThread::sendI2C(QObject *addressee
		, QByteArray const &buffer, unsigned const responseSize
		, details::lowLevelInputPort::InputPortEnum const &port)
{
	if (!mPort) {
		emit response(addressee, QByteArray());
		return;
	}

	QByteArray command(buffer.length() + 7, 0);
	command[0] = buffer.length() + 5;
	command[1] = 0x00;
	command[2] = telegramType::directCommandNoResponse;
	command[3] = commandCode::LSWRITE;
	command[4] = port;
	command[5] = buffer.length();
	command[6] = responseSize;
	for (int i = 0; i < buffer.length(); ++i) {
		command[i + 7] = buffer[i];
	}

	send(command);

	if (responseSize < 0)
		return;

	if (!waitForBytes(responseSize, port)) {
		qDebug() << "No response, connection error";
		emit response(addressee, QByteArray());
		return;
	}

	if (responseSize > 0) {
		command.clear();
		command.resize(5);

		command[0] = 0x03;
		command[1] = 0x00;
		command[2] = telegramType::directCommandResponseRequired;
		command[3] = commandCode::LSREAD;
		command[4] = port;

		send(command);

		QByteArray const result = receive(22);
		QByteArray decodedResult = result.right(result.length() - 5);

		emit response(addressee, decodedResult);
	} else {
		// TODO: Correctly process empty required response
		QByteArray result(1, 0);
		emit response(addressee, result);
	}
}

bool BluetoothRobotCommunicationThread::waitForBytes(int bytes, lowLevelInputPort::InputPortEnum const &port) const
{
	time_t const startTime = clock();
	do {
		int const bytesReady = i2cBytesReady(port);
		SleeperThread::msleep(10);
		if (clock() - startTime > timeout)
			return false;
		if (bytesReady >= bytes)
			return true;
	} while (true);
}

int BluetoothRobotCommunicationThread::i2cBytesReady(lowLevelInputPort::InputPortEnum const &port) const
{
	QByteArray command(5, 0);
	command[0] = 0x03;
	command[1] = 0x00;

	command[2] = telegramType::directCommandResponseRequired;
	command[3] = commandCode::LSGETSTATUS;
	command[4] = port;

	send(command);
	QByteArray const result = receive(6);

	if (result.isEmpty() || result[4] != errorCode::success) {
		return 0;
	} else {
		return result[5];
	}
}

void BluetoothRobotCommunicationThread::send(QByteArray const &buffer) const
{
//	qDebug() << "Sending:";
//	for (int i = 0; i < buffer.size(); ++i) {
//		qDebug() << "Byte" << i << static_cast<unsigned char>(buffer[i]);
//	}
	mPort->write(buffer);
}

QByteArray BluetoothRobotCommunicationThread::receive(int size) const
{
	QByteArray const result = mPort->read(size);
//	qDebug() << "Received:";
//	for (int i = 0; i < result.size(); ++i) {
//		qDebug() << "Byte" << i << static_cast<unsigned char>(result[i]);
//	}
	return result;
}
