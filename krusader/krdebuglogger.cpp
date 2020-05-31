/*****************************************************************************
 * Copyright (C) 2016 Rafi Yanai <krusader@users.sf.net>                     *
 * Copyright (C) 2016 Shie Erlich <krusader@users.sf.net>                    *
 * Copyright (C) 2016-2020 Krusader Krew [https://krusader.org]              *
 *                                                                           *
 * This file is part of Krusader [https://krusader.org].                     *
 *                                                                           *
 * Krusader is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * Krusader is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with Krusader.  If not, see [http://www.gnu.org/licenses/].         *
 *****************************************************************************/

#include "krdebuglogger.h"

#include <QStringBuilder>

KrDebugLogger krDebugLogger;

KrDebugLogger::KrDebugLogger()
{
    // Sets the level of detail/verbosity
    const QByteArray krDebugBrief = qgetenv("KRDEBUG_BRIEF").toLower();
    briefMode = (krDebugBrief == "true" || krDebugBrief == "yes" || krDebugBrief == "on" || krDebugBrief == "1");
}

QString KrDebugLogger::indentationEtc(const QString &argFunction, int line, const QString &fnStartOrEnd) const
{
    QString result = QString(indentation - 1, ' ') %  // Applies the indentation level to make logs clearer
                     fnStartOrEnd % argFunction;  // Uses QStringBuilder to concatenate
    if (!briefMode)
        result = QString("Pid:%1 ").arg(getpid()) %
                result %
                (line != 0 ? QString("(%1)").arg(line) : "");
    return result;
}

void KrDebugLogger::decreaseIndentation()
{
    indentation -= indentationIncrease;
}

void KrDebugLogger::increaseIndentation()
{
    indentation += indentationIncrease;
}

// ---------------------------------------------------------------------------------------
// Member functions of the KrDebugFnLogger class
// ---------------------------------------------------------------------------------------

KrDebugFnLogger::KrDebugFnLogger(const QString &argFunction, int line, KrDebugLogger &argKrDebugLogger) :
    functionName(argFunction), krDebugLogger(argKrDebugLogger)
{
    // Shows that a function has been started
    qDebug().nospace().noquote() << krDebugLogger.indentationEtc(functionName, line, "┏");

    krDebugLogger.increaseIndentation();
}

KrDebugFnLogger::~KrDebugFnLogger()
{
    krDebugLogger.decreaseIndentation();
    // Shows that a function is going to finish
    qDebug().nospace().noquote() << krDebugLogger.indentationEtc(functionName, 0, "┗");
}
