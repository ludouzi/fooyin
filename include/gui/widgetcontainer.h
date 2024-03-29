/*
 * Fooyin
 * Copyright © 2023, Luke Taylor <LukeT1@proton.me>
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

#include <gui/fywidget.h>

namespace Fooyin {
class WidgetProvider;

/*!
 * Represents a container of FyWidgets.
 * This should be used as the base class for FyWidgets which can contain/hold other
 * FyWidgets.
 * It's recommended to save the child widgets under the 'Widgets' key
 * when reimplementing @fn saveLayoutData.
 */
class WidgetContainer : public FyWidget
{
    Q_OBJECT

public:
    explicit WidgetContainer(WidgetProvider* widgetProvider, QWidget* parent = nullptr);

    virtual void addWidget(FyWidget* widget)                             = 0;
    virtual void removeWidget(FyWidget* widget)                          = 0;
    virtual void replaceWidget(FyWidget* oldWidget, FyWidget* newWidget) = 0;

    virtual WidgetList widgets() const = 0;

    /*!
     * Convenience method to load all widgets in the @p widgets array.
     */
    void loadWidgets(const QJsonArray& widgets);

private:
    WidgetProvider* m_widgetProvider;
};
} // namespace Fooyin
