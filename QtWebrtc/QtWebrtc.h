#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtWebrtc.h"
#include "WebrtcLogic.h"

class QtWebrtc : public QMainWindow
{
	Q_OBJECT

public:
	QtWebrtc(QWidget *parent = Q_NULLPTR);
	~QtWebrtc();

protected:
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);
private slots:
	void on_btnLogin_clicked();
	void on_btnLogout_clicked();

	void on_btnConnectPeer_clicked();
	void on_btnDisConnectPeer_clicked();

	void on_btnPreviewStart_clicked();
	void on_btnPreviewStop_clicked();

	void on_btnClose_clicked();
	void on_btnCamreaRefresh_clicked();
	void on_btnCamreaOpen_clicked();

	void on_btnScreenRefresh_clicked();
	void on_btnScreenRect_clicked();
	void on_btnScreenOpen_clicked();

	void on_btnWindowRefresh_clicked();
	void on_btnWindowOpen_clicked();

	void on_cbWindowTitle_currentIndexChanged(int index);
	void on_cbWindowId_currentIndexChanged(int index);

	void on_btnSendEnable_clicked();
	void on_btnSendDisable_clicked();

private:
	Ui::QtWebrtcClass ui;

	WebrtcLogic logic;
};
