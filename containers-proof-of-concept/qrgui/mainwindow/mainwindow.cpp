#include "mainwindow.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSvg/QSvgGenerator>
#include <QtCore/QPluginLoader>
#include <QtGui/QProgressBar>
#include <QtCore/QSettings>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <QtCore/QDebug>

#include "dialogs/plugindialog.h"
#include "editorinterface.h"
#include "../model/model.h"
#include "../view/editorview.h"
#include "../umllib/uml_element.h"
#include "../generators/xmi/xmiHandler.h"

using namespace qReal;

MainWindow::MainWindow()
		: mPropertyModel(mgr)
{
	QSettings settings("SPbSU", "QReal");
	bool showSplash = settings.value("ShowSplashScreen", true).toBool();
	QSplashScreen* splash =
		new QSplashScreen(QPixmap(":/icons/kroki2.PNG"), Qt::SplashScreen | Qt::WindowStaysOnTopHint);

	QProgressBar *progress = new QProgressBar((QWidget*) splash);
	progress->move(20,270);
	progress->setFixedWidth(600);
	progress->setFixedHeight(15);
	progress->setRange(0, 100);
	progress->setValue(5);
	if (showSplash)
	{
		splash->show();
		QApplication::processEvents();
	}

	ui.setupUi(this);

	if (!showSplash)
		ui.actionShowSplash->setChecked(false);

	ui.view->setMainWindow(this);

	ui.minimapView->setScene(ui.view->scene());
	ui.minimapView->setRenderHint(QPainter::Antialiasing, true);

	// activated под линуксом - одинарный клик, что очень плохо.
	// connect(ui.diagramExplorer, SIGNAL(activated(const QModelIndex &)),
	//	ui.view->mvIface(), SLOT(setRootIndex(const QModelIndex &)));
	connect(ui.view->scene(), SIGNAL(selectionChanged()), SLOT(sceneSelectionChanged()));
	connect(ui.diagramExplorer, SIGNAL(clicked(const QModelIndex &)),
		this, SLOT(activateItemOrDiagram(const QModelIndex &)));

	progress->setValue(20);

	connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(close()));

	connect(ui.actionZoom_In, SIGNAL(triggered()), ui.view, SLOT(zoomIn()));
	connect(ui.actionZoom_Out, SIGNAL(triggered()), ui.view, SLOT(zoomOut()));

	connect(ui.actionAntialiasing, SIGNAL(toggled(bool)), ui.view, SLOT(toggleAntialiasing(bool)));
	connect(ui.actionOpenGL_Renderer, SIGNAL(toggled(bool)), ui.view, SLOT(toggleOpenGL(bool)));
	connect(ui.actionShowSplash, SIGNAL(toggled(bool)), this, SLOT (toggleShowSplash(bool)));

	connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
	connect(ui.actionMakeSvg, SIGNAL(triggered()), this, SLOT(makeSvg()));

	connect(ui.actionDeleteFromDiagram, SIGNAL(triggered()), this, SLOT(deleteFromDiagram()));

	connect(ui.actionExport_to_XMI, SIGNAL(triggered()), this, SLOT(exportToXmi()));

	connect(ui.actionPlugins, SIGNAL(triggered()), this, SLOT(settingsPlugins()));

	connect(ui.actionHelp, SIGNAL(triggered()), this, SLOT(showHelp()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
	connect(ui.actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	connect(ui.minimapZoomSlider, SIGNAL(valueChanged(int)), this, SLOT(adjustMinimapZoom(int)));
	adjustMinimapZoom(ui.minimapZoomSlider->value());

	progress->setValue(40);

	// XXX: kludge... don't know how to do it in designer
	ui.diagramDock->setWidget(ui.diagramExplorer);

	ui.paletteDock->setWidget(ui.paletteToolbox);

	//ui.propertyEditor->horizontalHeader()->setStretchLastSection(true);
	//ui.propertyEditor->horizontalHeader()->hide();
	//	ui.propertyEditor->setModel(&propertyModel);

	ui.propertyEditor->setModel(&mPropertyModel);
	ui.propertyEditor->verticalHeader()->hide();
	ui.propertyEditor->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	ui.propertyEditor->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	ui.propertyEditor->setItemDelegate(&mDelegate);

	connect(ui.diagramExplorer, SIGNAL(clicked(QModelIndex const &)),
		&mPropertyModel, SLOT(setIndex(QModelIndex const &)));

	ui.diagramExplorer->addAction(ui.actionDeleteFromDiagram);

	settings.beginGroup("MainWindow");
	resize(settings.value("size", QSize(1024, 800)).toSize());
	move(settings.value("pos", QPoint(0, 0)).toPoint());
	settings.endGroup();
	progress->setValue(60);

	loadPlugins();
	showMaximized();

	progress->setValue(70);

	mModel = new model::Model(mgr);
	IdList missingPlugins = mgr.checkNeededPlugins(mModel->api());
	if (!missingPlugins.isEmpty()) {
		QString text = "These plugins are not present, but needed to load the save:\n";
		foreach (Id const id, missingPlugins) {
			text += id.editor() + "\n";
		}
		QMessageBox::warning(this, tr("Some plugins are missing"), text);
		close();  // Всё, собственно.
		return;
	}

	connect(ui.actionClear, SIGNAL(triggered()), mModel, SLOT(exterminate()));

	progress->setValue(80);

	mPropertyModel.setSourceModel(mModel);
	ui.diagramExplorer->setModel(mModel);
	ui.diagramExplorer->setRootIndex(mModel->rootIndex());

	ui.view->mvIface()->setModel(mModel);
	connect(mModel, SIGNAL(layoutChanged()), ui.view->mvIface(), SLOT(updateGeometries()));
	ui.view->mvIface()->setRootIndex(mModel->rootIndex());
	progress->setValue(100);
	if (showSplash)
		splash->close();
	delete splash;
}

MainWindow::~MainWindow()
{
	delete mModel;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings("SPbSU", "QReal");
	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.endGroup();
	event->accept();
}

void MainWindow::loadPlugins()
{
	foreach (Id const editor, mgr.editors()) {
		foreach (Id const diagram, mgr.diagrams(editor)) {
			ui.paletteToolbox->addDiagramType(diagram, mgr.friendlyName(diagram) );

			foreach (Id const element, mgr.elements(diagram)) {
				ui.paletteToolbox->addItemType(element, mgr.friendlyName(element), mgr.icon(element));
			}
		}
	}
	ui.paletteToolbox->initDone();
}

void MainWindow::adjustMinimapZoom(int zoom)
{
	ui.minimapView->resetMatrix();
	ui.minimapView->scale(0.01*zoom,0.01*zoom);
}

void MainWindow::activateItemOrDiagram(const QModelIndex &idx)
{
	QModelIndex parent = idx.parent();

	// Is level-one diagram?
	if (!parent.isValid())
	{
		// Показать "корневую диаграмму". На самом деле, её, вероятно, не будет,
		// будет набор корневых диаграмм.
		ui.view->mvIface()->setRootIndex(mModel->rootIndex());
		ui.diagramExplorer->setCurrentIndex(mModel->rootIndex());
	}
	else
	{
		if (ui.view->mvIface()->rootIndex() != parent)
		{
			// activate parent diagram
			ui.view->mvIface()->setRootIndex(parent);
		}
	}

	// select this item on diagram
	ui.view->scene()->clearSelection();
	UML::Element *e = (static_cast<EditorViewScene *>(ui.view->scene()))->getElemByModelIndex(idx);
	if (e)
		e->setSelected(true);
	else
		qDebug() << "shit happened!!!\n";
}

void MainWindow::activateSubdiagram(QModelIndex const &idx) {
	QModelIndex diagramToActivate = idx;
	while (diagramToActivate.isValid() && diagramToActivate.parent().isValid()
		&& diagramToActivate.parent() != ui.view->mvIface()->rootIndex())
	{
		diagramToActivate = diagramToActivate.parent();
	}

	if (diagramToActivate.model()->rowCount(diagramToActivate) > 0) {
		QModelIndex childIndex = diagramToActivate.model()->index(0, 0, diagramToActivate);
		activateItemOrDiagram(childIndex);
	}
}

void MainWindow::sceneSelectionChanged()
{
	QList<QGraphicsItem*> graphicsItems = ui.view->scene()->selectedItems();
	if (graphicsItems.size() == 1) {
		QGraphicsItem *item = graphicsItems[0];
		if (UML::Element *elem = dynamic_cast<UML::Element *>(item)) {
			if (elem->index().isValid()) {
				ui.diagramExplorer->setCurrentIndex(elem->index());
				mPropertyModel.setIndex(elem->index());
			}
		}
	}
	else
	{
		ui.diagramExplorer->setCurrentIndex(QModelIndex());
		mPropertyModel.setIndex(QModelIndex());
	}
}

void MainWindow::print()
{
	QPrinter printer(QPrinter::HighResolution);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec() == QDialog::Accepted) {
		QPainter painter(&printer);
		//		QRect allScene = pieChart->mapFromScene(pieChart->scene()->sceneRect()).boundingRect();
		ui.view->scene()->render(&painter);
	}
}

void MainWindow::makeSvg()
{
	QSvgGenerator newSvg;

	QString fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty())
		return;

	newSvg.setFileName(fileName);
	newSvg.setSize(QSize(800,600));

	QPainter painter(&newSvg);
	ui.view->scene()->render(&painter);
}

void MainWindow::settingsPlugins()
{
	PluginDialog dialog( mgr , this);
	dialog.exec();
}

void MainWindow::deleteFromExplorer()
{
	QModelIndex idx = ui.diagramExplorer->currentIndex();
	if (idx.isValid())
		mModel->removeRow(idx.row(), idx.parent());
}

void MainWindow::deleteFromScene()
{
	foreach (QGraphicsItem *item, ui.view->scene()->selectedItems())
		if (UML::Element *elem = dynamic_cast<UML::Element *>(item))
			if (elem->index().isValid())
				mModel->removeRow(elem->index().row(), elem->index().parent());
}

void MainWindow::deleteFromDiagram()
{
	if (mModel) {
		if (ui.diagramExplorer->hasFocus()) {
			deleteFromExplorer();
		} else if (ui.view->hasFocus()) {
			deleteFromScene();
		}
	}
}

void MainWindow::showAbout()
{
	QMessageBox::about(this, tr("About QReal"),
			tr("<center>This is <b>QReal</b><br>"
				"Just another CASE tool</center>"));
}

void MainWindow::showHelp()
{
	QMessageBox::about(this, tr("Help"),
			tr("To begin:\n"
				"1. To add items to diagrams, drag & drop them from Palette to editor\n"
				"2. Get more help from author :)"));
}

void MainWindow::toggleShowSplash(bool show)
{
	QSettings settings("SPbSU", "QReal");
	settings.setValue("ShowSplashScreen", show);
}

void MainWindow::exportToXmi()
{
	generators::XmiHandler xmi(mModel->api());

	QString const fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty())
		return;

	QString const errors = xmi.exportToXmi(fileName);

	if (!errors.isEmpty()) {
		QMessageBox::warning(this, tr("errors"), "Some errors occured. Export may be incorrect. Errors list: \n" + errors);
	} else {
		QMessageBox::information(this, tr("finished"), "Export is finished");
	}

	qDebug() << "Done.";
}
