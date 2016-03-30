/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmousesylixosinput_qws.h"

#ifndef QT_NO_QWS_MOUSE_SYLIXOSINPUT

#include "qplatformdefs.h"
#include "qsocketnotifier.h"
#include "private/qcore_unix_p.h"
#include <mouse.h>
#include <errno.h>
#include "qtextstream.h"
#include "qfile.h"

QT_BEGIN_NAMESPACE

class QSylixOSInputMousePrivate
{
public:
    QSylixOSInputMousePrivate();
    ~QSylixOSInputMousePrivate();
    
    QSocketNotifier *mouse_notifier;
    int mouse_fd;
    int calibrated;
};

QSylixOSInputMousePrivate::QSylixOSInputMousePrivate()
{
    mouse_fd = -1;
    calibrated = 0;
    mouse_notifier = 0;
}

QSylixOSInputMousePrivate::~QSylixOSInputMousePrivate()
{
}

QSylixOSInputMouseHandler::QSylixOSInputMouseHandler(const QString &driver, const QString &device)
    : QObject(), QWSCalibratedMouseHandler(driver, device), d(new QSylixOSInputMousePrivate)
{
    d->mouse_fd = QT_OPEN(device.isEmpty() ? "/dev/input/xmse" : device.toLatin1().constData(),
                      QT_OPEN_RDONLY | O_NONBLOCK);
    if (d->mouse_fd == -1) {
        qErrnoWarning(errno, "QSylixOSInputMouseHandler: Unable to open mouse device");
    } else {
        d->mouse_notifier = new QSocketNotifier(d->mouse_fd, QSocketNotifier::Read, this);
        connect(d->mouse_notifier, SIGNAL(activated(int)), this, SLOT(socketActivated()));
        QString calFile = QString::fromLocal8Bit(qgetenv("POINTERCAL_FILE"));
        if (calFile.isEmpty())
           calFile = QLatin1String("/etc/pointercal");

        QFile file(calFile);
        if (file.open(QIODevice::ReadOnly)) {
           d->calibrated = true;
        } else {
           d->calibrated = false;
        }
        qDebug("QSylixOSInputMouseHandler: connected.");
        ioctl(d->mouse_fd, FIOFLUSH, 0); // flush buffered data
    }
}

QSylixOSInputMouseHandler::~QSylixOSInputMouseHandler()
{
    if (d->mouse_fd != -1) {
        disconnect(d->mouse_notifier, SIGNAL(activated(int)), this, SLOT(socketActivated()));
        delete d->mouse_notifier;
        ioctl(d->mouse_fd, FIOFLUSH, 0);
        QT_CLOSE(d->mouse_fd);
        delete d;
    }
}

void QSylixOSInputMouseHandler::resume()
{
    if (d->mouse_notifier)
        d->mouse_notifier->setEnabled(true);

    ioctl(d->mouse_fd, FIOFLUSH, 0);
}

void QSylixOSInputMouseHandler::suspend()
{
    if (d->mouse_notifier)
        d->mouse_notifier->setEnabled(false);
}

void QSylixOSInputMouseHandler::socketActivated()
{
    QPoint queuedPos = mousePos;

    mouse_event_notify buffer[32];

    int n = QT_READ(d->mouse_fd, reinterpret_cast<char *>(buffer), sizeof(buffer));
    if (n <= 0) {
        qErrnoWarning(errno, "QSylixOSInputMouseHandler: Could not read from input device");
        return;
    } else if (n % sizeof(buffer[0]) != 0) {
        qErrnoWarning(errno, "QSylixOSInputMouseHandler: Internal error");
        return;
    }
    n /= sizeof(buffer[0]);

    for (int i = 0; i < n; i++) {
        const mouse_event_notify *notify = &buffer[i];

        int buttons = Qt::NoButton;
        if (notify->kstat & MOUSE_LEFT)
            buttons |= Qt::LeftButton;
        if (notify->kstat & MOUSE_MIDDLE)
            buttons |= Qt::MidButton;
        if (notify->kstat & MOUSE_RIGHT)
            buttons |= Qt::RightButton;

        if (notify->ctype == MOUSE_CTYPE_ABS) {
            if (d->calibrated) {
                QPoint pos;
                pos = transform(QPoint(notify->xanalog, notify->yanalog));
                limitToScreen(pos);
                mouseChanged(pos, buttons, 0);
                queuedPos = pos;
            } else {
                qDebug("QSylixOSInputMouseHandler: send (%d, %d) to filter", notify->xanalog, notify->yanalog);
                       sendFiltered(QPoint(notify->xanalog, notify->yanalog), buttons);
            }
        } else if (notify->ctype == MOUSE_CTYPE_REL) {
            queuedPos += QPoint(notify->xanalog, notify->yanalog);
            limitToScreen(queuedPos);
            mouseChanged(queuedPos, buttons, notify->wscroll[0] << 7 /* same as *128 */ );
        } else {
            qWarning("QSylixOSInputMouseHandler: unknown mouse event type=%x", notify->ctype);
        }
    }

    mousePos = queuedPos;
}

void QSylixOSInputMouseHandler::clearCalibration()
{
    QWSCalibratedMouseHandler::clearCalibration();
}

void QSylixOSInputMouseHandler::calibrate(const QWSPointerCalibrationData *data)
{
    QWSCalibratedMouseHandler::calibrate(data);
}

QT_END_NAMESPACE

#endif // !QT_NO_QWS_MOUSE_SYLIXOSINPUT
