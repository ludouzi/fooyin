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

#include <QWidget>

class Track;

class ProgressWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressWidget(QWidget* parent = nullptr);
    ~ProgressWidget() override;

    void setupUi();

    void changeTrack(Track* track);
    void setCurrentPosition(int ms);
    void updateTime(int elapsed);
    void reset();
    void stateChanged(Player::PlayState state);

signals:
    void movedSlider(int pos);

protected:
    void toggleRemaining();
    void sliderDropped();

private:
    struct Private;
    std::unique_ptr<ProgressWidget::Private> p;
};
