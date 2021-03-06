#pragma once

#include <QtGui/QMainWindow>
#include <QtSql/QSqlDatabase>
#include <QtCore/QDir>
#include <QSplashScreen>
#include <QtGui>

#include "../editorManager/editorManager.h"
#include "propertyeditorproxymodel.h"
#include "propertyeditordelegate.h"
#include "igesturespainter.h"
#include "gesturesShow/gestureswidget.h"

namespace Ui{
class MainWindowUi;
}

namespace qReal {

class EditorView;
class ListenerManager;
class VisualDebugger;

namespace models {
class Models;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

	EditorManager* manager();
	EditorView *getCurrentTab();
	ListenerManager *listenerManager();
	IGesturesPainter *gesturesPainter();
	QModelIndex rootIndex() const;

	QAction *actionDeleteFromDiagram() const;
	void setIndexesOfPropertyEditor(Id const &id);

signals:
	void gesturesShowed();
	void currentIdealGestureChanged();
	void rootDiagramChanged();

public slots:
	void adjustMinimapZoom(int zoom);
	void toggleShowSplash(bool show);

	void updateTabName(Id const &id);

	void settingsPlugins();

	void showAbout();
	void showHelp();

	void checkoutDialogOk();
	void checkoutDialogCancel();
	void open();
	void saveAs();
	void saveAll();

	void print();
	void makeSvg();
	void showGrid(bool isChecked);

	void finalClose();

	void sceneSelectionChanged();

	void doCheckout();
	void doCommit();
	void exportToXmi();
	void generateToJava();
	void parseJavaLibraries();
	void deleteFromScene();
	void deleteFromScene(QGraphicsItem *target);

	void activateSubdiagram(QModelIndex const &idx);
	void activateItemOrDiagram(Id const &id, bool bl = true, bool isSetSel = true);
	void activateItemOrDiagram(QModelIndex const &idx, bool bl = true, bool isSetSel = true);
	void propertyEditorScrollTo(QModelIndex const &index);
	void selectItemWithError(Id const &id);

	void debug();
	void debugSingleStep();

private slots:
	void deleteFromDiagram();
	void changeMiniMapSource(int index);
	void closeTab(int index);
	void closeTab(QModelIndex const &graphicsIndex);
	void exterminate();
	void generateEditor();
	void generateEditorWithQRMC();
	void parseEditorXml();
	void generateToHascol();
	void parseHascol();
	void showPreferencesDialog();

	void centerOn(Id const &id);
	void graphicalModelExplorerClicked(const QModelIndex &index);
	void logicalModelExplorerClicked(const QModelIndex &index);

	void openShapeEditor();
	void openNewTab(const QModelIndex &index);
	void initCurrentTab(const QModelIndex &rootIndex);

	void showGestures();
	void showAlignment(bool isChecked);
	void switchGrid(bool isChecked);
	void switchAlignment(bool isChecked);
	void setShape( QString const &data, QPersistentModelIndex const &index, int const &role);

	void setDiagramCreateFlag();
	void diagramInCreateListDeselect();
	void diagramInCreateListSelected(int num);

private:
	Ui::MainWindowUi *mUi;

	QCloseEvent *mCloseEvent;
	models::Models *mModels;
	EditorManager mEditorManager;
	ListenerManager *mListenerManager;
	PropertyEditorModel *mPropertyModel;
	PropertyEditorDelegate mDelegate;
	GesturesWidget *mGesturesWidget;

	QVector<bool> mSaveListChecked;
	bool mDiagramCreateFlag;

	QStringList mDiagramsList;
	QModelIndex mRootIndex;

	void createDiagram(const QString &idString);
	void loadNewEditor(QString const &directoryName, QString const &metamodelName,
					   QString const &commandFirst, QString const &commandSecond, QString const &extension, QString const &prefix);

	void loadPlugins();

	QListWidget* createSaveListWidget();
	void suggestToSave();
	void suggestToCreateDiagram();

	virtual void closeEvent(QCloseEvent *event);
	void deleteFromExplorer(bool isLogicalModel);
	void keyPressEvent(QKeyEvent *event);
	QString getWorkingDir(QString const &dialogWindowTitle);

	int getTabIndex(const QModelIndex &index);

	void initGridProperties();
	void disconnectZoom(QGraphicsView* view);
	void connectZoom(QGraphicsView* view);
	void disconnectActionZoomTo(QWidget* widget);
	void connectActionZoomTo(QWidget* widget);
	void setConnectActionZoomTo(QWidget* widget);
	void clickErrorListWidget();
	VisualDebugger *mVisualDebugger;

	void setShowGrid(bool isChecked);
	void setShowAlignment(bool isChecked);
	void setSwitchGrid(bool isChecked);
	void setSwitchAlignment(bool isChecked);
};
}
