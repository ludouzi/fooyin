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

#include "libraryscanner.h"

#include "core/library/librarymanager.h"
#include "core/tagging/tags.h"
#include "database/database.h"
#include "database/librarydatabase.h"
#include "models/album.h"
#include "models/track.h"
#include "utils/utils.h"

#include <QDir>

namespace Library {
LibraryScanner::LibraryScanner(LibraryManager* libraryManager, QObject* parent)
    : Worker(parent)
    , m_libraryManager(libraryManager)
{ }

LibraryScanner::~LibraryScanner()
{
    DB::Database::instance()->closeDatabase();
}

void LibraryScanner::stopThread()
{
    setState(State::Idle);
}

void LibraryScanner::scanLibrary(TrackPtrList& tracks, const LibraryInfo& info)
{
    if(isRunning()) {
        return;
    }

    auto* db = DB::Database::instance();
    DB::LibraryDatabase* libraryDatabase = db->libraryDatabase();

    setState(State::Running);

    TrackPathMap trackMap{};
    IdSet tracksToDelete{};

    for(const auto& track : tracks) {
        if(!Util::File::exists(track->filepath())) {
            tracksToDelete.insert(track->id());
        }
        else {
            trackMap.insert(track->filepath(), track);
        }
        if(track->hasCover() && !Util::File::exists(track->coverPath())) {
            Util::storeCover(*track);
        }
        if(!mayRun()) {
            return;
        }
    }

    bool deletedSuccess = libraryDatabase->deleteTracks(tracksToDelete);

    if(deletedSuccess && !tracksToDelete.empty()) {
        emit tracksDeleted(tracksToDelete);
    }

    if(!mayRun()) {
        return;
    }

    getAndSaveAllFiles(info.id(), info.path(), trackMap);

    setState(State::Idle);
}

void LibraryScanner::scanAll(TrackPtrList& tracks)
{
    const auto libraries = m_libraryManager->allLibraries();

    for(const auto& info : libraries) {
        scanLibrary(tracks, info);
    }
}

void LibraryScanner::storeTracks(TrackList& tracks) const
{
    if(!mayRun()) {
        return;
    }

    auto* db = DB::Database::instance();
    DB::LibraryDatabase* libraryDatabase = db->libraryDatabase();

    libraryDatabase->storeTracks(tracks);

    if(!mayRun()) {
        return;
    }
}

QStringList LibraryScanner::getFiles(QDir& baseDirectory)
{
    QStringList ret;
    QList<QDir> stack{baseDirectory};

    QStringList soundFileExtensions{"*.mp3", "*.ogg", "*.opus", "*.oga",  "*.m4a", "*.wav",  "*.flac",
                                    "*.aac", "*.wma", "*.mpc",  "*.aiff", "*.ape", "*.webm", "*.mp4"};

    while(!stack.isEmpty()) {
        QDir dir = stack.takeFirst();
        for(const auto& subDir : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            stack.append(QDir{subDir.absoluteFilePath()});
        }
        for(const auto& file : dir.entryInfoList(soundFileExtensions, QDir::Files)) {
            ret.append(file.absoluteFilePath());
        }
    }
    return ret;
}

bool LibraryScanner::getAndSaveAllFiles(int libraryId, const QString& path, const TrackPathMap& tracks)
{
    if(path.isEmpty() || !Util::File::exists(path)) {
        return false;
    }

    QDir dir(path);

    TrackList tracksToStore{};
    TrackList tracksToUpdate{};

    QStringList files = getFiles(dir);

    for(const auto& filepath : files) {
        if(!mayRun()) {
            return false;
        }

        QFileInfo info{filepath};

        auto modified = info.lastModified().isValid() ? info.lastModified().toMSecsSinceEpoch() : 0;

        bool fileWasRead;

        Track* libraryTrack = tracks.value(filepath, nullptr);

        if(libraryTrack && libraryTrack->id() >= 0) {
            if(libraryTrack->mTime() == modified) {
                continue;
            }

            Track changedTrack{filepath};
            fileWasRead = Tagging::readMetaData(changedTrack, Tagging::Quality::Fast);
            if(fileWasRead) {
                tracksToUpdate << changedTrack;
                continue;
            }
        }

        Track track{filepath};
        track.setLibraryId(libraryId);

        fileWasRead = Tagging::readMetaData(track, Tagging::Quality::Quality);
        if(fileWasRead) {
            tracksToStore << track;
            if(tracksToStore.size() >= 500) {
                storeTracks(tracksToStore);
                emit addedTracks(tracksToStore);
                tracksToStore.clear();
            }
        }
    }

    storeTracks(tracksToStore);
    storeTracks(tracksToUpdate);

    if(!tracksToStore.isEmpty()) {
        emit addedTracks(tracksToStore);
    }
    if(!tracksToUpdate.isEmpty()) {
        emit updatedTracks(tracksToUpdate);
    }

    tracksToStore.clear();

    return true;
}
} // namespace Library
