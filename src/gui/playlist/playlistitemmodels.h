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

#include "playlistpreset.h"

#include <core/models/trackfwd.h>

namespace Fy::Gui::Widgets::Playlist {
class Container
{
public:
    Container();
    virtual ~Container() = default;

    [[nodiscard]] Core::TrackList tracks() const;
    [[nodiscard]] int trackCount() const;
    [[nodiscard]] uint64_t duration() const;

    virtual void addTrack(const Core::Track& track);
    virtual void removeTrack(const Core::Track& trackToRemove);

private:
    Core::TrackList m_tracks;
    uint64_t m_duration;
};
using ContainerHashMap = std::unordered_map<QString, std::unique_ptr<Container>>;

class Header : public Container
{
public:
    Header(TextBlock title, TextBlock subtitle, TextBlock sideText, TextBlock info, QString coverPath);

    [[nodiscard]] TextBlock title() const;
    [[nodiscard]] TextBlock subtitle() const;
    [[nodiscard]] TextBlock sideText() const;
    [[nodiscard]] TextBlock info() const;

    [[nodiscard]] bool hasCover() const;
    [[nodiscard]] QString coverPath() const;

    void addTrack(const Core::Track &track) override;
    void removeTrack(const Core::Track &trackToRemove) override;

private:
    void calculateInfo();

    TextBlock m_title;
    TextBlock m_subtitle;
    TextBlock m_sideText;
    TextBlock m_info;

    QStringList m_genres;

    QString m_coverPath;
};

class Subheader : public Container
{
public:
    Subheader(TextBlockList title, TextBlockList subtitle);

    [[nodiscard]] TextBlockList title() const;
    [[nodiscard]] TextBlockList subtitle() const;

private:
    TextBlockList m_title;
    TextBlockList m_subtitle;
};

class Track
{
public:
    Track() = default;
    Track(TextBlockList left, TextBlockList right, const Core::Track& track);

    [[nodiscard]] TextBlockList left() const;
    [[nodiscard]] TextBlockList right() const;
    [[nodiscard]] Core::Track track() const;

private:
    TextBlockList m_left;
    TextBlockList m_right;

    Core::Track m_track;
};
} // namespace Fy::Gui::Widgets::Playlist
