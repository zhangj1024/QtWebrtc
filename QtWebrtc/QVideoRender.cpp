#include "QVideoRender.h"

QVideoRender::QVideoRender(QWidget *parent)
	: QWidget(parent)
{
}

QVideoRender::~QVideoRender()
{

}

void QVideoRender::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (isFullScreen())
	{
		exitFullScreen();
	}
	else
	{
		enterFullScreen();
	}
}

void QVideoRender::enterFullScreen()
{
	if (!isFullScreen())
	{
		m_pParentWidget = parentWidget();
		m_Rect = geometry();
		this->setParent(NULL);
		this->showFullScreen();
	}
}

void QVideoRender::exitFullScreen()
{
	if (isFullScreen())
	{
		this->setParent(m_pParentWidget);
		this->setGeometry(m_Rect);
		this->showNormal();
	}
}