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

#include "qkbdsylixosinput_qws.h"

#ifndef QT_NO_QWS_KBD_SYLIXOSINPUT

#include "qplatformdefs.h"
#include "qsocketnotifier.h"
#include "private/qcore_unix_p.h"
#include <keyboard.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

class QSylixOSInputKbdPrivate
{
public:
    QSylixOSInputKbdPrivate();
    ~QSylixOSInputKbdPrivate();
    
    QSocketNotifier *kbd_notifier;
    int    kbd_fd;
};

QSylixOSInputKbdPrivate::QSylixOSInputKbdPrivate()
{
    kbd_fd = -1;
    kbd_notifier = 0;
}

QSylixOSInputKbdPrivate::~QSylixOSInputKbdPrivate()
{
}

QSylixOSInputKeyboardHandler::QSylixOSInputKeyboardHandler(const QString &device)
    : QObject(), QWSKeyboardHandler(device), d(new QSylixOSInputKbdPrivate)
{
    d->kbd_fd = QT_OPEN(device.isEmpty() ? "/dev/input/xkbd" : device.toLatin1().constData(),
                        QT_OPEN_RDONLY | O_NONBLOCK);
    if (d->kbd_fd == -1) {
        qErrnoWarning(errno, "QSylixOSInputKeyboardHandler: Unable to open keyboard device");
    } else {
        d->kbd_notifier = new QSocketNotifier(d->kbd_fd, QSocketNotifier::Read, this);
        connect(d->kbd_notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));
        qDebug("QSylixOSInputKeyboardHandler: connected.");
        ioctl(d->kbd_fd, FIOFLUSH, 0); // flush buffered data
    }
}

QSylixOSInputKeyboardHandler::~QSylixOSInputKeyboardHandler()
{
    if (d->kbd_fd != -1) {
        disconnect(d->kbd_notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));
        delete d->kbd_notifier;
        ioctl(d->kbd_fd, FIOFLUSH, 0);
        QT_CLOSE(d->kbd_fd);
        delete d;
    }
}

void QSylixOSInputKeyboardHandler::resume()
{
    if (d->kbd_notifier)
        d->kbd_notifier->setEnabled(true);

    ioctl(d->kbd_fd, FIOFLUSH, 0);
}

void QSylixOSInputKeyboardHandler::suspend()
{
    if (d->kbd_notifier)
        d->kbd_notifier->setEnabled(false);
}

void QSylixOSInputKeyboardHandler::readKeycode()
{
    static bool last_pressed = false;
    static int last_keycode = Qt::Key_unknown, last_unicode = 0;
    static Qt::KeyboardModifiers last_modifiers = Qt::NoModifier;

    keyboard_event_notify buffer[8];
    
    int n = QT_READ(d->kbd_fd, reinterpret_cast<char *>(buffer), sizeof(buffer));
    if (n <= 0) {
        qErrnoWarning(errno, "QSylixOSInputKeyboardHandler: Could not read from input device");
        return;
    } else if (n % sizeof(buffer[0]) != 0) {
        qErrnoWarning(errno, "QSylixOSInputKeyboardHandler: Internal error");
        return;
    }
    n /= sizeof(buffer[0]);
    
    for (int i = 0; i < n; i++) {
        const keyboard_event_notify *notify = &buffer[i];
        
        bool pressed;
        bool repeat;
        bool tolower;
        int keycode = Qt::Key_unknown;
        int unicode = 0;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        
        if (notify->fkstat & (KE_FK_CTRL | KE_FK_CTRLR)) {
            modifiers |= Qt::ControlModifier;
        }
        if (notify->fkstat & (KE_FK_ALT | KE_FK_ALTR)) {
            modifiers |= Qt::AltModifier;
        }
        if (notify->fkstat & (KE_FK_SHIFT | KE_FK_SHIFTR)) {
            modifiers |= Qt::ShiftModifier;
        }
        
        if (notify->type == KE_PRESS) {
            pressed = 1;
        } else {
            pressed = 0;
        }

        unicode = notify->keymsg[0];
        
        if (unicode < 0xff) {
            // modify unicode to correct stat.
            if (notify->fkstat & (KE_FK_SHIFT | KE_FK_SHIFTR)) {
                if (notify->ledstate & KE_LED_CAPSLOCK) {
                    tolower = true;
                } else {
                    tolower = false;
                }
            } else {
                if (notify->ledstate & KE_LED_CAPSLOCK) {
                    tolower = false;
                } else {
                    tolower = true;
                }
            }
            if (tolower) {
                if (unicode >= 'A' && unicode <= 'Z') {
                    unicode = 'a' + unicode - 'A';
                }
            } else {
                if (unicode >= 'a' && unicode <= 'z') {
                    unicode = 'A' + unicode - 'a';
                }
            }
            
            // change unicode to keycode
            if (unicode >= 'A' && unicode <= 'Z') {
                keycode = Qt::Key_A + unicode - 'A';
            } else if (unicode >= 'a' && unicode <= 'z') {
                keycode = Qt::Key_A + unicode - 'a';
            } else {
                keycode = unicode;
            }

            // some driver process <esc>, <tab>, <backspace> as ascii
            if (unicode == 27) {
                keycode = Qt::Key_Escape;
            } else if (unicode == 9) {
                keycode = Qt::Key_Tab;
            } else if (unicode == 8) { // ascii 8 is <backspace>
                keycode = Qt::Key_Backspace;
            } else if (unicode == 13) { // unix like enter
                keycode = Qt::Key_Enter;
            }

            // Ctrl<something> or Alt<something> is not a displayable character
            if (modifiers & (Qt::ControlModifier | Qt::AltModifier)) {
                unicode = 0;
            }
        
        } else {
            unicode = 0;
            
            switch (notify->keymsg[0]) {
            
            case K_F1:     keycode = Qt::Key_F1;     break;
            case K_F2:     keycode = Qt::Key_F2;     break;
            case K_F3:     keycode = Qt::Key_F3;     break;
            case K_F4:     keycode = Qt::Key_F4;     break;
            case K_F5:     keycode = Qt::Key_F5;     break;
            case K_F6:     keycode = Qt::Key_F6;     break;
            case K_F7:     keycode = Qt::Key_F7;     break;
            case K_F8:     keycode = Qt::Key_F8;     break;
            case K_F9:     keycode = Qt::Key_F9;     break;
            case K_F10:    keycode = Qt::Key_F10;    break;
            case K_F11:    keycode = Qt::Key_F11;    break;
            case K_F12:    keycode = Qt::Key_F12;    break;
            case K_F13:    keycode = Qt::Key_F13;    break;
            case K_F14:    keycode = Qt::Key_F14;    break;
            case K_F15:    keycode = Qt::Key_F15;    break;
            case K_F16:    keycode = Qt::Key_F16;    break;
            case K_F17:    keycode = Qt::Key_F17;    break;
            case K_F18:    keycode = Qt::Key_F18;    break;
            case K_F19:    keycode = Qt::Key_F19;    break;
            case K_F20:    keycode = Qt::Key_F20;    break;
            
            case K_ESC:          keycode = Qt::Key_Escape;      unicode = 27;    break; // ascii 27 is <esc>
            case K_TAB:          keycode = Qt::Key_Tab;         unicode =  9;    break; // ascii 8 is <tab>
            case K_CAPSLOCK:     keycode = Qt::Key_CapsLock;    break;
            case K_SHIFT:        keycode = Qt::Key_Shift;       break;
            case K_CTRL:         keycode = Qt::Key_Control;     break;
            case K_ALT:          keycode = Qt::Key_Alt;         break;
            case K_SHIFTR:       keycode = Qt::Key_Shift;       break;
            case K_CTRLR:        keycode = Qt::Key_Control;     break;
            case K_ALTR:         keycode = Qt::Key_Alt;         break;
            case K_ENTER:        keycode = Qt::Key_Enter;       break;
            case K_BACKSPACE:    keycode = Qt::Key_Backspace;   unicode =  8;    break; // ascii 8 is <backspace>
            
            case K_PRINTSCREEN:  keycode = Qt::Key_Print;       break;
            case K_SCROLLLOCK:   keycode = Qt::Key_ScrollLock;  break;
            case K_PAUSEBREAK:   keycode = Qt::Key_Pause;       break;
            
            case K_INSERT:       keycode = Qt::Key_Insert;      break;
            case K_HOME:         keycode = Qt::Key_Home;        break;
            case K_DELETE:       keycode = Qt::Key_Delete;      break;
            case K_END:          keycode = Qt::Key_End;         break;
            case K_PAGEUP:       keycode = Qt::Key_PageUp;      break;
            case K_PAGEDOWN:     keycode = Qt::Key_PageDown;    break;
            
            case K_UP:           keycode = Qt::Key_Up;          break;
            case K_DOWN:         keycode = Qt::Key_Down;        break;
            case K_LEFT:         keycode = Qt::Key_Left;        break;
            case K_RIGHT:        keycode = Qt::Key_Right;       break;
            
            default: break;
            }
        }
        
        if (keycode == Qt::Key_unknown && unicode == 0) {
            continue;
        }

        if ((keycode == last_keycode) && (unicode == last_unicode) && (modifiers == last_modifiers) && (pressed == last_pressed)) {
            repeat = true;
        } else {
            repeat = false;
            last_keycode = keycode;
            last_unicode = unicode;
            last_modifiers = modifiers;
            last_pressed = pressed;
        }
        
        processKeyEvent(unicode, keycode, modifiers, pressed, repeat);
    }
}

QT_END_NAMESPACE

#endif // !QT_NO_QWS_KBD_SYLIXOSINPUT
