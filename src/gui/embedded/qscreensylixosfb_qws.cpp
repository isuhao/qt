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

#ifndef QT_NO_QWS_SYLIXOSFB

#include "qscreensylixosfb_qws.h"

#include <qwindowsystem_qws.h>
#include <qsocketnotifier.h>
#include <qapplication.h>
#include <qscreen_qws.h>
#include <qmousedriverfactory_qws.h>
#include <qkbddriverfactory_qws.h>
#include <qdebug.h>
#include <sys/mman.h>

QT_BEGIN_NAMESPACE

class QSylixOSFbScreenPrivate
{
public:
    QSylixOSFbScreenPrivate();
    ~QSylixOSFbScreenPrivate();

    int fd;
    LW_GM_VARINFO var;
    LW_GM_SCRINFO fix;
};

QSylixOSFbScreenPrivate::QSylixOSFbScreenPrivate()
{
}

QSylixOSFbScreenPrivate::~QSylixOSFbScreenPrivate()
{
}

QSylixOSFbScreen::QSylixOSFbScreen(int display_id)
    : QScreen(display_id, SylixOSFbClass), d_ptr(new QSylixOSFbScreenPrivate)
{
    d_ptr->fd = -1;
    data = 0;
}

QSylixOSFbScreen::~QSylixOSFbScreen()
{
    delete d_ptr;
}

bool QSylixOSFbScreen::connect(const QString & displaySpec)
{
    const QStringList args = displaySpec.split(QLatin1Char(':'));
   
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#ifndef QT_QWS_FRAMEBUFFER_LITTLE_ENDIAN
    if (args.contains(QLatin1String("littleendian")))
#endif
        QScreen::setFrameBufferLittleEndian(true);
#endif

    QString dev = QLatin1String("/dev/fb0");
    foreach(QString d, args) {
        if (d.startsWith(QLatin1Char('/'))) {
            dev = d;
            break;
        }
    }

    d_ptr->fd = open(dev.toLatin1().constData(), O_RDWR, 0666);
    if (d_ptr->fd < 0) {
        qErrnoWarning(errno, "QSylixOSFbScreen: Unable to open framebuffer device");
        return false;
    }

    if (ioctl(d_ptr->fd, LW_GM_GET_VARINFO, &d_ptr->var) < 0) {
        qErrnoWarning(errno, "QSylixOSFbScreen: Unable to get variable screen information");
        close(d_ptr->fd);
        d_ptr->fd = -1;
        return false;
    }

    if (ioctl(d_ptr->fd, LW_GM_GET_SCRINFO, &d_ptr->fix) < 0) {
        qErrnoWarning(errno, "QSylixOSFbScreen: Unable to get fixed screen information");
        close(d_ptr->fd);
        d_ptr->fd = -1;
        return false;
    }

    mapsize = size = d_ptr->fix.GMSI_stMemSize;

    data = (uchar *)mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED, d_ptr->fd, 0);
    if ((long)data == -1) {
        qErrnoWarning(errno, "QSylixOSFbScreen: Unable to map framebuffer device");
        close(d_ptr->fd);
        d_ptr->fd = -1;
        return false;
    }

    lstep  = d_ptr->fix.GMSI_stMemSizePerLine;

    dw = w = d_ptr->var.GMVI_ulXRes;
    dh = h = d_ptr->var.GMVI_ulYRes;

    // default values
    int dpi = 72; // Dot Per Inch

    physWidth  = qRound(dw * 25.4 / dpi);
    physHeight = qRound(dh * 25.4 / dpi);

    d = d_ptr->var.GMVI_ulBitsPerPixel;
    switch (d) {
    case 1:
        setPixelFormat(QImage::Format_Mono);
        break;
    case 8:
        setPixelFormat(QImage::Format_Indexed8);
        break;
    case 12:
        setPixelFormat(QImage::Format_RGB444);
        break;
    case 15:
        setPixelFormat(QImage::Format_RGB555);
        break;
    case 16:
        setPixelFormat(QImage::Format_RGB16);
        break;
    case 18:
        setPixelFormat(QImage::Format_RGB666);
        break;
    case 24:
        setPixelFormat(QImage::Format_RGB888);
#ifdef QT_QWS_DEPTH_GENERIC
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
            qt_set_generic_blit(this, 24,
                    d_ptr->var.GMVI_gmbfRed.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfGreen.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfBlue.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfTrans.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfRed.GMBF_uiOffset,
                    d_ptr->var.GMVI_gmbfGreen.GMBF_uiOffset,
                    d_ptr->var.GMVI_gmbfBlue.GMBF_uiOffset,
                    d_ptr->var.GMVI_gmbfTrans.GMBF_uiOffset);
#else
            qt_set_generic_blit(this, 24,
                    d_ptr->var.GMVI_gmbfRed.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfGreen.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfBlue.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfTrans.GMBF_uiLength,
                    16 - d_ptr->var.GMVI_gmbfRed.GMBF_uiOffset,
                    16 - d_ptr->var.GMVI_gmbfGreen.GMBF_uiOffset,
                    16 - d_ptr->var.GMVI_gmbfBlue.GMBF_uiOffset,
                    16 - d_ptr->var.GMVI_gmbfTrans.GMBF_uiOffset);
#endif
#endif
        break;
    case 32:
        setPixelFormat(QImage::Format_ARGB32_Premultiplied);
#ifdef QT_QWS_DEPTH_GENERIC
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
            qt_set_generic_blit(this, 32,
                    d_ptr->var.GMVI_gmbfRed.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfGreen.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfBlue.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfTrans.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfRed.GMBF_uiOffset,
                    d_ptr->var.GMVI_gmbfGreen.GMBF_uiOffset,
                    d_ptr->var.GMVI_gmbfBlue.GMBF_uiOffset,
                    d_ptr->var.GMVI_gmbfTrans.GMBF_uiOffset);
#else
            qt_set_generic_blit(this, 32,
                    d_ptr->var.GMVI_gmbfRed.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfGreen.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfBlue.GMBF_uiLength,
                    d_ptr->var.GMVI_gmbfTrans.GMBF_uiLength,
                    24 - d_ptr->var.GMVI_gmbfRed.GMBF_uiOffset,
                    24 - d_ptr->var.GMVI_gmbfGreen.GMBF_uiOffset,
                    24 - d_ptr->var.GMVI_gmbfBlue.GMBF_uiOffset,
                    24 - d_ptr->var.GMVI_gmbfTrans.GMBF_uiOffset);
#endif
#endif
        break;
    }

    qDebug("Connected to SylixOS FrameBuffer server: %d x %d x %d %dx%dmm (%dx%ddpi)",
        w, h, d, physWidth, physHeight, qRound(dw * 25.4 / physWidth), qRound(dh * 25.4 / physHeight) );

    QWSServer::setDefaultMouse("sylixosinput");
    QWSServer::setDefaultKeyboard("sylixosinput");

    return true;
}

void QSylixOSFbScreen::disconnect()
{
    if (data) {
        munmap((char*)data, mapsize);
        data = 0;
    }

    if (d_ptr->fd != -1) {
        close(d_ptr->fd);
        d_ptr->fd = -1;
    }
}

bool QSylixOSFbScreen::initDevice()
{
#ifndef QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    return true;
}

void QSylixOSFbScreen::shutdownDevice()
{

}

void QSylixOSFbScreen::setMode(int nw, int nh, int nd)
{

}

void QSylixOSFbScreen::exposeRegion(QRegion r, int changing)
{
    // here is where the actual magic happens. QWS will call exposeRegion whenever
    // a region on the screen is dirty and needs to be updated on the actual screen.

    // first, call the parent implementation. The parent implementation will update
    // the region on our in-memory surface
    QScreen::exposeRegion(r, changing);
}

// save the state of the graphics card
void QSylixOSFbScreen::save()
{

}

// restore the state of the graphics card.
void QSylixOSFbScreen::restore()
{

}

void QSylixOSFbScreen::setDirty(const QRect & rect)
{

}

void QSylixOSFbScreen::setBrightness(int b)
{

}

void QSylixOSFbScreen::blank(bool on)
{

}

#endif // QT_NO_QWS_SYLIXOSFB

QT_END_NAMESPACE
