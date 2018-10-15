#include "StdAfx.h"
#include "ServerHelper.h"

QString ServerHelper::s_serverUrl = "";

ServerHelper::ServerHelper(const QString& url, const QByteArray& content, http_callback const& receiver, QString &contenttype)
{
	m_url = url;
	m_content = content;
	m_receiver = receiver;
	m_contenttype = contenttype;
}

ServerHelper::~ServerHelper()
{

}

void ServerHelper::request(QString cmd, QString &content, http_callback const& receiver)
{
	QString szUrl = s_serverUrl + "/" + cmd;
	QString szContentType = "application/json";

	ServerHelper* p = new ServerHelper(szUrl, content.toLatin1(), receiver, szContentType);
	p->doRequest();
}

void ServerHelper::doRequest()
{
	NetworkHelper::post( m_url, m_content, std::bind(&ServerHelper::OnNetworkReply, this, _1, _2), 5000, m_contenttype);
}

void ServerHelper::OnNetworkReply( int errCode, const QByteArray& bytes)
{
	if (!m_receiver)
	{
		delete this;
		return;
	}

	if (errCode==E_NetTimeOut)
	{
		m_receiver(E_ServerTimeOut, "Not received reply unitl timeout.", "");
	}
	else if (errCode==E_NetReplyError)
	{
		m_receiver(E_ServerNetReplyError, "Network reply error.", "");
	}
	else if (errCode==E_NetOK)
	{
		m_receiver(E_ServerOK, "", bytes);
	}
	delete this;
}

