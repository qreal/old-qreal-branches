#pragma once

#include "message.h"
#include "../../qrrepo/repoApi.h"
#include "classes/modelTreeItem.h"
#include "../editorManager/editorManager.h"

namespace qReal {
	class Repairer : QObject {
		Q_OBJECT
		public:
			Repairer(qrRepo::RepoApi &api, EditorManager const &editorManager);
			bool process(Id const target);

		public slots:
			void patchSave();
			void patchEditor();
			void repairElements();

		private:
			qrRepo::RepoApi &mApi;
			const EditorManager &mEditorManager;
			QMultiHash<QString, QLinkedList<Message> > mLogs;

			bool isCorrect(Id const target) const;
			void repairElements(Id const target);
			void readLog(QString const diagram);
			Id correctId(Id const target);
	};
}
