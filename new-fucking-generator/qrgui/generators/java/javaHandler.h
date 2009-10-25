#pragma once

#include <QtCore/QString>

#include "../../kernel/ids.h"

namespace qReal {

	namespace client {
		class RepoApi;
	}

	namespace generators {

		class JavaHandler
		{
		public:
			explicit JavaHandler(client::RepoApi const &api);

			QString generateToJava(QString const &pathToFile);
		private:
			QString serializeObject(Id const &id, Id const &parentId);
			QString serializeChildren(Id const &id);
			QString getVisibility(Id const &id);
			QString getType(Id const &id);
			QString getDefaultValue(Id const &id);
			QString getOperationFactors(Id const &id);
			QString hasModifier(Id const &id, QString const &modifier);
			QString getParents(Id const &id);

			QString serializeMultiplicity(Id const &id, QString const &multiplicity) const;
			bool isTypeSuitable(QString const &type) const;
			bool isVisibilitySuitable(QString const &type) const;

			void addError(QString const &errorText);

			client::RepoApi const &mApi;
			QString mErrorText;
		};

	}
}
