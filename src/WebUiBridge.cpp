#include "WebUiBridge.h"

#include <QJsonDocument>
#include <QJsonObject>
#include "Logger.h"

WebUiBridge::WebUiBridge(QObject* parent)
    : QObject(parent),
      m_webSocket(std::make_unique<QWebSocket>()),
      m_reconnectTimer(new QTimer(this)) {

    connect(m_webSocket.get(), &QWebSocket::connected, this, &WebUiBridge::onConnected);
    connect(m_webSocket.get(), &QWebSocket::disconnected, this, &WebUiBridge::onDisconnected);
    connect(m_webSocket.get(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        this, &WebUiBridge::onErrorOccurred);
    connect(m_webSocket.get(), &QWebSocket::textMessageReceived,
            this, &WebUiBridge::onTextMessageReceived);

    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebUiBridge::onReconnectTimerTimeout);
}

void WebUiBridge::connectToServer(const QString& url) {
    m_url = url;
    Logger::instance().infoContext("WebUiBridge", QString("Connecting to %1").arg(url));
    m_webSocket->open(QUrl(url));
}

void WebUiBridge::onConnected() {
    Logger::instance().infoContext("WebUiBridge", "Connected to dash_server");
    m_connected = true;
    emit connectedChanged(true);
}

void WebUiBridge::onDisconnected() {
    Logger::instance().warningContext("WebUiBridge", "Disconnected from dash_server, retrying...");
    m_connected = false;
    emit connectedChanged(false);
    m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
}

void WebUiBridge::onErrorOccurred(QAbstractSocket::SocketError error) {
    Logger::instance().warningContext("WebUiBridge",
        QString("Socket error: %1 (%2)").arg(m_webSocket->errorString()).arg(static_cast<int>(error)));

    m_connected = false;
    emit connectedChanged(false);

    if (!m_reconnectTimer->isActive()) {
        m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
    }
}

void WebUiBridge::onReconnectTimerTimeout() {
    connectToServer(m_url);
}

void WebUiBridge::onTextMessageReceived(const QString& message) {
    Logger::instance().debugContext("WebUiBridge", QString("Received: %1").arg(message));

    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        Logger::instance().warningContext("WebUiBridge", "Received non-object message");
        return;
    }
    handleMessage(doc.object());
}

void WebUiBridge::handleMessage(const QJsonObject& obj) {
    const QString type = obj.value("type").toString();
    const QString action = obj.value("action").toString();

    if (type == QLatin1String("browser") && action == QLatin1String("tabChange")) {
        const QJsonObject payload = obj.value("payload").toObject();
        const QString tab = payload.value("tab").toString();

        const bool shouldShowAA = (tab == QLatin1String("android_auto"));
        if (shouldShowAA != m_aaOverlayVisible) {
            m_aaOverlayVisible = shouldShowAA;
            emit aaOverlayVisibleChanged(m_aaOverlayVisible);
            Logger::instance().infoContext("WebUiBridge",
                QString("aaOverlayVisible -> %1 (tab=%2)").arg(m_aaOverlayVisible).arg(tab));
        }
        return;
    }

    if (type == QLatin1String("browser") && action == QLatin1String("powerDialogActive")) {
        const QJsonObject payload = obj.value("payload").toObject();
        const bool active = payload.value("enabled").toBool();

        if (active != m_powerDialogActive) {
            m_powerDialogActive = active;
            emit powerDialogActiveChanged(m_powerDialogActive);
            Logger::instance().infoContext("WebUiBridge",
                QString("powerDialogActive -> %1").arg(m_powerDialogActive));
        }
        return;
    }

    if (type == QLatin1String("system") && action == QLatin1String("darkMode")) {
        const QJsonObject payload = obj.value("payload").toObject();
        const bool nightMode = payload.value("value").toBool();

        emit nightModeChanged(nightMode);
        Logger::instance().infoContext("WebUiBridge",
            QString("Night mode -> %1 (source=%2)")
                .arg(nightMode ? "on" : "off")
                .arg(payload.value("source").toString()));
        return;
    }

    if (type == QLatin1String("network") || type == QLatin1String("canbus") || type == QLatin1String("cpuTemp")) {
        return;
    }
    
    Logger::instance().debugContext("WebUiBridge",
        QString("Ignoring message type=%1 action=%2").arg(type, action));
}