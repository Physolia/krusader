/*
    SPDX-FileCopyrightText: 2002 Shie Erlich <erlich@users.sourceforge.net>
    SPDX-FileCopyrightText: 2002 Rafi Yanai <yanai@users.sourceforge.net>
    SPDX-FileCopyrightText: 2004-2022 Krusader Krew <https://krusader.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "krsqueezedtextlabel.h"

// QtGui
#include <QDragEnterEvent>
#include <QPainter>
// QtWidgets
#include <QToolTip>
#include <QLabel>

#include <KCoreAddons/KUrlMimeData>
#include <KCoreAddons/KStringHandler>

#include "../compat.h"

KrSqueezedTextLabel::KrSqueezedTextLabel(QWidget *parent):
        KSqueezedTextLabel(parent), _index(-1), _length(-1)
{
    setAutoFillBackground(true);
}


KrSqueezedTextLabel::~KrSqueezedTextLabel()
= default;

void KrSqueezedTextLabel::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
    emit clicked(e);
}

void KrSqueezedTextLabel::squeezeTextToLabel(int index, int length)
{
    if (index == -1 || length == -1)
        KSqueezedTextLabel::squeezeTextToLabel();
    else {
        QString sqtext = fullText;
        QFontMetrics fm(fontMetrics());
        int labelWidth = size().width();
        int textWidth = fm.horizontalAdvance(sqtext);
        if (textWidth > labelWidth) {
            int avgCharSize = textWidth / sqtext.length();
            int numOfExtraChars = (textWidth - labelWidth) / avgCharSize;
            int delta;

            // remove as much as possible from the left, and then from the right
            if (index > 3) {
                delta = qMin(index, numOfExtraChars);
                numOfExtraChars -= delta;
                sqtext.replace(0, delta, "...");
            }

            if (numOfExtraChars > 0 && ((int)sqtext.length() > length + 3)) {
                delta = qMin(numOfExtraChars, (int)sqtext.length() - (length + 3));
                sqtext.replace(sqtext.length() - delta, delta, "...");
            }
            QLabel::setText(sqtext);

            setToolTip(QString());
            setToolTip(fullText);
        } else {
            QLabel::setText(fullText);

            setToolTip(QString());
            QToolTip::hideText();
        }
    }
}

void KrSqueezedTextLabel::setText(const QString &text, int index, int length)
{
    _index = index;
    _length = length;
    fullText = text;
    KSqueezedTextLabel::setText(fullText);
    squeezeTextToLabel(_index, _length);
}

void KrSqueezedTextLabel::paintEvent(QPaintEvent * e)
{
    KSqueezedTextLabel::paintEvent(e);
}


