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

#include "tabstackwidget.h"

#include <core/constants.h>
#include <gui/widgetprovider.h>
#include <utils/actions/actioncontainer.h>
#include <utils/actions/actionmanager.h>
#include <utils/enum.h>
#include <utils/widgets/editabletabbar.h>
#include <utils/widgets/editabletabwidget.h>

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QStyleOptionViewItem>
#include <QTabBar>
#include <QVBoxLayout>

namespace Fooyin {
TabStackWidget::TabStackWidget(ActionManager* actionManager, WidgetProvider* widgetProvider, QWidget* parent)
    : WidgetContainer{widgetProvider, parent}
    , m_actionManager{actionManager}
    , m_widgetProvider{widgetProvider}
    , m_tabs{new EditableTabWidget(this)}
{
    QObject::setObjectName(TabStackWidget::name());

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(m_tabs);

    QObject::connect(m_tabs->tabBar(), &QTabBar::tabMoved, this, [this](int from, int to) {
        if(from >= 0 && from < static_cast<int>(m_widgets.size())) {
            auto* widget = m_widgets.at(from);
            m_widgets.erase(m_widgets.begin() + from);
            m_widgets.insert(m_widgets.begin() + to, widget);
        }
    });
}

QString TabStackWidget::name() const
{
    return QStringLiteral("Tab Stack");
}

QString TabStackWidget::layoutName() const
{
    return QStringLiteral("TabStack");
}

void TabStackWidget::layoutEditingMenu(ActionContainer* menu)
{
    const QString addTitle{tr("&Add")};
    auto addMenuId = id().append(addTitle);

    auto* addMenu = m_actionManager->createMenu(addMenuId);
    addMenu->menu()->setTitle(addTitle);

    m_widgetProvider->setupWidgetMenu(addMenu, [this](FyWidget* newWidget) { addWidget(newWidget); });
    menu->addMenu(addMenu);
}

void TabStackWidget::saveLayoutData(QJsonObject& layout)
{
    QJsonArray widgets;
    for(FyWidget* widget : m_widgets) {
        widget->saveLayout(widgets);
    }

    QString state;
    for(int i{0}; i < m_tabs->count(); ++i) {
        if(!state.isEmpty()) {
            state.append(u"\037");
        }
        state.append(m_tabs->tabText(i));
    }

    layout[QStringLiteral("Position")] = Utils::Enum::toString(m_tabs->tabPosition());
    layout[QStringLiteral("State")]    = state;
    layout[QStringLiteral("Widgets")]  = widgets;
}

void TabStackWidget::loadLayoutData(const QJsonObject& layout)
{
    if(const auto position
       = Utils::Enum::fromString<QTabWidget::TabPosition>(layout.value(QStringLiteral("Position")).toString())) {
        m_tabs->setTabPosition(position.value());
    }

    const auto widgets = layout.value(QStringLiteral("Widgets")).toArray();

    WidgetContainer::loadWidgets(widgets);

    const auto state         = layout.value(QStringLiteral("State")).toString();
    const QStringList titles = state.split(QStringLiteral("\037"));

    for(int i{0}; const QString& title : titles) {
        if(i < m_tabs->count()) {
            m_tabs->setTabText(i++, title);
        }
    }
}

void TabStackWidget::addWidget(FyWidget* widget)
{
    const int index = m_tabs->addTab(widget, widget->name());
    m_widgets.insert(m_widgets.begin() + index, widget);
}

void TabStackWidget::removeWidget(FyWidget* widget)
{
    const int index = indexOfWidget(widget);
    if(index >= 0) {
        m_tabs->removeTab(index);
        m_widgets.erase(m_widgets.begin() + index);
    }
}

void TabStackWidget::replaceWidget(FyWidget* oldWidget, FyWidget* newWidget)
{
    const int index = indexOfWidget(oldWidget);
    if(index >= 0) {
        m_tabs->removeTab(index);
        m_tabs->insertTab(index, newWidget, newWidget->name());

        m_widgets.erase(m_widgets.begin() + index);
        m_widgets.insert(m_widgets.begin() + index, newWidget);

        m_tabs->setCurrentIndex(index);
    }
}

WidgetList TabStackWidget::widgets() const
{
    return m_widgets;
}

void TabStackWidget::contextMenuEvent(QContextMenuEvent* event)
{
    auto* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    const QPoint point = m_tabs->tabBar()->mapFrom(this, event->pos());
    const int index    = m_tabs->tabBar()->tabAt(point);

    if(index < 0 || index >= m_tabs->count()) {
        FyWidget::contextMenuEvent(event);
        return;
    }

    auto* widget = m_widgets.at(index);

    auto* posMenu = new QMenu(tr("&Position"), menu);

    auto* positionGroup = new QActionGroup(menu);

    auto* north = new QAction(tr("&North"), posMenu);
    auto* east  = new QAction(tr("&East"), posMenu);
    auto* south = new QAction(tr("&South"), posMenu);
    auto* west  = new QAction(tr("&West"), posMenu);

    north->setCheckable(true);
    east->setCheckable(true);
    south->setCheckable(true);
    west->setCheckable(true);

    const auto tabPos = m_tabs->tabPosition();

    switch(tabPos) {
        case(QTabWidget::North):
            north->setChecked(true);
            break;
        case(QTabWidget::East):
            east->setChecked(true);
            break;
        case(QTabWidget::South):
            south->setChecked(true);
            break;
        case(QTabWidget::West):
            west->setChecked(true);
            break;
    }

    QObject::connect(north, &QAction::triggered, this, [this]() { changeTabPosition(QTabWidget::North); });
    QObject::connect(east, &QAction::triggered, this, [this]() { changeTabPosition(QTabWidget::East); });
    QObject::connect(south, &QAction::triggered, this, [this]() { changeTabPosition(QTabWidget::South); });
    QObject::connect(west, &QAction::triggered, this, [this]() { changeTabPosition(QTabWidget::West); });

    positionGroup->addAction(north);
    positionGroup->addAction(east);
    positionGroup->addAction(south);
    positionGroup->addAction(west);

    posMenu->addAction(north);
    posMenu->addAction(east);
    posMenu->addAction(south);
    posMenu->addAction(west);

    auto* rename = new QAction(tr("&Rename"), menu);
    QObject::connect(rename, &QAction::triggered, m_tabs->editableTabBar(), &EditableTabBar::showEditor);

    auto* remove = new QAction(tr("Re&move"), menu);
    QObject::connect(remove, &QAction::triggered, this, [this, widget]() { removeWidget(widget); });

    menu->addMenu(posMenu);
    menu->addSeparator();
    menu->addAction(rename);
    menu->addAction(remove);

    menu->popup(m_tabs->tabBar()->mapToGlobal(point));
}

int TabStackWidget::indexOfWidget(FyWidget* widget) const
{
    auto it = std::ranges::find(m_widgets, widget);
    if(it != m_widgets.cend()) {
        return static_cast<int>(std::distance(m_widgets.cbegin(), it));
    }
    return -1;
}

void TabStackWidget::changeTabPosition(QTabWidget::TabPosition position) const
{
    m_tabs->setTabPosition(position);
    m_tabs->adjustSize();
}
} // namespace Fooyin

#include "moc_tabstackwidget.cpp"
