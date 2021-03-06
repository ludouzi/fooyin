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

#include "utils/filterfwd.h"
#include "utils/typedefs.h"

#include <QAbstractListModel>

class FilterItem;

namespace Library {
class MusicLibrary;
class FilterModel : public QAbstractListModel
{
public:
    FilterModel(Filters::FilterType type, int index, MusicLibrary* library, QObject* parent = nullptr);
    ~FilterModel() override;

    void setType(Filters::FilterType type);
    [[nodiscard]] int index() const;
    void setIndex(int index);

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent) const override;

    MusicLibrary* library();

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits,
                                        Qt::MatchFlags flags) const override;
    void reload(const FilterList& result);

protected:
    void beginReset();
    void setupModelData(const FilterList& items);

private:
    std::unique_ptr<FilterItem> m_root;
    Filters::FilterType m_type;
    int m_index;
    MusicLibrary* m_library;
};
} // namespace Library
