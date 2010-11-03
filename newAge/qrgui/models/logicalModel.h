#pragma once

#include "../../qrrepo/repoApi.h"
#include "../editorManager/editorManager.h"
#include "details/logicalModelItem.h"
#include "details/abstractModel.h"
#include "graphicalModelView.h"

namespace qReal {

	namespace models {

		class GraphicalModel;

		class LogicalModel : public details::AbstractModel
		{
			Q_OBJECT;
		public:
			LogicalModel(qrRepo::RepoApi &repoApi, EditorManager const &editorManager);

			void connectToGraphicalModel(GraphicalModel * const graphicalModel);
			void updateElements(Id const &logicalId, QString const &name);
			QModelIndex indexById(Id const &id) const;

		private:
			GraphicalModelView mGraphicalModelView;

			virtual details::AbstractModelItem *createModelItem(Id const &id, details::AbstractModelItem *parentItem) const;
		};

	}

}
