#include "blocksTable.h"

#include "blocks/block.h"
#include "blocksFactory.h"

using namespace qReal;
using namespace interpreters::robots::details;

BlocksTable::BlocksTable(models::GraphicalModelAssistApi const &graphicalModelApi
		, models::LogicalModelAssistApi const &logicalModelApi
		, RobotModel * const robotModel
	)
	: mGraphicalModelApi(graphicalModelApi)
	, mLogicalModelApi(logicalModelApi)
	, mBlocksFactory(new BlocksFactory(robotModel))
{
}

BlocksTable::~BlocksTable()
{
	foreach (blocks::Block *block, mBlocks.values())
		delete block;
	delete mBlocksFactory;
}

blocks::Block *BlocksTable::block(Id const &element)
{
	if (mBlocks.contains(element))
		return mBlocks[element];

	blocks::Block *newBlock = mBlocksFactory->block(element, mGraphicalModelApi, mLogicalModelApi, *this);
	return newBlock;
}

void BlocksTable::clear()
{
	foreach (blocks::Block *block, mBlocks.values())
		delete block;
	mBlocks.clear();
}

void BlocksTable::addBlock(Id const &element, blocks::Block *block)
{
	mBlocks.insert(element, block);
}
