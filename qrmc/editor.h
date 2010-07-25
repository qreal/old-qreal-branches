#pragma once

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtCore/QSet>

#include "../../trunk/qrrepo/repoApi.h"


class MetaCompiler;
class Diagram;
class Type;
class EnumType;

namespace utils {
	class OutFile;
}

class Editor
{
public:
	Editor(MetaCompiler *metaCompiler, qrRepo::RepoApi *api, qReal::Id const &id);
	~Editor();
	MetaCompiler *metaCompiler();
	bool isLoaded();
	bool load();
	void generate(QString const &headerTemplate, QString const &sourceTemplate, QMap<QString, QString> const &utils);

	Type *findType(QString const &name);
	QSet<EnumType*> getAllEnumTypes();
	Diagram *findDiagram(QString const &name);
	QMap<QString, Diagram*> diagrams();

	QString name();

private:
	bool generatePluginHeader(QString const &headerTemplate);
	bool generatePluginSource();

	void generateDiagramsMap();
	void generateDiagramNodeNamesMap();
	void generateNamesMap();
	void generateMouseGesturesMap();
	void generatePropertiesMap();
	void generatePropertyDefaultsMap();
	void generateElementsFactory();
	void generateContaners();
	void generateConnections();
	void generateUsages();

	MetaCompiler *mMetaCompiler;
	qrRepo::RepoApi *mApi;
	qReal::Id mId;
	QString mName;
	bool mLoadingComplete;
	QList<Editor*> mIncludes;
	QMap<QString, Diagram*> mDiagrams;

	QMap<QString, QString> mUtilsTemplate;
	QString mSourceTemplate;

};
