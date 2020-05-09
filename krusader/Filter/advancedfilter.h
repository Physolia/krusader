/*****************************************************************************
 * Copyright (C) 2003 Shie Erlich <krusader@users.sourceforge.net>           *
 * Copyright (C) 2003 Rafi Yanai <krusader@users.sourceforge.net>            *
 * Copyright (C) 2003 Csaba Karai <krusader@users.sourceforge.net>           *
 * Copyright (C) 2004-2020 Krusader Krew [https://krusader.org]              *
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

#ifndef ADVANCEDFILTER_H
#define ADVANCEDFILTER_H

#include "filterbase.h"

// QtWidgets
#include <QWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolButton>

#include <KCompletion/KComboBox>
#include <KCompletion/KLineEdit>

class QSpinBox;

class AdvancedFilter : public QWidget, public FilterBase
{
    Q_OBJECT

public:
    explicit AdvancedFilter(FilterTabs *tabs, QWidget *parent = nullptr);

    void          queryAccepted() override {}
    QString       name() override {
        return "AdvancedFilter";
    }
    FilterTabs *  filterTabs() override {
        return fltTabs;
    }
    bool getSettings(FilterSettings&) override;
    void applySettings(const FilterSettings&) override;

public slots:
    void modifiedBetweenSetDate1();
    void modifiedBetweenSetDate2();
    void notModifiedAfterSetDate();

public:
    QCheckBox* minSizeEnabled;
    QSpinBox* minSizeAmount;
    KComboBox* minSizeType;

    QCheckBox* maxSizeEnabled;
    QSpinBox* maxSizeAmount;
    KComboBox* maxSizeType;

    QRadioButton* anyDateEnabled;
    QRadioButton* modifiedBetweenEnabled;
    QRadioButton* notModifiedAfterEnabled;
    QRadioButton* modifiedInTheLastEnabled;

    KLineEdit* modifiedBetweenData1;
    KLineEdit* modifiedBetweenData2;

    QToolButton* modifiedBetweenBtn1;
    QToolButton* modifiedBetweenBtn2;
    QToolButton* notModifiedAfterBtn;

    KLineEdit* notModifiedAfterData;
    QSpinBox* modifiedInTheLastData;
    QSpinBox* notModifiedInTheLastData;
    KComboBox* modifiedInTheLastType;
    KComboBox* notModifiedInTheLastType;

    QCheckBox* belongsToUserEnabled;
    KComboBox* belongsToUserData;
    QCheckBox* belongsToGroupEnabled;
    KComboBox* belongsToGroupData;

    QCheckBox* permissionsEnabled;

    KComboBox* ownerW;
    KComboBox* ownerR;
    KComboBox* ownerX;
    KComboBox* groupW;
    KComboBox* groupR;
    KComboBox* groupX;
    KComboBox* allW;
    KComboBox* allX;
    KComboBox* allR;

    FilterTabs *fltTabs;

private:
    void changeDate(KLineEdit *p);
    void fillList(KComboBox *list, const QString& filename);
    void invalidDateMessage(KLineEdit *p);
    static QDate stringToDate(const QString& text) {
        // 30.12.16 is interpreted as 1916-12-30
        return QLocale().toDate(text, QLocale::ShortFormat).addYears(100);
    }
    static QString dateToString(const QDate& date) {
        return QLocale().toString(date, QLocale::ShortFormat);
    }
};

#endif /* ADVANCEDFILTER_H */
