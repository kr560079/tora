
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

#ifndef TOQUERYHISTORY_H
#define TOQUERYHISTORY_H

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QDialog>

class QLineEdit;
class QListView;
class QPlainTextEdit;
class QPushButton;
class QSortFilterProxyModel;
class QStandardItemModel;
class toConnection;

/**
 * Persistent history of successfully executed statements, one JSON file per
 * connection in <settings dir>/history/ (see toSettings::directory()).
 * Newest entry first; re-running a statement moves it to the top.
 * The file is re-read on every call so all editors on a connection share it.
 */
class toQueryHistory
{
    public:
        struct Entry
        {
            QString sql;
            QDateTime executed;
        };

        explicit toQueryHistory(const toConnection &conn);

        QList<Entry> entries() const;

        /** Record @p sql as executed now. Statements that look like they carry
         *  a password (IDENTIFIED BY, PASSWORD '...') are never stored. */
        void add(const QString &sql);
        void remove(const QString &sql);

        /** Max entries kept per connection (Worksheet::QueryHistoryInt). */
        static int limit();

    private:
        void save(const QList<Entry> &entries) const;

        QString m_key;   // provider + full connection description, stored in the file
        QString m_file;
};

/**
 * Ctrl+H popup: filter box, list of past statements, preview of the selected
 * one. Enter / double click accepts; selectedSql() returns the statement.
 */
class toQueryHistoryDialog : public QDialog
{
        Q_OBJECT;
    public:
        toQueryHistoryDialog(toQueryHistory &history, QWidget *parent);

        QString selectedSql() const;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private slots:
        void slotFilterChanged(const QString &text);
        void slotCurrentChanged();
        void slotDelete();

    private:
        void reload();

        toQueryHistory &m_history;
        QLineEdit *m_filter;
        QListView *m_list;
        QPlainTextEdit *m_preview;
        QPushButton *m_deleteButton;
        QStandardItemModel *m_model;
        QSortFilterProxyModel *m_proxy;
};

#endif
