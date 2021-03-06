#pragma once

#include "../kernel/ids.h"

namespace qReal {
	class EditorManager;
	namespace model {
		class Model;

		/** @class ModelAssistApi
		 *	@brief class used for syncronization of logical model and repo/model.
		 *	if GUI wants to change model in a way different from setData(),
		 *	it should call this class' methods. ModelAssistApi modifies the repo
		 *	and notifies the model. it's in fact yet another interface to the model
		 *	and even may become a proxy model for repo one day.
		 *	ModelAssistApi used to somehow control GUI's actions towards the repo,
		 *	model's mutableApi() should be used only in parsers and here
		 * */

		class ModelAssistApi {
		public:
			ModelAssistApi(Model &model, EditorManager const &editorManager);
			virtual EditorManager const &editorManager() const;

			virtual void connect(qReal::Id const &source, qReal::Id const &destination);
			virtual void disconnect(qReal::Id const &source, qReal::Id const &destination);
			virtual void addUsage(qReal::Id const &source, qReal::Id const &destination);
			virtual void deleteUsage(qReal::Id const &source, qReal::Id const &destination);

			virtual Id createElement(qReal::Id const &parent, qReal::Id const &type);

			virtual void setProperty(qReal::Id const &elem, int const role, QVariant const &newValue);
			virtual QVariant getProperty(qReal::Id const &elem, int const role) const;

			void createConnected(qReal::Id const &sourceElement, qReal::Id const &elementType);
			void createUsed(qReal::Id const &sourceElement, qReal::Id const &elementType);
			qReal::IdList diagramsAbleToBeConnectedTo(qReal::Id const &element) const;
			qReal::IdList diagramsAbleToBeUsedIn(qReal::Id const &element) const;

		private:
			ModelAssistApi(ModelAssistApi const &);  // Copying is forbidden
			ModelAssistApi& operator =(ModelAssistApi const &); // Assignment is forbidden also

			static IdList diagramsFromList(IdList const &list);
			Id createConnectedElement(Id const &source, Id const &elementType);

			Model &mModel;
			EditorManager const &mEditorManager;
		};

	}
}
