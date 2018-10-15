#ifndef NetworkHelper_h_
#define NetworkHelper_h_
#include <QNetworkReply>

#define LimitTimeOut 5000 //请求超时时间(毫秒)

enum E_NetworkReply
{
	E_NetOK = 1,
	E_NetTimeOut,//超时
	E_NetReplyError,
};

typedef std::function<void(int errCode, const QByteArray& bytes)> ReceiveFun;

class NetworkHelper : public QObject
{
	Q_OBJECT
public:
	static void get(const QString& url, ReceiveFun receiver, int timeout, QString contenttype);
	static void post(const QString& url, const QByteArray& content, ReceiveFun receiver, int timeout, QString contenttype);

private slots:
	void OnReplyFinished(QNetworkReply* reply);
	void OnTimer();

private:
	NetworkHelper(QString url, QByteArray content, ReceiveFun receiver, int timeout, QString contenttype);
	~NetworkHelper();
	
	void excuteGet();
	void excutePost();

private:
	QNetworkAccessManager* m_pNetworkAccessManager;
	QTimer*		m_pTimer;

	QString		m_url;
	QByteArray	m_content;
	ReceiveFun  m_receiver;
	int			m_timeout;
	QString     m_ContentType;
};

#endif//NetworkHelper_h_