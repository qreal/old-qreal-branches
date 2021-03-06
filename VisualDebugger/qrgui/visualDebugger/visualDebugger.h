#pragma once

#include <QList>
#include <QtGui/QGraphicsEffect>

#include "../view/editorview.h"
#include "../mainwindow/errorReporter.h"

#include "blockParser.h"

namespace qReal {
	class VisualDebugger : QObject
	{
		Q_OBJECT

	public:
		enum DebugType {
			noDebug,
			singleStepDebug,
			fullDebug
		};
	public:
		VisualDebugger(models::LogicalModelAssistApi const &modelApi, models::GraphicalModelAssistApi const &mGraphicalModelApi);
		~VisualDebugger();
		void clearErrorReporter();
		void setEditor(EditorView *editor);
		bool canDebug(VisualDebugger::DebugType type);
		
		void createIdByLineCorrelation();
		QList<int>* computeBreakpoints();
		Id getIdByLine(int line);
		void lightElement(Id id);
	public slots:
		void generateCode();
		gui::ErrorReporter& debug();
		gui::ErrorReporter& debugSingleStep();
	private:
		enum ErrorType {
			missingBeginNode,
			missingEndOfLinkNode,
			endWithNotEndNode,
			missingValidLink,
			wrongEditor,
			someDiagramIsRunning,
			noErrors
		};
	private:
		EditorView *mEditor;
		models::LogicalModelAssistApi const &mLogicalModelApi;
		models::GraphicalModelAssistApi const &mGraphicalModelApi;
		UML::Element *mCurrentElem;
		VisualDebugger::ErrorType mError;
		Id mCurrentId;
		QGraphicsColorizeEffect *mEffect;
		gui::ErrorReporter *mErrorReporter;
		BlockParser *mBlockParser;
		int mTimeout;
		DebugType mDebugType;
		QColor mDebugColor;
		QMap<int, Id> mIdByLineCorrelation;

		void error(ErrorType e);
		ErrorType checkEditor();
		UML::Element* findBeginNode(QString name);
		UML::Element* findEndNode(QString name);
		Id findValidLink();
		void pause(int time);
		bool isFinalNode(Id id);
		bool hasEndOfLinkNode(Id id);
		ErrorType doFirstStep(UML::Element* elem);
		void doStep(Id id);
		void deinitialize();
		void processAction();
		void setTimeout(int timeout);
		void setDebugColor(QString color);
		void generateCode(UML::Element* elem, QFile &codeFile);
		QVariant getProperty(Id id, QString propertyName);
		void createIdByLineCorrelation(UML::Element *elem, int& line);
	};
}
