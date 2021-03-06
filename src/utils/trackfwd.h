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

#include "models/album.h"
#include "models/artist.h"
#include "models/track.h"
#include "utils/typedefs.h"

using TrackPtrList = QList<Track*>;
using TrackHash = QHash<int, Track*>;
using TrackPathMap = QHash<QString, Track*>;
using TrackList = QList<Track>;

using AlbumList = QList<Album>;
using ArtistHash = QHash<int, Artist>;
using GenreHash = QHash<int, QString>;

using ContainerHash = std::unordered_map<QString, std::unique_ptr<Container>>;
