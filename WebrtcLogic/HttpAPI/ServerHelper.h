#pragma once
#include "NetworkHelper.h"

enum E_ServerRelpy
{
	E_ServerOK = 1,
	E_ServerTimeOut,//³¬Ê±
	E_ServerNetReplyError,
	E_ServerJsonParseError,//Json½âÎö´íÎó
};

// typedef void (*ServerRecFun)(int errorCode, QString errorInfo, QByteArray bytes, void* pCusData);

typedef std::function<void(int errorCode, QString errorInfo, QByteArray bytes)> http_callback;

class ServerHelper
{
public:
	static void request(QString cmd, QString &content, http_callback const& receiver);

	static QString s_serverUrl;
private:
	ServerHelper(const QString &url, const QByteArray &content, http_callback const &receiver, QString &contenttype);
	~ServerHelper();
	void doRequest();

	void OnNetworkReply(int errCode, const QByteArray& bytes);

private:
	QString	  m_url;
	QByteArray  m_content;
	QString	  m_contenttype;
	http_callback m_receiver;
};

