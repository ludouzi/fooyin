/*
 * Fooyin
 * Copyright 2022-2023, Luke Taylor <LukeT1@proton.me>
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

#include "databasemodule.h"

#include <core/trackfwd.h>

namespace Fooyin {
class TrackDatabase : public DatabaseModule
{
public:
    explicit TrackDatabase(const QString& connectionName);

    bool storeTracks(TrackList& tracksToStore);

    bool getAllTracks(TrackList& result);
    bool getAllTracks(TrackList& result, int offset, int limit);

    [[nodiscard]] bool dbFetchTracks(DatabaseQuery& q, TrackList& result) const;
    [[nodiscard]] int dbTrackCount() const;

    bool updateTrack(const Track& track);
    bool deleteTrack(int id);
    bool deleteTracks(const TrackList& tracks);

private:
    int insertTrack(const Track& track);

    QString m_connectionName;
};
} // namespace Fooyin