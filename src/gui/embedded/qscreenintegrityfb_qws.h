/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCREENINTEGRITYFB_QWS_H
#define QSCREENINTEGRITYFB_QWS_H

#include <QtGui/qscreen_qws.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_QWS_INTEGRITYFB

class QIntfbScreenPrivate;

class Q_GUI_EXPORT QIntfbScreen : public QScreen
{
public:
    explicit QIntfbScreen(int display_id);
    virtual ~QIntfbScreen();
    virtual bool initDevice();
    virtual bool connect(const QString &displaySpec);
    virtual void disconnect();
    virtual void shutdownDevice();
    virtual void save();
    virtual void restore();
    virtual void setMode(int nw,int nh,int nd);
    virtual void setDirty(const QRect& r);
    virtual void blank(bool);
    static void setBrightness(int b);

private:
    QIntfbScreenPrivate *d_ptr;
};

#endif // QT_NO_QWS_INTEGRITYFB

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCREENINTEGRITYFB_QWS_H