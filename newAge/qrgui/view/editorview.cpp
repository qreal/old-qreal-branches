#include <QtGui>

#ifdef QT_OPENGL_LIB
#include <QtOpenGL/QGLWidget>
#endif

#include "editorview.h"

using namespace qReal;

EditorView::EditorView(QWidget *parent)
	: QGraphicsView(parent), mSettings("SPbSU", "QReal")
{
	setRenderHint(QPainter::Antialiasing, true);

	mScene = new EditorViewScene(this);
	connect(mScene, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
	connect(mScene, SIGNAL(zoomOut()), this, SLOT(zoomOut()));

	mMVIface = new EditorViewMViface(this, mScene);
	setScene(mScene);

	setAcceptDrops(true);
	setDragMode(RubberBandDrag);
	setEnabled(false);

	setMouseTracking(true);
}

EditorView::~EditorView()
{
	delete mMVIface;
	delete mScene;
}

void EditorView::toggleAntialiasing(bool checked)
{
	setRenderHint(QPainter::Antialiasing, checked);
	setRenderHint(QPainter::SmoothPixmapTransform, checked);
}

void EditorView::toggleOpenGL(bool checked)
{
	Q_UNUSED(checked)
#ifdef QT_OPENGL_LIB
			setViewport(checked ? new QGLWidget(QGLFormat(QGL::SampleBuffers)) : new QWidget);
#endif
}

void EditorView::zoomIn()
{
	double zoomFactor = static_cast<double>(mSettings.value("ZoomFactor", 2).toInt()) / 10 + 1;
	scale(zoomFactor, zoomFactor);
}

void EditorView::zoomOut()
{
	double zoomFactor = 1 / (static_cast<double>(mSettings.value("ZoomFactor", 2).toInt()) / 10 + 1);
	scale(zoomFactor, zoomFactor);
}

void EditorView::setMainWindow(qReal::MainWindow *mainWindow)
{
	mMVIface->scene()->setMainWindow(mainWindow);
}

void EditorView::setDrawSceneGrid(bool show)
{
	mScene->setNeedDrawGrid(show);
	mScene->invalidate();
}

void EditorView::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::RightButton)
		setDragMode(NoDrag);
	else
		setDragMode(RubberBandDrag);
	if (mScene->getNeedDrawGrid())
		mScene->invalidate();
	QGraphicsView::mouseMoveEvent(event);
}

void EditorView::mouseReleaseEvent(QMouseEvent *event)
{
	QGraphicsView::mouseReleaseEvent(event);
	if (mScene->getNeedDrawGrid())
		mScene->invalidate();
}

void EditorView::scrollContentsBy(int dx, int dy)
{
	QGraphicsView::scrollContentsBy(dx, dy);
	if (mScene->getNeedDrawGrid())
		mScene->invalidate();
}

