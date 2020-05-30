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

#ifndef KRDEBUGLOGGER_H
#define KRDEBUGLOGGER_H

// QtCore
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>

#include <unistd.h>

//! Manages a system aimed to show debug messages
class KrDebugLogger
{
public:
    explicit KrDebugLogger();
    ~KrDebugLogger() = default;

    //! Builds a QString that contains the corresponding indentation and other information that has to be written
    /*!
        \param argFunction   The name of the function where this method is called.
        \param line          The line where this method is called.
        \param fnStartOrEnd  In the case of a function: This QString is used to indicate the user if the function is starting or ending.
        \return The corresponding indentation (a group of spaces) and other information that has to be written in the same line.
    */
    QString indentationEtc(const QString &argFunction, int line = 0, const QString &fnStartOrEnd = "") const;
    //! Decreases the indentation that is going to be used next time
    void decreaseIndentation();
    //! Increases the indentation that is going to be used next time
    void increaseIndentation();
private:
    //! The indentation that is presently used, it represents how many spaces are going to be used to indent
    int indentation = 1;
    //! The quantity of spaces that are going be added to the indentation when increasing it
    const int indentationIncrease = 4;
    //! Indicates if debug messages are going to be less detailed, which will be useful e.g. when comparing traces
    bool briefMode = false;
};

// ---------------------------------------------------------------------------------------

//! A class to manage the automatic indentation of debug messages, and their writing when a function starts or ends
class KrDebugFnLogger
{
public:
    //! This constructor is used inside the KRFUNC macro. For more details: the description of the KRFUNC macro can be seen
    explicit KrDebugFnLogger(const QString &argFunction, int line, KrDebugLogger &argKrDebugLogger);
    //! This desstructor is used inside the KRFUNC macro. For more details: the description of the KRFUNC macro can be seen
    ~KrDebugFnLogger();

private:
    //! The name of a function which is going to be written about
    QString function;
    //! The KrDebugLogger that manages aspects that are common to KrDebugFnLogger objects
    KrDebugLogger &krDebugLogger;
};

// ---------------------------------------------------------------------------------------

//! An object that manages debug messages in a convenient way
extern KrDebugLogger krDebugLogger;

//! Writes a function name, etc. when entering the function and automatically before exiting from it
#define KRFUNC \
    KrDebugFnLogger functionLogger(__FUNCTION__, __LINE__, krDebugLogger);

#define KRDEBUG(X...) \
    qDebug().nospace().noquote() << krDebugLogger.indentationEtc(__FUNCTION__, __LINE__) << ": " << X;

#endif // KRDEBUGLOGGER_H

