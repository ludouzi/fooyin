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

#include <gui/widgetcontainer.h>

#include <gui/widgetprovider.h>

#include <QJsonArray>
#include <QJsonObject>

namespace Fooyin {
WidgetContainer::WidgetContainer(WidgetProvider* widgetProvider, QWidget* parent)
    : FyWidget{parent}
    , m_widgetProvider{widgetProvider}
{ }

void WidgetContainer::loadWidgets(const QJsonArray& widgets)
{
    for(const auto& widget : widgets) {
        if(!widget.isObject()) {
            continue;
        }
        const QJsonObject widgetObject = widget.toObject();

        const auto widgetName = widgetObject.constBegin().key();

        if(auto* childWidget = m_widgetProvider->createWidget(widgetName)) {
            addWidget(childWidget);
            const auto childValue = widgetObject.value(widgetName);

            if(childValue.isObject()) {
                childWidget->loadLayout(childValue.toObject());
            }

            childWidget->finalise();
        }
    }
}
} // namespace Fooyin

#include "gui/moc_widgetcontainer.cpp"
