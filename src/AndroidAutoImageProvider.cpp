#include "AndroidAutoImageProvider.h"

#include <QMutexLocker>

AndroidAutoImageProvider::AndroidAutoImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage AndroidAutoImageProvider::requestImage(
    const QString &id,
    QSize *size,
    const QSize &requestedSize)
{
    QMutexLocker locker(&m_mutex);

    if (size) {
        *size = m_frame.size();
    }

    if (m_frame.isNull()) {
        QImage empty(
            requestedSize.isValid() ?
                requestedSize :
                QSize(1280,720),
            QImage::Format_RGB32
        );

        empty.fill(Qt::black);

        return empty;
    }

    return m_frame;
}

void AndroidAutoImageProvider::setFrame(const QImage &image)
{
    {
        QMutexLocker locker(&m_mutex);
        m_frame = image.copy();
    }

    emit frameUpdated();
}

void AndroidAutoImageProvider::clearFrame()
{
    QMutexLocker locker(&m_mutex);
    m_frame = QImage();
}

