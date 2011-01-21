#include "enginesForwardBlock.h"

using namespace qReal;
using namespace interpreters::robots::details::blocks;

EnginesForwardBlock::EnginesForwardBlock(robotParts::Motor &motor1, robotParts::Motor &motor2)
	: mMotor1(motor1)
	, mMotor2(motor2)
{
}

void EnginesForwardBlock::run()
{
	mMotor1.on(50);
	mMotor2.on(50);
	emit done(mNextBlock);
}
