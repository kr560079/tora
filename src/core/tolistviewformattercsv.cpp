
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

#include "core/tolistviewformattercsv.h"
#include "core/tolistviewformatterfactory.h"
#include "core/tolistviewformatteridentifier.h"

#include <QtCore/QRegExp>

#include <iostream>
#include <vector>
#include "tools/toresultview.h"

namespace
{
    toListViewFormatter* createCSV()
    {
        return new toListViewFormatterCSV();
    }
    const bool registered = toListViewFormatterFactory::Instance().Register(
                                toListViewFormatterIdentifier::CSV, createCSV);
}


toListViewFormatterCSV::toListViewFormatterCSV() : toListViewFormatter()
{
}

toListViewFormatterCSV::~toListViewFormatterCSV()
{
}

QString toListViewFormatterCSV::QuoteString(const QString &str)
{
    static QRegExp quote(QString::fromLatin1("\""));
    QString t = str;
    t.replace(quote, QString::fromLatin1("\"\""));
    return t;
}

// Standard CSV quoting (RFC 4180): only wrap a field in the delimiter
// (quote) character when it actually needs it - i.e. when it contains the
// separator, the delimiter itself, or a line break. Quoting every single
// field regardless of content (the previous behavior here) is not how CSV
// is normally produced and makes the output noisier than it needs to be.
static QString FormatCsvField(QString const& text, QString const& separator, QString const& delimiter)
{
    if (delimiter.isEmpty())
        return text;

    bool const needsQuoting = text.contains(delimiter)
            || (!separator.isEmpty() && text.contains(separator))
            || text.contains('\n') || text.contains('\r');

    if (!needsQuoting)
        return text;

    QString escaped = text;
    escaped.replace(delimiter, delimiter + delimiter); // double any embedded delimiter/quote chars
    return delimiter + escaped + delimiter;
}

QString toListViewFormatterCSV::getFormattedString(toExportSettings &settings,
        //const toResultModel *model);
        const QAbstractItemModel * model)
{
    int     columns   = model->columnCount();
    int     rows      = model->rowCount();
    QString separator = settings.separator;
    QString delimiter = settings.delimiter;

    QString output;
    QString indent;

    QVector<int> rlist = selectedRows(settings.selected);
    QVector<int> clist = selectedColumns(settings.selected);

    if (settings.columnsHeader)
    {
        for (int j = (settings.rowsHeader ? 0 : 1); j < columns; j++)
        {
            if (settings.columnsExport == toExportSettings::ColumnsSelected && !clist.contains(j))
                continue;
            output += FormatCsvField(model->headerData(
                                          j,
                                          Qt::Horizontal,
                                          Qt::DisplayRole).toString(),
                                      separator, delimiter)
                      + separator;
        }
        if (output.length() > 0)
            output = output.left(output.length() - separator.length());

        endLine(output);
    }

    QModelIndex mi;
    for (int row = 0; row < rows; row++)
    {
        if (settings.rowsExport == toExportSettings::RowsSelected && !rlist.contains(row))
            continue;

        QString line;
        for (int i = 0; i < columns; i++)
        {
            if (settings.columnsExport == toExportSettings::ColumnsSelected && !clist.contains(i))
                continue;
            if (!settings.rowsHeader && i == 0)
                continue;

            mi = model->index(row, i);
            QString text = model->data(mi, Qt::EditRole).toString();
            line += indent;

            line += FormatCsvField(text, separator, delimiter) + separator;
        }

        line = line.left(line.length() - separator.length());
        output += line;
        endLine(output);
    }

    return output;
}
