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

#include "core/library/libraryinfo.h"
#include "utils/trackfwd.h"
#include "utils/worker.h"

class QDir;

namespace Library {
class LibraryManager;
class LibraryScanner : public Worker
{
    Q_OBJECT

public:
    explicit LibraryScanner(LibraryManager* libraryManager, QObject* parent = nullptr);
    ~LibraryScanner() override;

    void stopThread() override;

    void scanLibrary(TrackPtrList& tracks, const LibraryInfo& info);
    void scanAll(TrackPtrList& tracks);

signals:
    void libraryAdded();
    void updatedTracks(TrackList& tracks);
    void addedTracks(TrackList& tracks);
    void tracksDeleted(const IdSet& tracks);

protected:
    void storeTracks(TrackList& tracks) const;
    static QStringList getFiles(QDir& baseDirectory);
    bool getAndSaveAllFiles(int libraryId, const QString& path, const TrackPathMap& tracks);

private:
    LibraryManager* m_libraryManager;
};
} // namespace Library
