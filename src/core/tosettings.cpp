
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

#include "core/tosettings.h"
#include "core/toconf.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

// NOTE: toConfiguration (and so the first toSettings) is created during static
// initialization, before QApplication exists. Only use what works pre-main here.

toSettings::toSettings(Store store, QObject *parent)
    : QSettings(storePath(store), QSettings::IniFormat, parent)
{
    setIniCodec("UTF-8");
}

QString toSettings::directory()
{
    QString dir = QString::fromLocal8Bit(qgetenv("TORA_HOME"));
    if (dir.isEmpty())
        dir = QDir::homePath() + QString::fromLatin1("/.tora");
    return QDir::cleanPath(dir);
}

QString toSettings::storePath(Store store)
{
    // thread-safe, runs exactly once, before the first file is opened
    static const bool migrated = migrateNativeSettings();
    Q_UNUSED(migrated);

    switch (store)
    {
        case Connections:
            return directory() + QString::fromLatin1("/connections.ini");
        case Main:
        default:
            return directory() + QString::fromLatin1("/tora.ini");
    }
}

bool toSettings::migrateNativeSettings()
{
    QDir dir(directory());
    QString mainPath = dir.filePath(QString::fromLatin1("tora.ini"));
    QString connPath = dir.filePath(QString::fromLatin1("connections.ini"));

    if (!dir.mkpath(QString::fromLatin1(".")))
        return false;
    if (QFileInfo::exists(mainPath) || QFileInfo::exists(connPath))
        return false; // already using the ini files

    QSettings native(QSettings::NativeFormat, QSettings::UserScope, TOORGNAME, TOAPPNAME);
    QStringList keys = native.allKeys();
    if (keys.isEmpty())
        return false; // fresh install

    QSettings mainIni(mainPath, QSettings::IniFormat);
    QSettings connIni(connPath, QSettings::IniFormat);
    mainIni.setIniCodec("UTF-8");
    connIni.setIniCodec("UTF-8");
    Q_FOREACH(QString const& key, keys)
    {
        QSettings &target = key.startsWith(QLatin1String("connections/"), Qt::CaseInsensitive) ? connIni : mainIni;
        target.setValue(key, native.value(key));
    }
    mainIni.sync();
    connIni.sync();
    return true;
}
