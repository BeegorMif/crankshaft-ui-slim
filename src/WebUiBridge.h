#pragma once

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <memory>

class WebUiBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool aaOverlayVisible READ aaOverlayVisible NOTIFY aaOverlayVisibleChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

public:
    explicit WebUiBridge(QObject* parent = nullptr);

    [[nodiscard]] auto aaOverlayVisible() const -> bool { return m_aaOverlayVisible; }
    [[nodiscard]] auto connected() const -> bool { return m_connected; }

    Q_INVOKABLE void connectToServer(const QString& url = QStringLiteral("ws://localhost:3001"));

signals:
    void aaOverlayVisibleChanged(bool visible);
    void connectedChanged(bool connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);
    void onReconnectTimerTimeout();

private:
    void handleMessage(const QJsonObject& obj);

    std::unique_ptr<QWebSocket> m_webSocket;
    QTimer* m_reconnectTimer;
    QString m_url;
    bool m_aaOverlayVisible = true;
    bool m_connected = false;

    static constexpr int RECONNECT_INTERVAL_MS = 3000;
};