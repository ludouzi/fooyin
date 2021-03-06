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

#include "core/player/playermanager.h"
#include "engine.h"

class Track;
class mpv_event;

class EngineMpv : public Engine
{
    Q_OBJECT

public:
    explicit EngineMpv(QObject* parent = nullptr);
    ~EngineMpv() override;

    void engineSetup();
    void processEvents();

    void play() override;
    void stop() override;
    void pause() override;
    void seek(quint64 pos) override;
    void changeTrack(Track* track) override;
    void setVolume(float value) override;

signals:
    void mpvEvent();

protected:
    void handleEvent(mpv_event* event);
    void handlePropertyChange(mpv_event* event);

private:
    struct Private;
    std::unique_ptr<EngineMpv::Private> p;
};
