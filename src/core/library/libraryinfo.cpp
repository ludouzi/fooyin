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

#include "libraryinfo.h"

#include "utils/utils.h"

namespace Library {
LibraryInfo::LibraryInfo(const QString& path, QString name, int id)
    : m_path(Util::File::cleanPath(path))
    , m_name(std::move(name))
    , m_id(id)
{ }

LibraryInfo::~LibraryInfo() = default;

QString LibraryInfo::path() const
{
    return m_path;
}

QString LibraryInfo::name() const
{
    return m_name;
}

int LibraryInfo::id() const
{
    return m_id;
}
} // namespace Library
