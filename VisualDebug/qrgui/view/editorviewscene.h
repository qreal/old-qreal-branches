#pragma once

#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QSignalMapper>
#include "../kernel/roles.h"
#include "../umllib/uml_nodeelement.h"
#include "gestures/mousemovementmanager.h"

const int indexGrid = 30; // distance between two lines in the grid

namespace qReal {
	class EditorViewMViface;
	class EditorView;
	class MainWindow;
	namespace model {
		class Model;
	}
}

class EditorViewScene : public QGraphicsScene
{
	Q_OBJECT

public:
	explicit EditorViewScene(QObject *parent = 0);
	~EditorViewScene();

	void clearScene();

	virtual int launchEdgeMenu(UML::EdgeElement* edge, UML::NodeElement* node, QPointF scenePos);
	virtual qReal::Id *createElement(const QString &, QPointF scenePos);
	virtual void createElement(const QMimeData *mimeData, QPointF scenePos);

	// is virtual only to trick linker. is used from plugins and generators and we have no intention of
	// including the scene (with dependencies) there
	virtual UML::Element *getElem(qReal::Id const &uuid);
	virtual UML::Element *getElemByModelIndex(const QModelIndex& index );

	virtual QPersistentModelIndex rootItem();
	void setMainWindow(qReal::MainWindow *mainWindow);
	qReal::MainWindow *mainWindow() const;
	void setEnabled(bool enabled);

	void setNeedDrawGrid(bool show);

	bool canBeContainedBy(qReal::Id container, qReal::Id candidate);
	bool getNeedDrawGrid();

	UML::Element* getLastCreated();

signals:
	void elementCreated(qReal::Id const &id);

protected:
	void dragEnterEvent( QGraphicsSceneDragDropEvent *event);
	void dragMoveEvent( QGraphicsSceneDragDropEvent *event);
	void dragLeaveEvent( QGraphicsSceneDragDropEvent *event);
	void dropEvent ( QGraphicsSceneDragDropEvent *event);

	void keyPressEvent( QKeyEvent *event);

	void mousePressEvent( QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );
	void mouseMoveEvent (QGraphicsSceneMouseEvent *event);

	void mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event);

	virtual void drawBackground( QPainter *painter, const QRectF &rect);

private:
	UML::Element* lastCreatedWithEdge;

	bool mRightButtonPressed;
	bool mNeedDrawGrid; // if true, the grid will be shown (as scene's background)
	qreal mWidthOfGrid;

	void getObjectByGesture();
	void getLinkByGesture(UML::NodeElement * parent, UML::NodeElement const & child);
	void drawGesture();
	void deleteGesture();
	void createEdgeMenu(QList<QString> const & ids);

	void drawGrid(QPainter *painter, const QRectF &rect);

	UML::Element *getElemAt(const QPointF &position);

	void createConnectionSubmenus(QMenu &contextMenu, UML::Element const * const element) const;
	void createGoToSubmenu(QMenu * const goToMenu, QString const &name, qReal::IdList const &ids) const;
	void createAddConnectionMenu(UML::Element const * const element
								 , QMenu &contextMenu, QString const &menuName
								 , qReal::IdList const &connectableTypes, qReal::IdList const &alreadyConnectedElements
								 , qReal::IdList const &connectableDiagrams, const char *slot) const;

	void createDisconnectMenu(UML::Element const * const element
							  , QMenu &contextMenu, QString const &menuName
							  , qReal::IdList const &outgoingConnections, qReal::IdList const &incomingConnections
							  , const char *slot) const;

	void initContextMenu(UML::Element *e, QPointF const & pos);
	qReal::model::Model* model() const;

	QPointF newElementsPosition;
	QList<QGraphicsItem*> mGesture;

	qReal::EditorViewMViface *mv_iface;
	qReal::EditorView *view;

	qReal::MainWindow *mWindow;

	QPointF mPrevPosition;
	QGraphicsItem *mPrevParent;

	QPointF mCreatePoint;

	MouseMovementManager * mouseMovementManager;

	QSignalMapper *mActionSignalMapper;

	friend class qReal::EditorViewMViface;

public slots:

	qReal::Id *createElement(const QString &);
	// TODO: get rid of it here
private slots:

	void connectActionTriggered();
	void goToActionTriggered();
	void disconnectActionTriggered();
	void addUsageActionTriggered();
	void deleteUsageActionTriggered();
	void printElementsOfRootDiagram();
	void drawIdealGesture();
	void initMouseMoveManager();
	void createEdge(QString const &);
};
