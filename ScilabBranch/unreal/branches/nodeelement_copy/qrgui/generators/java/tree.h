#pragma once

#include <QtCore/QString>

#include "../../kernel/ids.h"

namespace qrRepo {
    class RepoApi;
}

namespace qReal {

    namespace generators {

        class Tree
        {
        public:
            Tree(Id const &element, QList<Tree> const &children);

            bool operator== ( Tree const & other  ) const;

            Id element() const;
            QList<Tree> children() const;
        private:
            Id mElement;
            QList<Tree> mChildren;

            void moveChildToEnd(Tree const &child);
//            QString serializeObject(Id const &id);
//            QString serializeChildren(Id const &id);
//            QString getVisibility(Id const &id);
//            QString getMultiplicity(Id const &id);
//            QString getType(Id const &id);
//            QString getDefaultValue(Id const &id);
//            QString getOperationFactors(Id const &id);
//            QString hasModifier(Id const &id, QString const &modifier);
//            QString getSuperclass(Id const &id);
//            QString getMethodCode(Id const &id);
//            QString getFlowGuard(Id const &id);
//
//            QString serializeMultiplicity(Id const &id, QString const &multiplicity) const;
//            bool isTypeSuitable(QString const &type) const;
//            bool isVisibilitySuitable(QString const &type) const;
//
//            void addError(QString const &errorText);
//
//            qrRepo::RepoApi const &mApi;
//            QString mErrorText;
//            QString pathToDir;
        };

    }
}
