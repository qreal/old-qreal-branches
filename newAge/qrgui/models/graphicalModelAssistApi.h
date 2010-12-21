#pragma once
#include "../kernel/ids.h"
#include "details/graphicalModel.h"
#include "details/modelsAssistApi.h"

namespace qReal {

	class EditorManager;

	namespace models {

		namespace details {
			class GraphicalModel;
		}

		class GraphicalModelAssistApi : public details::ModelsAssistApi {
		public:
			GraphicalModelAssistApi(details::GraphicalModel &graphicalModel, EditorManager const &editorManager);
			qrRepo::GraphicalRepoApi const &graphicalRepoApi() const;
			virtual Id createElement(Id const &parent, Id const &type);
			virtual Id createElement(Id const &parent, Id const &id, bool isFromLogicalModel, QString const &name, QPointF const &position);
			virtual IdList children(Id const &element) const;
			virtual void changeParent(Id const &element, Id const &parent, QPointF const &position);

			void setConfiguration(Id const &elem, QPolygon const &newValue);
			QPolygon configuration(Id const &elem) const;

			void setPosition(Id const &elem, QPointF const &newValue);
			QPointF position(Id const &elem) const;

			void setToPort(Id const &elem, qreal const &newValue);
			qreal toPort(Id const &elem) const;

			void setFromPort(Id const &elem, qreal const &newValue);
			qreal fromPort(Id const &elem) const;

			void setName(Id const &elem, QString const &newValue);
			QString name(Id const &elem) const;

			void setToolTip(Id const &elem, QString const &newValue);
			QString toolTip(Id const &elem) const;

			Id logicalId(Id const &elem) const;
			IdList graphicalIdsByLogicalId(Id const &logicalId) const;

		private:
			GraphicalModelAssistApi(GraphicalModelAssistApi const &);
			GraphicalModelAssistApi& operator =(GraphicalModelAssistApi const &);

			details::GraphicalModel &mGraphicalModel;
		};
	}
}
