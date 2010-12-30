#pragma once

#include "details/graphicalModel.h"
#include "details/logicalModel.h"
#include "graphicalModelAssistApi.h"
#include "logicalModelAssistApi.h"

namespace qReal {

namespace models {

class Models
{
public:
	explicit Models(QString const &workingCopy, EditorManager const &editorManager);
	~Models();

	QAbstractItemModel* graphicalModel() const;
	QAbstractItemModel* logicalModel() const;
	void saveTo(QString const &workingDirectory);
	GraphicalModelAssistApi &graphicalModelAssistApi() const;
	LogicalModelAssistApi &logicalModelAssistApi() const;
	qrRepo::RepoControlInterface const *repoControlApi() const;
	void resetChangedDiagrams();
	void resetChangedDiagrams(const IdList &list);

	qrRepo::LogicalRepoApi const &logicalRepoApi() const;
	qrRepo::LogicalRepoApi &mutableLogicalRepoApi() const;
	void reinit();

private:
	models::details::GraphicalModel *mGraphicalModel;
	models::details::LogicalModel *mLogicalModel;
	qrRepo::RepoControlInterface *mRepoApi;
};

}

}
