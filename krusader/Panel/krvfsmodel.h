/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This package is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this package; if not, write to the Free Software               *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#ifndef KRVFSMODEL_H
#define KRVFSMODEL_H

// QtCore
#include <QAbstractListModel>
// QtGui
#include <QFont>

#include "krinterview.h"
#include "krsort.h"


class FileItem;
class KrViewProperties;

class KrVfsModel: public QAbstractListModel
{
    Q_OBJECT

public:
    KrVfsModel(KrInterView *);
    virtual ~KrVfsModel();

    inline bool ready() const {
        return _ready;
    }
    void populate(const QList<FileItem*> &files, FileItem *dummy);
    QModelIndex addItem(FileItem *);
    QModelIndex removeItem(FileItem *);
    void updateItem(FileItem *fileitem);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const  Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const  Q_DECL_OVERRIDE;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole)  Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const  Q_DECL_OVERRIDE;
    void setExtensionEnabled(bool exten) {
        _extensionEnabled = exten;
    }
    inline const KrViewProperties * properties() const {
        return _view->properties();
    }
    void sort() {
        sort(lastSortOrder(), lastSortDir());
    }
    void clear();
    QList<FileItem*> fileItems() {
        return _fileItems;
    }
    FileItem * fileItemAt(const QModelIndex &index);
    FileItem *dummyFileItem() const {
        return _dummyFileItem;
    }
    const QModelIndex & fileItemIndex(const FileItem *);
    const QModelIndex & nameIndex(const QString &);
    const QModelIndex & indexFromUrl(const QUrl &url);
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    void emitChanged() {
        emit layoutChanged();
    }
    Qt::SortOrder lastSortDir() const {
        return (properties()->sortOptions & KrViewProperties::Descending) ? Qt::DescendingOrder : Qt::AscendingOrder;
    }
    int lastSortOrder() const {
        return properties()->sortColumn;
    }
    void setAlternatingTable(bool altTable) {
        _alternatingTable = altTable;
    }

public slots:
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) Q_DECL_OVERRIDE;

protected:
    KrSort::LessThanFunc lessThanFunc() {
        return KrSort::itemLessThan;
    }
    KrSort::LessThanFunc greaterThanFunc() const {
        return KrSort::itemGreaterThan;
    }
    QVariant customSortData(FileItem *) const {
        return QVariant();
    }
    KrSort::Sorter createSorter();
    QString nameWithoutExtension(const FileItem * fileitem, bool checkEnabled = true) const;

private:
    void updateIndices(FileItem *file, int index);

    QList<FileItem*>               _fileItems;
    QHash<FileItem *, QModelIndex> _fileItemNdx;
    QHash<QString, QModelIndex> _nameNdx;
    QHash<QUrl, QModelIndex>    _urlNdx;
    bool                        _extensionEnabled;
    KrInterView                 * _view;
    FileItem *                     _dummyFileItem;
    bool                        _ready;
    QFont                       _defaultFont;
    bool                        _justForSizeHint;
    bool                        _alternatingTable;
};

#endif // __krvfsmodel__
