#include "QtWebrtc.h"
#include <QCloseEvent>
#define QT_TO_STD(qt) (qt.toLatin1().data())
#define STD_TO_QT(std) (QString::fromStdString(std))


QtWebrtc::QtWebrtc(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

#if 1
	//for peerconnection_client.exe
	ui.lEIP->setText("192.168.1.223");
#else
	//licode
	ui.lEIP->setText("https://192.168.1.107:3004");
#endif
	//janus
	ui.lEIP->setText("ws://192.168.1.222:8188");

	logic.Init();

	QVector<HWND>remote;

	remote.push_back((HWND)ui.remoteRender_1->winId());
	remote.push_back((HWND)ui.remoteRender_2->winId());
	remote.push_back((HWND)ui.remoteRender_3->winId());

 	logic.SetWnds((HWND)ui.localRender->winId(), remote);
}

QtWebrtc::~QtWebrtc()
{

}

void QtWebrtc::showEvent(QShowEvent *event)
{
	on_btnCamreaRefresh_clicked();
	on_btnScreenRefresh_clicked();
	on_btnWindowRefresh_clicked();
// 	logic.Login(ui.lEIP->text());
}

void QtWebrtc::on_btnLogin_clicked()
{
	logic.Login(ui.lEIP->text());
}

void QtWebrtc::on_btnLogout_clicked()
{
	logic.Logout();
}

void QtWebrtc::on_btnConnectPeer_clicked()
{
	logic.ConnectToPeer(ui.lEPeerID->text().toInt());
}

void QtWebrtc::on_btnDisConnectPeer_clicked()
{
	logic.DisConnectToPeer();
}

void QtWebrtc::closeEvent(QCloseEvent *event)
{
	logic.Uninit();

	event->accept();
}

void QtWebrtc::on_btnPreviewStart_clicked()
{
	logic.StartPreview();
}

void QtWebrtc::on_btnPreviewStop_clicked()
{
	logic.StopPreview();
}


#pragma region media

void QtWebrtc::on_btnClose_clicked()
{
	logic.CloseVideo();
}

void QtWebrtc::on_btnCamreaRefresh_clicked()
{
	ui.cbCamera->clear();
	QVector<QString> cameras;
	logic.GetCameraList(cameras);

	for (auto camrea : cameras)
	{
		ui.cbCamera->addItem(camrea);
	}
}

void QtWebrtc::on_btnCamreaOpen_clicked()
{
	QString camera = ui.cbCamera->currentText();
	logic.OpenCamrea(camera);
}

void QtWebrtc::on_btnScreenRefresh_clicked()
{
	ui.cbScreenTitle->clear();
	ui.cbScreenId->clear();

	QVector<QPair<QString, int> > screens;
	logic.GetScreenList(screens);

	for (auto screen : screens)
	{
		ui.cbScreenTitle->addItem(screen.first);
		ui.cbScreenId->addItem(QString::number(screen.second));
	}

	on_btnScreenRect_clicked();
}

void QtWebrtc::on_btnScreenRect_clicked()
{
	QRect rect= logic.GetScreenRect(ui.cbScreenId->currentText().toInt());

	ui.lEScreenLeft->setText(QString::number(rect.left()));
	ui.lEScreenTop->setText(QString::number(rect.top()));
	ui.lEScreenRight->setText(QString::number(rect.right()));
	ui.lEScreenBottom->setText(QString::number(rect.bottom()));
}

void QtWebrtc::on_btnScreenOpen_clicked()
{
	int screenId = ui.cbScreenId->currentText().toInt();
	if (ui.rBFullScreen->isChecked())
	{
		logic.OpenScreen(screenId);
	}
	else
	{
		int left = ui.lEScreenLeft->text().toInt();
		int top = ui.lEScreenTop->text().toInt();
		int right = ui.lEScreenRight->text().toInt();
		int bottom = ui.lEScreenBottom->text().toInt();

		logic.OpenScreen(screenId, QRect(left, top, right - left + 1, bottom - top + 1));
	}
}

void QtWebrtc::on_btnWindowRefresh_clicked()
{
	ui.cbWindowTitle->clear();
	ui.cbWindowId->clear();

	QVector <QPair<QString, int> > windows;
	logic.GetWindowList(windows);

	for (auto window : windows)
	{
		ui.cbWindowTitle->addItem(window.first);
		ui.cbWindowId->addItem(QString::number(window.second));
	}
}

void QtWebrtc::on_btnWindowOpen_clicked()
{
	QString windowId = ui.cbWindowId->currentText();
	logic.OpenWindow(windowId.toInt());
}


void QtWebrtc::on_cbWindowTitle_currentIndexChanged(int index)
{
	ui.cbWindowId->blockSignals(true);
	ui.cbWindowId->setCurrentIndex(index);
	ui.cbWindowId->blockSignals(false);
}

void QtWebrtc::on_cbWindowId_currentIndexChanged(int index)
{
	ui.cbWindowTitle->blockSignals(true);
	ui.cbWindowTitle->setCurrentIndex(index);
	ui.cbWindowTitle->blockSignals(false);
}

void QtWebrtc::on_btnSendEnable_clicked()
{
	logic.EnableSend(true);
}

void QtWebrtc::on_btnSendDisable_clicked()
{
	logic.EnableSend(false);
}


#pragma endregion

