#include "stdafx.h"
#include "NetworkHelper.h"

NetworkHelper::NetworkHelper(QString url, QByteArray content, ReceiveFun receiver, int timeout, QString contenttype)
{
	m_url = url;
	m_content = content;
	m_receiver = receiver;
	m_timeout = timeout;
	m_ContentType = contenttype;

	m_pNetworkAccessManager = new QNetworkAccessManager();
	m_pTimer = new QTimer();

	connect( m_pNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(OnReplyFinished(QNetworkReply*)) );
	connect( m_pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()) );
}

NetworkHelper::~NetworkHelper()
{
	SafeDelete(m_pNetworkAccessManager);
	SafeDelete(m_pTimer);
}

void NetworkHelper::get(const QString& url, ReceiveFun receiver, int timeout, QString contenttype)
{
	NetworkHelper* p = new NetworkHelper(url, "", receiver, timeout, contenttype);
	p->excuteGet();
}

void NetworkHelper::post(const QString& url, const QByteArray& content, ReceiveFun receiver, int timeout, QString contenttype)
{
	NetworkHelper* p = new NetworkHelper(url, content, receiver, timeout, contenttype);
	p->excutePost();
}

void NetworkHelper::OnReplyFinished( QNetworkReply* reply )
{
	if (m_pTimer->isActive())
	{
		m_pTimer->stop();
		if (reply->error() == QNetworkReply::NoError)
		{
			QByteArray bytes = reply->readAll();
			if (m_receiver)
			{
				RTC_LOG(LS_INFO) << QT_TO_STD(QString("E_NetOK bytes[%1]").arg(QString(bytes)));
				m_receiver(E_NetOK, bytes);
			}
		}
		else
		{
			if (m_receiver)
			{
				m_receiver(E_NetReplyError, QByteArray());
			}
			QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
			RTC_LOG(LS_INFO) << QT_TO_STD(QString("found error ....code: %1 %2").arg(statusCodeV.toInt()).arg((int)reply->error()));
			RTC_LOG(LS_INFO) << qPrintable(reply->errorString());
		}

		this->deleteLater();
	}

	reply->deleteLater();
}

void NetworkHelper::OnTimer()
{
	m_pTimer->stop();
	if (m_receiver)
	{
		RTC_LOG(LS_INFO) << "E_NetTimeOut";
		m_receiver(E_NetTimeOut, QByteArray()); //超时
	}
	this->deleteLater();
}

void NetworkHelper::excuteGet()
{
	RTC_LOG(LS_INFO) << QT_TO_STD(m_url);

	QNetworkRequest network_request;
	network_request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(m_ContentType));
	network_request.setUrl( QUrl(m_url) );
	m_pNetworkAccessManager->get(network_request);
	m_pTimer->start(m_timeout);
}

void NetworkHelper::excutePost()
{
	RTC_LOG(LS_INFO) << QT_TO_STD(m_url);

	QNetworkRequest network_request;
	network_request.setUrl( QUrl(m_url) );
	network_request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(m_ContentType));	
	network_request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()).toString());

	if (m_url.startsWith("https", Qt::CaseInsensitive))
	{
		//QNetworkAccessManager建立SSH连接时出现SslHandshakeFailedError
		QSslConfiguration config = QSslConfiguration::defaultConfiguration();
		// 	config.setProtocol(QSsl::TlsV1);
		config.setPeerVerifyMode(QSslSocket::VerifyNone);
		network_request.setSslConfiguration(config);
	}

	m_pNetworkAccessManager->post(network_request, m_content);
	m_pTimer->start(m_timeout);
}

