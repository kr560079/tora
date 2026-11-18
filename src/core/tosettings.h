
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

#ifndef TOSETTINGS_H
#define TOSETTINGS_H

#include "core/tora_export.h"

#include <QtCore/QSettings>

/**
 * QSettings stored as .ini files in the TOra settings directory instead of
 * the Windows registry (which is wiped on non-persistent VMs/VDI profiles).
 *
 * Directory: $TORA_HOME if set, otherwise ~/.tora (%USERPROFILE%\.tora).
 *   tora.ini         preferences, window layout, per-tool state
 *   connections.ini  saved connections ("connections" group)
 *
 * On first use, when neither file exists yet, everything found in the old
 * native store (HKCU\Software\TOra\TOra3 on Windows) is copied over once.
 * The registry copy is left in place untouched.
 */
class TORA_EXPORT toSettings : public QSettings
{
    public:
        enum Store
        {
            Main,
            Connections
        };

        explicit toSettings(Store store = Main, QObject *parent = 0);

        /** Directory holding all TOra settings files (created on demand). */
        static QString directory();

        /** Absolute path of the .ini file backing @p store. */
        static QString storePath(Store store);

    private:
        static bool migrateNativeSettings();
};

#endif
