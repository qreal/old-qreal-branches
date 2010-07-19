#pragma once

#include <QtGui/QWidget>
#include <QtCore/QString>
#include <QtXml/QDomDocument>

#include "scene.h"
#include "item.h"

namespace Ui {
	class ShapeEdit;
}

class ShapeEdit : public QWidget {
	Q_OBJECT
public:
	explicit ShapeEdit(QWidget *parent = NULL);
	ShapeEdit(QPersistentModelIndex const &index, int const &role);
	void load(const QString &text);
	~ShapeEdit();
signals:
	void shapeSaved(QString const &shape, QPersistentModelIndex const &index, int const &role);

protected:
	void changeEvent(QEvent *e);

private slots:
	void saveToXml();
	void save();
	void open();
	void setNoPalette();
	void setItemPalette(QPen const &penItem, QBrush const &brushItem);

private:
	Scene *mScene;
	QGraphicsItemGroup mItemGroup;
	QDomDocument mDocument;
	QPointF mTopLeftPicture;
	Ui::ShapeEdit *mUi;
	QPersistentModelIndex const mIndex;
	int const mRole;
	void initPalette();
	void init();

	void setValuePenStyleComboBox(Qt::PenStyle penStyle);
	void setValuePenColorComboBox(QColor penColor);
	void setValuePenWidthSpinBox(int width);
	void setValueBrushStyleComboBox(Qt::BrushStyle brushStyle);
	void setValueBrushColorComboBox(QColor brushColor);

	void generateDom();
	void exportToXml(QString const &fileName);
	QList<QDomElement> generateGraphics();
};
