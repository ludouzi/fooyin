/*
 * Fooyin
 * Copyright 2022, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "utils/typedefs.h"

#include <QString>

namespace Library {
class LibraryInfo
{
public:
    LibraryInfo() = default;
    LibraryInfo(const QString& path, QString name, int id);
    ~LibraryInfo();

    [[nodiscard]] QString path() const;
    [[nodiscard]] QString name() const;

    [[nodiscard]] int id() const;

private:
    QString m_path;
    QString m_name;
    int m_id;
};
} // namespace Library
