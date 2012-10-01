#include "rest.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QtDebug>
#include <QMutex>
#include <QMutexLocker>

//--------------------------------------------------------------------------------------------------
// Rest
//--------------------------------------------------------------------------------------------------

Lvk::DAS::Rest::Rest(QObject *parent) :
    QObject(parent), m_manager(new QNetworkAccessManager(this)), m_reply(0),
    m_replyMutex(new QMutex(QMutex::Recursive))
{
}

//--------------------------------------------------------------------------------------------------

Lvk::DAS::Rest::~Rest()
{
    {
        QMutexLocker locker(m_replyMutex);
        delete m_reply;
        delete m_manager;
    }

    delete m_replyMutex;
}

//--------------------------------------------------------------------------------------------------

bool Lvk::DAS::Rest::request(const QString &url)
{
    QMutexLocker locker(m_replyMutex);

    if (m_reply) {
        if (m_reply->isRunning()) {
            qCritical() << "Previous REST request still runnning";
            return false;
        }

        delete m_reply;
        m_reply = 0;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "Chatbot");

    QNetworkReply *reply = m_manager->get(request);

    connect(reply, SIGNAL(finished()), SLOT(onFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            SLOT(onError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(onSslErrors(QList<QSslError>)));

    m_reply = reply;

    return true;
}

//--------------------------------------------------------------------------------------------------

void Lvk::DAS::Rest::abort()
{
    QMutexLocker locker(m_replyMutex);

    if (m_reply) {
        m_reply->abort();
    }
}

//--------------------------------------------------------------------------------------------------

void Lvk::DAS::Rest::onFinished()
{
    qDebug() << "Lvk::DAS::Rest::onFinished";

    QString resp;

    {
        QMutexLocker locker(m_replyMutex);
        resp = QString::fromUtf8(m_reply->readAll());
    }

    qDebug() << resp;
    emit response(resp);
}

//--------------------------------------------------------------------------------------------------

void Lvk::DAS::Rest::onError(QNetworkReply::NetworkError err)
{
    qDebug() << "Lvk::DAS::Rest::onError" << err;

    emit error(err);
}

//--------------------------------------------------------------------------------------------------

void Lvk::DAS::Rest::onSslErrors(const QList<QSslError> &/*errs*/)
{
    qDebug() << "Lvk::DAS::Rest::onSslErrors";
}