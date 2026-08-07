#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

class AndroidAutoImageProvider : public QQuickImageProvider
{
    Q_OBJECT
    
public:
    AndroidAutoImageProvider();

    QImage requestImage(
        const QString &id,
        QSize *size,
        const QSize &requestedSize) override;

    void setFrame(const QImage &image);
    void clearFrame();

private:
    QImage m_frame;
    QMutex m_mutex;

signals:
    void frameUpdated();
};