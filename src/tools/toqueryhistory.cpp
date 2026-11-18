
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

#include "tools/toqueryhistory.h"
#include "tools/toworksheet.h"
#include "core/toconfiguration.h"
#include "core/toconnection.h"
#include "core/tosettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>
#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QFontDatabase>
#include <QtGui/QKeyEvent>
#include <QtGui/QStandardItemModel>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

static const int SqlRole = Qt::UserRole + 1;

toQueryHistory::toQueryHistory(const toConnection &conn)
    : m_key(conn.provider() + QString::fromLatin1(":") + conn.fullDescription())
{
    // readable prefix (user@sid) + hash of the full key so that different
    // hosts with the same SID never share a file
    QString name = conn.description(false);
    name.replace(QRegularExpression(QString::fromLatin1("[^A-Za-z0-9@._-]")), QString::fromLatin1("_"));
    QString hash = QString::fromLatin1(QCryptographicHash::hash(m_key.toUtf8(), QCryptographicHash::Sha1).toHex().left(10));
    m_file = toSettings::directory() + QString::fromLatin1("/history/") + name.left(60) +
             QString::fromLatin1("-") + hash + QString::fromLatin1(".json");
}

int toQueryHistory::limit()
{
    return toConfigurationNewSingle::Instance().option(ToConfiguration::Worksheet::QueryHistoryInt).toInt();
}

QList<toQueryHistory::Entry> toQueryHistory::entries() const
{
    QList<Entry> ret;
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly))
        return ret;

    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).object().value(QString::fromLatin1("entries")).toArray();
    Q_FOREACH(QJsonValue const& v, arr)
    {
        QJsonObject o = v.toObject();
        Entry e;
        e.sql = o.value(QString::fromLatin1("sql")).toString();
        e.executed = QDateTime::fromString(o.value(QString::fromLatin1("executed")).toString(), Qt::ISODate);
        if (!e.sql.isEmpty())
            ret << e;
    }
    return ret;
}

void toQueryHistory::save(const QList<Entry> &entries) const
{
    QJsonArray arr;
    Q_FOREACH(Entry const& e, entries)
    {
        QJsonObject o;
        o.insert(QString::fromLatin1("sql"), e.sql);
        o.insert(QString::fromLatin1("executed"), e.executed.toString(Qt::ISODate));
        arr.append(o);
    }
    QJsonObject root;
    root.insert(QString::fromLatin1("connection"), m_key);
    root.insert(QString::fromLatin1("entries"), arr);

    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QSaveFile f(m_file); // atomic: never leaves a half-written file behind
    if (f.open(QIODevice::WriteOnly))
    {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.commit();
    }
}

void toQueryHistory::add(const QString &sql)
{
    QString stmt = sql.trimmed();
    if (stmt.isEmpty())
        return;

    static const QRegularExpression secret(QString::fromLatin1("\\bIDENTIFIED\\s+BY\\b|\\bPASSWORD\\s+'"),
                                           QRegularExpression::CaseInsensitiveOption);
    if (secret.match(stmt).hasMatch())
        return;

    int max = limit();
    if (max <= 0)
        return;

    QList<Entry> list = entries();
    QString key = stmt.simplified();
    for (int i = list.size() - 1; i >= 0; --i)
        if (list.at(i).sql.simplified() == key)
            list.removeAt(i);

    Entry e;
    e.sql = stmt;
    e.executed = QDateTime::currentDateTime();
    list.prepend(e);
    while (list.size() > max)
        list.removeLast();
    save(list);
}

void toQueryHistory::remove(const QString &sql)
{
    QList<Entry> list = entries();
    for (int i = list.size() - 1; i >= 0; --i)
        if (list.at(i).sql == sql)
            list.removeAt(i);
    save(list);
}

toQueryHistoryDialog::toQueryHistoryDialog(toQueryHistory &history, QWidget *parent)
    : QDialog(parent)
    , m_history(history)
{
    setWindowTitle(tr("Query History"));
    resize(800, 500);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Type to filter (matches anywhere in the statement)"));
    m_filter->setClearButtonEnabled(true);
    m_filter->installEventFilter(this);

    m_model = new QStandardItemModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterRole(SqlRole);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_list = new QListView(this);
    m_list->setModel(m_proxy);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setUniformItemSizes(true);

    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    QSplitter *split = new QSplitter(Qt::Vertical, this);
    split->addWidget(m_list);
    split->addWidget(m_preview);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    buttons->addButton(tr("&Insert"), QDialogButtonBox::AcceptRole)->setDefault(true);
    m_deleteButton = buttons->addButton(tr("&Delete entry"), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_filter);
    layout->addWidget(split);
    layout->addWidget(new QLabel(tr("Up/Down to choose, Enter to insert into the editor, Esc to close."), this));
    layout->addWidget(buttons);

    connect(m_filter, SIGNAL(textChanged(const QString &)), this, SLOT(slotFilterChanged(const QString &)));
    connect(m_list->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(slotCurrentChanged()));
    connect(m_list, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(accept()));
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(slotDelete()));
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

    reload();
    m_filter->setFocus();
}

void toQueryHistoryDialog::reload()
{
    m_model->clear();
    Q_FOREACH(toQueryHistory::Entry const& e, m_history.entries())
    {
        QString oneLine = e.sql.simplified();
        if (oneLine.length() > 150)
            oneLine = oneLine.left(150) + QString::fromLatin1("...");
        QStandardItem *item = new QStandardItem(e.executed.toString(QString::fromLatin1("yyyy-MM-dd HH:mm")) +
                                                QString::fromLatin1("   ") + oneLine);
        item->setData(e.sql, SqlRole);
        item->setToolTip(e.sql);
        m_model->appendRow(item);
    }
    slotFilterChanged(m_filter->text());
}

void toQueryHistoryDialog::slotFilterChanged(const QString &text)
{
    m_proxy->setFilterFixedString(text);
    if (m_proxy->rowCount() > 0)
        m_list->setCurrentIndex(m_proxy->index(0, 0));
    slotCurrentChanged();
}

void toQueryHistoryDialog::slotCurrentChanged()
{
    QString sql = selectedSql();
    m_preview->setPlainText(sql);
    m_deleteButton->setEnabled(!sql.isEmpty());
}

void toQueryHistoryDialog::slotDelete()
{
    QString sql = selectedSql();
    if (sql.isEmpty())
        return;
    int row = m_list->currentIndex().row();
    m_history.remove(sql);
    reload();
    if (m_proxy->rowCount() > 0)
        m_list->setCurrentIndex(m_proxy->index(qMin(row, m_proxy->rowCount() - 1), 0));
}

QString toQueryHistoryDialog::selectedSql() const
{
    QModelIndex idx = m_list->currentIndex();
    return idx.isValid() ? idx.data(SqlRole).toString() : QString();
}

bool toQueryHistoryDialog::eventFilter(QObject *obj, QEvent *event)
{
    // keep focus in the filter box but let Up/Down/PgUp/PgDn drive the list
    if (obj == m_filter && event->type() == QEvent::KeyPress)
    {
        int key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_PageUp || key == Qt::Key_PageDown)
        {
            QCoreApplication::sendEvent(m_list, event);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}
