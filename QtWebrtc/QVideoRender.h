#pragma once

#include <QWidget>
#include <QMouseEvent>

class QVideoRender : public QWidget
{
	Q_OBJECT

public:
	QVideoRender(QWidget *parent);
	~QVideoRender();

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent * event);

private:
	void enterFullScreen();
	void exitFullScreen();

	QWidget *m_pParentWidget;
	QRect m_Rect;
};
