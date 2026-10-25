
/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * TOra - An Oracle Toolkit for DBA's and developers
 *
 * Shared/mixed copyright is held throughout files in this product
 *
 * Portions Copyright (C) 2000-2001 Underscore AB
 * Portions Copyright (C) 2003-2005 Quest Software, Inc.
 * Portions Copyright (C) 2004-2013 Numerous Other Contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;  only version 2 of
 * the License is valid for this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program as the file COPYING.txt; if not, please see
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *      As a special exception, you have permission to link this program
 *      with the Oracle Client libraries and distribute executables, as long
 *      as you follow the requirements of the GNU GPL in regard to all of the
 *      software in the executable aside from Oracle client libraries.
 *
 * All trademarks belong to their respective owners.
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef TOSCHEMABROWSERLITE_H
#define TOSCHEMABROWSERLITE_H

#include "core/todocklet.h"

class QComboBox;
class QLineEdit;
class QListView;
class QToolButton;
class QStringListModel;
class QSortFilterProxyModel;
class QModelIndex;
class toEditWidget;
class toConnection;

//! \brief Lightweight always-visible tables/views browser, docked alongside
//! Connections/Directory/Outline/QueryModel. Unlike the full Schema Browser
//! tool, this reads only the already-cached in-memory schema metadata (no
//! DB queries of its own) and is meant purely for quickly finding/inserting
//! an object name while editing a worksheet.
class toSchemaBrowserLite : public toDocklet
{
        Q_OBJECT;

    public:
        toSchemaBrowserLite(QWidget *parent = 0,
                            toWFlags flags = 0);

        virtual QIcon icon() const;
        virtual QString name() const;

    public slots:
        void refreshSchemaList();
        void refreshObjectList();
        void handleFilterChanged(QString const& text);
        void handleActivated(const QModelIndex &index);
        void handleFocusChanged(QWidget *old, QWidget *now);

    private:
        QComboBox              *SchemaCombo;
        QLineEdit               *FilterEdit;
        QToolButton             *RefreshButton;
        QListView               *ListView;
        QStringListModel         *Model;
        QSortFilterProxyModel    *ProxyModel;

        //! \brief Last worksheet editor seen gaining focus - tracked locally
        //! since there's no global "current editor" accessor (same pattern
        //! as toCodeOutline), so a click into this docklet's own widgets
        //! doesn't lose track of where to insert on double-click.
        toEditWidget *LastEditor;
};

#endif
