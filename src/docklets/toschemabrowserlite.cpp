
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

#include "docklets/toschemabrowserlite.h"
#include "core/toconnection.h"
#include "core/toconnectionregistry.h"
#include "core/tocache.h"
#include "core/toeditwidget.h"
#include "editor/toworksheettext.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QApplication>

REGISTER_VIEW("Schema Objects", toSchemaBrowserLite);

toSchemaBrowserLite::toSchemaBrowserLite(QWidget *parent, toWFlags flags)
    : toDocklet(tr("Schema Objects"), parent, flags)
    , LastEditor(NULL)
{
    setObjectName("Schema Objects Docklet");

    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    SchemaCombo = new QComboBox(container);
    layout->addWidget(SchemaCombo);

    FilterEdit = new QLineEdit(container);
    FilterEdit->setPlaceholderText(tr("Filter tables/views..."));
    FilterEdit->setClearButtonEnabled(true);
    layout->addWidget(FilterEdit);

    ListView = new QListView(container);
    layout->addWidget(ListView);

    Model = new QStringListModel(this);
    ProxyModel = new QSortFilterProxyModel(this);
    ProxyModel->setSourceModel(Model);
    ProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ListView->setModel(ProxyModel);

    setFocusProxy(FilterEdit);
    setWidget(container);

    connect(SchemaCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshObjectList()));
    connect(FilterEdit, SIGNAL(textChanged(QString)), this, SLOT(handleFilterChanged(QString)));
    connect(ListView, SIGNAL(activated(const QModelIndex &)), this, SLOT(handleActivated(const QModelIndex &)));
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(handleFocusChanged(QWidget*,QWidget*)));

    refreshSchemaList();
}

QIcon toSchemaBrowserLite::icon() const
{
    return QIcon(":/icons/database.xpm");
}

QString toSchemaBrowserLite::name() const
{
    return tr("Schema Objects");
}

void toSchemaBrowserLite::refreshSchemaList()
{
    try
    {
        toConnection &connection = toConnectionRegistrySing::Instance().currentConnection();
        QStringList owners = connection.getCache().userList(toCache::OWNERS);
        owners.sort();

        QString const wanted = connection.defaultSchema().toUpper();

        bool const oldState = SchemaCombo->blockSignals(true);
        SchemaCombo->clear();
        SchemaCombo->addItems(owners);
        int idx = SchemaCombo->findText(wanted, Qt::MatchFixedString);
        if (idx >= 0)
            SchemaCombo->setCurrentIndex(idx);
        SchemaCombo->blockSignals(oldState);
    }
    catch (...)
    {
        // No current connection yet - leave the combo empty.
        SchemaCombo->clear();
        Model->setStringList(QStringList());
        return;
    }

    refreshObjectList();
}

void toSchemaBrowserLite::refreshObjectList()
{
    QString const schema = SchemaCombo->currentText();
    if (schema.isEmpty())
    {
        Model->setStringList(QStringList());
        return;
    }

    try
    {
        toConnection &connection = toConnectionRegistrySing::Instance().currentConnection();
        QStringList entries;

        foreach(toCache::CacheEntry const* e, connection.getCache().getEntriesInSchema(schema, toCache::TABLE))
            entries << ("T  " + e->name.second);
        foreach(toCache::CacheEntry const* e, connection.getCache().getEntriesInSchema(schema, toCache::VIEW))
            entries << ("V  " + e->name.second);

        entries.sort();
        Model->setStringList(entries);
    }
    catch (...)
    {
        Model->setStringList(QStringList());
    }
}

void toSchemaBrowserLite::handleFilterChanged(QString const& text)
{
    ProxyModel->setFilterFixedString(text);
}

void toSchemaBrowserLite::handleActivated(const QModelIndex &index)
{
    if (!index.isValid() || LastEditor == NULL)
        return;

    toWorksheetText *editor = dynamic_cast<toWorksheetText*>(LastEditor);
    if (!editor)
        return;

    // Strip the "T  "/"V  " type marker added in refreshObjectList().
    QString text = ProxyModel->data(index, Qt::DisplayRole).toString();
    text = text.mid(3);

    editor->insertObjectReference(text);
}

void toSchemaBrowserLite::handleFocusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old);
    toEditWidget *edit = toEditWidget::findEdit(now);
    if (edit && dynamic_cast<toWorksheetText*>(edit))
    {
        bool const isNewEditor = (edit != LastEditor);
        LastEditor = edit;
        if (isNewEditor)
            refreshSchemaList(); // likely switched worksheets/connections
    }
}
