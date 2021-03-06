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

#include <QJsonArray>
#include <QWidget>

class QMenu;

// Subclassing requires the inheritence of FactoryRegister<T> so widget is detected
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* parent);
    ~Widget() override;

    Widget* findParent();

    [[nodiscard]] virtual QString name() const = 0;
    virtual void addWidgetMenu(QMenu* menu, QAction* action);
    virtual void layoutEditingMenu(QMenu* menu);
    virtual void saveLayout(QJsonArray& array);
};
