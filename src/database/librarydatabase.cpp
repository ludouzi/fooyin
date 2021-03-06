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

#include "librarydatabase.h"

#include "core/tagging/tags.h"
#include "query.h"
#include "utils/helpers.h"
#include "utils/paths.h"
#include "utils/utils.h"

#include <QBuffer>
#include <QPixmap>

namespace {
QMap<QString, QVariant> getTrackBindings(const Track& track)
{
    return QMap<QString, QVariant>{
        {QStringLiteral("FilePath"), Util::File::cleanPath(track.filepath())},
        {QStringLiteral("Title"), track.title()},
        {QStringLiteral("TrackNumber"), track.trackNumber()},
        {QStringLiteral("AlbumArtistID"), track.albumArtistId()},
        {QStringLiteral("AlbumID"), track.albumId()},
        {QStringLiteral("CoverPath"), track.coverPath()},
        {QStringLiteral("DiscNumber"), track.discNumber()},
        {QStringLiteral("Year"), track.year()},
        {QStringLiteral("Lyrics"), track.lyrics()},
        {QStringLiteral("Comment"), track.comment()},
        {QStringLiteral("Duration"), track.duration()},
        {QStringLiteral("FileSize"), track.fileSize()},
        {QStringLiteral("BitRate"), track.bitrate()},
        {QStringLiteral("SampleRate"), track.sampleRate()},
        {QStringLiteral("ExtraTags"), track.extraTagsToJson()},
        {QStringLiteral("AddedDate"), track.addedTime()},
        {QStringLiteral("ModifiedDate"), track.mTime()},
        {QStringLiteral("LibraryID"), track.libraryId()},
    };
}

QString getFilterJoins(const Filters::FilterType type = {})
{
    QString joins;
    switch(type) {
        case(Filters::FilterType::AlbumArtist):
            joins += "LEFT JOIN Artists AS AlbumArtists ON AlbumArtists.ArtistID = "
                     "Tracks.AlbumArtistID ";
            break;
        case(Filters::FilterType::Artist):
            joins += "LEFT JOIN TrackArtists ON TrackArtists.TrackID = Tracks.TrackID ";
            joins += "LEFT JOIN Artists ON Artists.ArtistID = TrackArtists.ArtistID ";
            break;
        case(Filters::FilterType::Album):
            joins += "LEFT JOIN Albums ON Albums.AlbumID = Tracks.AlbumID ";
            break;
        case(Filters::FilterType::Genre):
            joins += "INNER JOIN TrackGenres ON TrackGenres.TrackID = Tracks.TrackID ";
            joins += "LEFT JOIN Genres ON Genres.GenreID = TrackGenres.GenreID ";
            break;
        case(Filters::FilterType::Year):
            break;
    }
    return joins;
}
QString getSearchJoins()
{
    QString joins;
    joins += "LEFT JOIN Artists AS AlbumArtists ON AlbumArtists.ArtistID = Tracks.AlbumArtistID ";
    joins += "LEFT JOIN Albums ON Albums.AlbumID = Tracks.AlbumID ";
    joins += "LEFT JOIN TrackArtists ON TrackArtists.TrackID = Tracks.TrackID ";
    joins += "LEFT JOIN Artists ON Artists.ArtistID = TrackArtists.ArtistID ";
    joins += "LEFT JOIN TrackGenres ON TrackGenres.TrackID = Tracks.TrackID ";
    joins += "LEFT JOIN Genres ON Genres.GenreID = TrackGenres.GenreID ";

    return joins;
}
} // namespace

namespace DB {
LibraryDatabase::LibraryDatabase(const QString& connectionName, int libraryId)
    : DB::Module(connectionName)
    , m_libraryId(libraryId)
    , m_connectionName(connectionName)
{ }

LibraryDatabase::~LibraryDatabase() = default;

bool LibraryDatabase::insertArtistsAlbums(TrackList& tracks)
{
    if(tracks.isEmpty()) {
        return {};
    }

    // Gather all albums
    QHash<QString, Album> albumMap;
    {
        AlbumList dbAlbums;
        getAllAlbums(dbAlbums);

        for(const auto& album : qAsConst(dbAlbums)) {
            QString hash = Util::calcAlbumHash(album.title(), album.artist(), album.year());
            albumMap.insert(hash, album);
        }
    }

    // Gather all artists
    QHash<QString, Artist> artistMap;
    {
        ArtistHash dbArtists;
        getAllArtists(dbArtists);

        for(const auto& artist : qAsConst(dbArtists)) {
            artistMap.insert(artist.name(), artist);
        }
    }

    // Gather all genres
    QHash<QString, int> genreMap;
    {
        GenreHash dbGenres;
        getAllGenres(dbGenres);

        for(const auto& [id, name] : asRange(dbGenres)) {
            genreMap.insert(name, id);
        }
    }

    // Gather all tracks
    QHash<QString, Track> trackMap;
    {
        TrackList dbTracks;
        getAllTracks(dbTracks);

        for(const auto& track : qAsConst(dbTracks)) {
            trackMap.insert(track.filepath(), track);
        }
    }

    db().transaction();

    for(auto& track : tracks) {
        if(track.libraryId() < 0) {
            track.setLibraryId(m_libraryId);
        }

        // Check artists
        for(const auto& trackArtist : track.artists()) {
            if(!artistMap.contains(trackArtist)) {
                Artist artist{trackArtist};
                int id = insertArtist(artist);
                artist.setId(id);
                artistMap.insert(trackArtist, artist);
            }
            auto artist = artistMap.value(trackArtist);
            track.addArtistId(artist.id());
        }

        // Check album artist
        if(!artistMap.contains(track.albumArtist())) {
            Artist albumArtist{track.albumArtist()};
            int id = insertArtist(albumArtist);
            albumArtist.setId(id);
            artistMap.insert(track.albumArtist(), albumArtist);
        }
        auto albumArtist = artistMap.value(track.albumArtist());
        track.setAlbumArtistId(albumArtist.id());

        // Check genres
        for(const auto& genre : track.genres()) {
            if(!genreMap.contains(genre)) {
                int id = insertGenre(genre);
                genreMap.insert(genre, id);
            }
            int trackGenre = genreMap.value(genre);
            track.addGenreId(trackGenre);
        }

        // Check album id
        QString hash = Util::calcAlbumHash(track.album(), track.albumArtist(), track.year());
        if(!albumMap.contains(hash)) {
            Album album{track.album()};
            album.setYear(track.year());
            album.setGenres(track.genres());
            album.setArtistId(albumArtist.id());
            album.setArtist(track.albumArtist());

            int id = insertAlbum(album);
            album.setId(id);
            albumMap.insert(hash, album);
        }
        auto album = albumMap.value(hash);
        track.setAlbumId(album.id());
        if(!album.hasCover()) {
            QString coverPath = Util::storeCover(track);
            album.setCoverPath(coverPath);
        }
        track.setCoverPath(album.coverPath());

        // Check track id
        if(trackMap.contains(track.filepath())) {
            int id = trackMap.value(track.filepath()).id();
            track.setId(id);
        }

        db().commit();
    }
    return true;
}

bool LibraryDatabase::storeTracks(TrackList& tracks)
{
    if(tracks.isEmpty()) {
        return true;
    }

    insertArtistsAlbums(tracks);

    db().transaction();

    for(auto& track : tracks) {
        if(track.id() >= 0) {
            updateTrack(track);
            updateTrackArtists(track.id(), track.artistIds());
            updateTrackGenres(track.id(), track.genreIds());
        }
        else {
            int id = insertTrack(track);
            track.setId(id);
            insertTrackArtists(id, track.artistIds());
            insertTrackGenres(id, track.genreIds());
        }
    }

    return db().commit();
}

bool LibraryDatabase::getAllTracks(TrackList& result) const
{
    auto q = Query(module());
    const auto query = fetchQueryTracks({}, {});
    q.prepareQuery(query);

    return dbFetchTracks(q, result);
}

bool LibraryDatabase::getAllAlbums(AlbumList& result) const
{
    const auto query = QString("%1 GROUP BY AlbumView.AlbumID, AlbumView.Title;").arg(fetchQueryAlbums({}, {}));

    auto q = Query(module());
    q.prepareQuery(query);

    return dbFetchAlbums(q, result);
}

bool LibraryDatabase::getAllArtists(ArtistHash& result) const
{
    const auto queryText = QString("%1 GROUP BY Artists.ArtistID, Artists.Name;").arg(fetchQueryArtists(""));

    auto query = Query(module());
    query.prepareQuery(queryText);

    return dbFetchArtists(query, result);
}

bool LibraryDatabase::getAllGenres(GenreHash& result) const
{
    const auto queryText = QString("%1;").arg(fetchQueryGenres(""));

    auto query = Query(module());
    query.prepareQuery(queryText);

    return dbFetchGenres(query, result);
}

bool LibraryDatabase::getAllItems(Filters::FilterType type, ::Library::SortOrder order, FilterList& result) const
{
    const auto join = getFilterJoins(type);
    const auto queryText = fetchQueryItems(type, {}, join, order);
    if(!queryText.isEmpty()) {
        auto query = Query(module());
        query.prepareQuery(queryText);

        return dbFetchItems(query, result);
    }
    return false;
}

bool LibraryDatabase::getItemsByFilter(Filters::FilterType type, const ActiveFilters& filters, const QString& search,
                                       ::Library::SortOrder order, FilterList& result) const
{
    if(!filters.isEmpty() || !search.isEmpty()) {
        QString where;
        QString join;

        for(const auto& [filter, ids] : asRange(filters)) {
            QString values;

            for(const auto& id : ids) {
                if(!values.isEmpty()) {
                    values += ",";
                }
                values += QString::number(id);
            }

            switch(filter) {
                case(Filters::FilterType::AlbumArtist): {
                    if(type != Filters::FilterType::AlbumArtist) {
                        if(!where.isEmpty()) {
                            where += " AND ";
                        }
                        where += QString("AlbumArtistID IN (%1)").arg(values);
                        if(search.isEmpty()) {
                            join += getFilterJoins(filter);
                        }
                    }
                    break;
                }
                case(Filters::FilterType::Artist): {
                    if(type != Filters::FilterType::Artist) {
                        if(!where.isEmpty()) {
                            where += " AND ";
                        }
                        where += QString("TrackArtists.ArtistID IN (%1)").arg(values);
                        if(search.isEmpty()) {
                            join += getFilterJoins(filter);
                        }
                    }
                    break;
                }
                case(Filters::FilterType::Album): {
                    if(type != Filters::FilterType::Album) {
                        if(!where.isEmpty()) {
                            where += " AND ";
                        }
                        where += QString("Tracks.AlbumID IN (%1)").arg(values);
                        if(search.isEmpty()) {
                            join += getFilterJoins(filter);
                        }
                    }
                    break;
                }
                case(Filters::FilterType::Year): {
                    if(type != Filters::FilterType::Year) {
                        if(!where.isEmpty()) {
                            where += " AND ";
                        }
                        where += QString("Tracks.Year IN (%1)").arg(values);
                    }
                    break;
                }
                case(Filters::FilterType::Genre): {
                    if(type != Filters::FilterType::Genre) {
                        if(!where.isEmpty()) {
                            where += " AND ";
                        }
                        where += QString("TrackGenres.GenreID IN (%1)").arg(values);
                        if(search.isEmpty()) {
                            join += getFilterJoins(filter);
                        }
                    }
                    break;
                }
            }
        }

        if(!search.isEmpty()) {
            if(!where.isEmpty()) {
                where += " AND ";
            }

            where += QString("(Tracks.Title LIKE '%%1%' OR Artists.Name LIKE '%1%%' "
                             "OR "
                             "AlbumArtists.Name LIKE '%%1%' OR Albums.Title LIKE "
                             "'%%1%')")
                         .arg(search.toLower());

            join += getSearchJoins();
        }
        else {
            join += getFilterJoins(type);
        }

        const auto queryText = fetchQueryItems(type, where, join, order);
        if(!queryText.isEmpty()) {
            auto query = Query(module());
            query.prepareQuery(queryText);

            return dbFetchItems(query, result);
        }
    }
    return false;
}

QString LibraryDatabase::fetchQueryTracks(const QString& where, const QString& join)
{
    static const auto fields = QStringList{
        QStringLiteral("TrackID"),       // 0
        QStringLiteral("FilePath"),      // 1
        QStringLiteral("Title"),         // 2
        QStringLiteral("TrackNumber"),   // 3
        QStringLiteral("ArtistIDs"),     // 4
        QStringLiteral("Artists"),       // 5
        QStringLiteral("AlbumArtistID"), // 6
        QStringLiteral("AlbumArtist"),   // 7
        QStringLiteral("AlbumID"),       // 8
        QStringLiteral("Album"),         // 9
        QStringLiteral("CoverPath"),     // 10
        QStringLiteral("DiscNumber"),    // 11
        QStringLiteral("Year"),          // 12
        QStringLiteral("GenreIDs"),      // 13
        QStringLiteral("Genres"),        // 14
        QStringLiteral("Lyrics"),        // 15
        QStringLiteral("Comment"),       // 16
        QStringLiteral("Duration"),      // 17
        QStringLiteral("PlayCount"),     // 18
        QStringLiteral("FileSize"),      // 19
        QStringLiteral("BitRate"),       // 20
        QStringLiteral("SampleRate"),    // 21
        QStringLiteral("ExtraTags"),     // 22
        QStringLiteral("AddedDate"),     // 23
        QStringLiteral("ModifiedDate"),  // 24
        QStringLiteral("LibraryID"),     // 25
    };

    const auto joinedFields = fields.join(", ");

    return QString("SELECT %1 FROM TrackView %2 WHERE %3;")
        .arg(joinedFields, join.isEmpty() ? "" : join, where.isEmpty() ? "1" : where);
}

QString LibraryDatabase::fetchQueryAlbums(const QString& where, const QString& join)
{
    static const auto fields = QStringList{
        QStringLiteral("AlbumView.AlbumID"),    // 0
        QStringLiteral("AlbumView.Title"),      // 1
        QStringLiteral("AlbumView.Year"),       // 2
        QStringLiteral("AlbumView.ArtistID"),   // 3
        QStringLiteral("AlbumView.ArtistName"), // 4
    };

    const auto joinedFields = fields.join(", ");

    auto query = QString("SELECT %1 FROM AlbumView %2 WHERE %3")
                     .arg(joinedFields, join.isEmpty() ? "" : join, where.isEmpty() ? "1" : where);

    return query;
}

QString LibraryDatabase::fetchQueryArtists(const QString& where)
{
    static const auto fields = QStringList{
        QStringLiteral("Artists.ArtistID"), // 0
        QStringLiteral("Artists.Name"),     // 1
    };

    const auto joinedFields = fields.join(", ");

    auto queryText = QString("SELECT %1 FROM Artists WHERE %2").arg(joinedFields, where.isEmpty() ? "1" : where);

    return queryText;
}

QString LibraryDatabase::fetchQueryGenres(const QString& where)
{
    static const auto fields = QStringList{
        QStringLiteral("Genres.GenreID"), // 0
        QStringLiteral("Genres.Name"),    // 1
    };

    const auto joinedFields = fields.join(", ");

    auto queryText = QString("SELECT %1 FROM Genres WHERE %2").arg(joinedFields, where.isEmpty() ? "1" : where);

    return queryText;
}

QString LibraryDatabase::fetchQueryItems(Filters::FilterType type, const QString& where, const QString& join,
                                         Library::SortOrder order)
{
    QStringList fields{};
    QString group{};
    QString sortOrder{};

    switch(type) {
        case(Filters::FilterType::AlbumArtist):
            fields.append(QStringLiteral("AlbumArtists.ArtistID"));
            fields.append(QStringLiteral("AlbumArtists.Name"));
            group = QStringLiteral("AlbumArtists.ArtistID");
            switch(order) {
                case(Library::SortOrder::TitleDesc):
                    sortOrder = QStringLiteral("LOWER(AlbumArtists.Name) DESC");
                    break;
                case(Library::SortOrder::TitleAsc):
                    sortOrder = QStringLiteral("LOWER(AlbumArtists.Name)");
                    break;
                case(Library::SortOrder::YearDesc):
                    sortOrder = QStringLiteral("Tracks.Year, LOWER(AlbumArtists.Name)");
                    break;
                case(Library::SortOrder::YearAsc):
                    sortOrder = QStringLiteral("Tracks.Year ASC, LOWER(AlbumArtists.Name)");
                    break;
                case(Library::SortOrder::NoSorting):
                    break;
            }
            break;
        case(Filters::FilterType::Artist):
            fields.append(QStringLiteral("Artists.ArtistID"));
            fields.append(QStringLiteral("Artists.Name"));
            group = QStringLiteral("Artists.ArtistID");
            sortOrder = QStringLiteral("LOWER(Artists.Name)");
            break;
        case(Filters::FilterType::Album):
            fields.append(QStringLiteral("Albums.AlbumID"));
            fields.append(QStringLiteral("Albums.Title"));
            group = QStringLiteral("Albums.AlbumID");
            switch(order) {
                case(Library::SortOrder::TitleDesc):
                    sortOrder = QStringLiteral("LOWER(Albums.Title) DESC");
                    break;
                case(Library::SortOrder::TitleAsc):
                    sortOrder = QStringLiteral("LOWER(Albums.Title)");
                    break;
                case(Library::SortOrder::YearDesc):
                    sortOrder = QStringLiteral("Albums.Year DESC, LOWER(Albums.Title)");
                    break;
                case(Library::SortOrder::YearAsc):
                    sortOrder = QStringLiteral("Albums.Year ASC, LOWER(Albums.Title)");
                    break;
                case(Library::SortOrder::NoSorting):
                    sortOrder = QStringLiteral("LOWER(Albums.Title)");
                    break;
            }
            break;
        case(Filters::FilterType::Year):
            fields.append(QStringLiteral("Tracks.Year"));
            fields.append(QStringLiteral("Tracks.Year"));
            group = QStringLiteral("Tracks.Year");
            break;
        case(Filters::FilterType::Genre):
            fields.append(QStringLiteral("Genres.GenreID"));
            fields.append(QStringLiteral("Genres.Name"));
            group = QStringLiteral("Genres.GenreID");
            break;
    }

    const auto joinedFields = fields.join(", ");

    auto queryText = QString("SELECT %1 FROM Tracks %2 WHERE %3 GROUP BY %4 ORDER BY %5")
                         .arg(joinedFields, join.isEmpty() ? "" : join, where.isEmpty() ? "1" : where,
                              group.isEmpty() ? "1" : group, sortOrder.isEmpty() ? "1" : sortOrder);

    return queryText;
}

bool LibraryDatabase::dbFetchTracks(Query& q, TrackList& result)
{
    result.clear();

    if(!q.execQuery()) {
        q.error("Cannot fetch tracks from database");
        return false;
    }

    while(q.next()) {
        Track track{q.value(1).toString()};

        track.setId(q.value(0).toInt());
        track.setTitle(q.value(2).toString());
        track.setTrackNumber(q.value(3).toInt());
        const QStringList artistIds = q.value(4).toString().split("|");
        track.setArtists(q.value(5).toString().split("|"));
        track.setAlbumArtistId(q.value(6).toInt());
        track.setAlbumArtist(q.value(7).toString());
        track.setAlbumId(q.value(8).toInt());
        track.setAlbum(q.value(9).toString());
        track.setCoverPath(q.value(10).toString());
        track.setDiscNumber(q.value(11).toInt());
        track.setYear(q.value(12).toInt());
        const QStringList genreIds = q.value(13).toString().split("|");
        track.setGenres(q.value(14).toString().split("|"));
        track.setLyrics(q.value(15).toString());
        track.setComment(q.value(16).toString());
        track.setDuration(q.value(17).value<quint64>());
        track.setPlayCount(q.value(18).toInt());
        track.setFileSize(q.value(19).toInt());
        track.setBitrate(q.value(20).toInt());
        track.setSampleRate(q.value(21).toInt());
        track.jsonToExtraTags(q.value(22).toByteArray());
        track.setAddedTime(static_cast<qint64>(q.value(23).toULongLong()));
        track.setMTime(static_cast<qint64>(q.value(24).toULongLong()));
        track.setLibraryId(q.value(25).toInt());

        for(const auto& id : artistIds) {
            track.addArtistId(id.toInt());
        }

        for(const auto& id : genreIds) {
            track.addGenreId(id.toInt());
        }

        result.append(track);
    }

    return true;
}

bool LibraryDatabase::dbFetchAlbums(Query& q, AlbumList& result)
{
    result.clear();

    if(!q.execQuery()) {
        q.error("Could not get all albums from database");
        return false;
    }

    while(q.next()) {
        Album album{q.value(1).toString()};

        album.setId(q.value(0).value<int>());
        album.setYear(q.value(2).value<int>());
        album.setArtistId(q.value(3).value<int>());
        album.setArtist(q.value(4).toString());
        album.setGenres(q.value(5).toString().split("|"));
        album.setDiscCount(q.value(6).value<int>());
        album.setTrackCount(q.value(7).value<int>());
        album.setDuration(q.value(8).value<quint64>());
        album.setCoverPath(q.value(9).toString());

        result.append(album);
    }

    return true;
}

bool LibraryDatabase::dbFetchArtists(Query& q, ArtistHash& result)
{
    result.clear();

    if(!q.execQuery()) {
        q.error("Could not get all artists from database");
        return false;
    }

    while(q.next()) {
        Artist artist{q.value(1).toString()};
        artist.setId(q.value(0).toInt());

        result.insert(artist.id(), artist);
    }

    return true;
}

bool LibraryDatabase::dbFetchGenres(Query& q, GenreHash& result)
{
    result.clear();

    if(!q.execQuery()) {
        q.error("Could not get all genres from database");
        return false;
    }

    while(q.next()) {
        auto id = q.value(0).toInt();
        auto genre = q.value(1).toString();

        result.insert(id, genre);
    }

    return true;
}

bool LibraryDatabase::dbFetchItems(Query& q, FilterList& result)
{
    result.clear();

    if(!q.execQuery()) {
        q.error("Could not get all items from database");
        return false;
    }

    while(q.next()) {
        FilterEntry item;

        item.id = q.value(0).toInt();
        item.name = q.value(1).toString();

        result.append(item);
    }

    return true;
}

bool LibraryDatabase::updateTrack(const Track& track)
{
    if(track.id() < 0 || track.albumId() < 0 || track.albumArtistId() < 0 || track.libraryId() < 0) {
        qDebug() << "Cannot update track (value negative): "
                 << " AlbumArtistID: " << track.albumArtistId() << " AlbumID: " << track.albumId()
                 << " TrackID: " << track.id() << " LibraryID: " << track.libraryId();
        return false;
    }

    auto bindings = getTrackBindings(track);

    const auto q = module()->update("Tracks", bindings, {"TrackID", track.id()},
                                    QString("Cannot update track %1").arg(track.filepath()));

    return (!q.hasError());
}

bool LibraryDatabase::deleteTrack(int id)
{
    const auto queryText = QStringLiteral("DELETE FROM Tracks WHERE TrackID = :TrackID;");
    const auto q = module()->runQuery(queryText, {":TrackID", id}, QString("Cannot delete track %1").arg(id));

    return (!q.hasError());
}

bool LibraryDatabase::deleteTracks(const IdSet& tracks)
{
    if(tracks.empty()) {
        return true;
    }

    module()->db().transaction();

    const int fileCount = static_cast<int>(std::count_if(tracks.cbegin(), tracks.cend(), [&](int trackId) {
        return deleteTrack(trackId);
    }));

    const auto success = module()->db().commit();

    return (success && (fileCount == static_cast<int>(tracks.size())));
}

bool LibraryDatabase::deleteLibraryTracks(int id)
{
    if(id < 0) {
        return false;
    }

    const auto query = QStringLiteral("DELETE FROM Tracks WHERE LibraryID=:libraryId;");
    const auto q = module()->runQuery(query, {":libraryId", id}, "Cannot delete library tracks");
    return (!q.hasError());
}

DB::Module* LibraryDatabase::module()
{
    return this;
}

const DB::Module* LibraryDatabase::module() const
{
    return this;
}

int LibraryDatabase::insertArtist(const Artist& artist)
{
    const auto bindings = QMap<QString, QVariant>{
        {"Name", artist.name()},
    };

    auto query = module()->insert("Artists", bindings, QString("Cannot insert artist %1").arg(artist.name()));
    return (query.hasError()) ? -1 : query.lastInsertId().toInt();
}

int LibraryDatabase::insertAlbum(const Album& album)
{
    const auto bindings
        = QMap<QString, QVariant>{{"Title", album.title()}, {"ArtistID", album.artistId()}, {"Year", album.year()}};

    const auto q = module()->insert("Albums", bindings, QString("Cannot insert album %1").arg(album.title()));

    return (!q.hasError()) ? q.lastInsertId().toInt() : -1;
}

bool LibraryDatabase::insertTrackArtists(int id, const IdSet& artists)
{
    for(const auto& artist : artists) {
        const auto bindings = QMap<QString, QVariant>{{"TrackID", id}, {"ArtistID", artist}};

        const auto q = module()->insert("TrackArtists", bindings, QString("Cannot insert track artist %1").arg(artist));
        if(!q.hasError()) {
            continue;
        }
        return false;
    }
    return true;
}

bool LibraryDatabase::insertTrackGenres(int id, const IdSet& genres)
{
    for(const auto& genre : genres) {
        const auto bindings = QMap<QString, QVariant>{{"TrackID", id}, {"GenreID", genre}};

        const auto q = module()->insert("TrackGenres", bindings, QString("Cannot insert track genre %1").arg(genre));
        if(!q.hasError()) {
            continue;
        }
        return false;
    }
    return true;
}

bool LibraryDatabase::updateTrackArtists(int id, const IdSet& artists)
{
    const auto query = QString("SELECT ArtistID FROM TrackArtists WHERE TrackID = %1;").arg(id);

    auto q = Query(module());
    q.prepareQuery(query);

    if(!q.execQuery()) {
        q.error("Could not get all track artists from database");
        return false;
    }

    IdSet databaseArtists;
    IdSet artistsToDelete;
    IdSet artistsToInsert;

    if(!q.hasError()) {
        while(q.next()) {
            int artistId = q.value(0).toInt();
            databaseArtists.insert(artistId);
        }
    }

    for(auto artistId : databaseArtists) {
        if(artists.contains(artistId)) {
            artistsToDelete.insert(artistId);
        }
    }

    for(auto artistId : artists) {
        if(databaseArtists.contains(artistId)) {
            artistsToInsert.insert(artistId);
        }
    }

    if(!artistsToDelete.empty()) {
        for(auto artistId : artistsToDelete) {
            const auto bindings = QList<QPair<QString, QVariant>>{{"TrackID", id}, {"ArtistID", artistId}};

            const auto q2
                = module()->remove("TrackArtists", bindings, QString("Cannot remove track artist %1").arg(artistId));
            if(!q2.hasError()) {
                continue;
            }
            return false;
        }
    }

    for(auto artist : artistsToInsert) {
        const auto bindings = QMap<QString, QVariant>{{"TrackID", id}, {"ArtistID", artist}};

        const auto q2
            = module()->insert("TrackArtists", bindings, QString("Cannot insert track artist %1").arg(artist));
        if(!q2.hasError()) {
            continue;
        }
        return false;
    }
    return true;
}

bool LibraryDatabase::updateTrackGenres(int id, const IdSet& genres)
{
    const auto query = QString("SELECT GenreID FROM TrackGenres WHERE TrackID = %1;").arg(id);

    auto q = Query(module());
    q.prepareQuery(query);

    if(!q.execQuery()) {
        q.error("Could not get all track genres from database");
        return false;
    }

    IdSet databaseGenres;
    IdSet genresToDelete;
    IdSet genresToInsert;

    if(!q.hasError()) {
        while(q.next()) {
            int genreId = q.value(0).toInt();
            databaseGenres.insert(genreId);
        }
    }

    for(auto genreId : databaseGenres) {
        if(genres.contains(genreId)) {
            genresToDelete.insert(genreId);
        }
    }

    for(auto genreId : genres) {
        if(databaseGenres.contains(genreId)) {
            genresToInsert.insert(genreId);
        }
    }

    if(!genresToDelete.empty()) {
        for(auto genreId : genresToDelete) {
            const auto bindings = QList<QPair<QString, QVariant>>{{"TrackID", id}, {"GenreID", genreId}};

            const auto q2
                = module()->remove("TrackGenres", bindings, QString("Cannot remove track genre %1").arg(genreId));
            if(!q2.hasError()) {
                continue;
            }
            return false;
        }
    }

    for(auto genre : genresToInsert) {
        const auto bindings = QMap<QString, QVariant>{{"TrackID", id}, {"GenreID", genre}};

        const auto q3 = module()->insert("TrackGenres", bindings, QString("Cannot insert track genre %1").arg(genre));
        if(!q3.hasError()) {
            continue;
        }
        return false;
    }
    return true;
}

int LibraryDatabase::insertGenre(const QString& genre)
{
    const auto bindings = QMap<QString, QVariant>{
        {"Name", genre},
    };

    auto query = module()->insert("Genres", bindings, QString("Cannot insert genre %1").arg(genre));
    return (query.hasError()) ? -1 : query.lastInsertId().toInt();
}

int LibraryDatabase::insertTrack(const Track& track)
{
    auto bindings = getTrackBindings(track);

    const auto query = module()->insert("Tracks", bindings, QString("Cannot insert track %1").arg(track.filepath()));

    return (query.hasError()) ? -1 : query.lastInsertId().toInt();
}
} // namespace DB
