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

#include "splitterwidget.h"

#include "dummy.h"
#include "splitter.h"
#include "utils/enumhelper.h"
#include "utils/utils.h"
#include "utils/widgetprovider.h"

#include <QJsonObject>
#include <QMenu>

static const bool m_isRegistered = Util::factory()->registerClass<SplitterWidget>("Splitter", {"Splitters"});

SplitterWidget::SplitterWidget(WidgetProvider* widgetProvider, QWidget* parent)
    : Widget(parent)
    , m_layout(new QHBoxLayout(this))
    , m_splitter(new Splitter(Qt::Vertical))
    , m_widgetProvider(widgetProvider)
    , m_dummy(new Dummy(m_widgetProvider, this))
{
    setObjectName(QString("%1 Splitter").arg(orientation() == Qt::Horizontal ? "Horizontal" : "Vertical"));

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_splitter);

    m_splitter->addWidget(m_dummy);

    if(!m_isRegistered) {
        qDebug() << SplitterWidget::name() << " not registered";
    }
}

SplitterWidget::~SplitterWidget() = default;

Qt::Orientation SplitterWidget::orientation() const
{
    return m_splitter->orientation();
}

void SplitterWidget::setOrientation(Qt::Orientation orientation)
{
    m_splitter->setOrientation(orientation);
}

QByteArray SplitterWidget::saveState() const
{
    return m_splitter->saveState();
}

bool SplitterWidget::restoreState(const QByteArray& state)
{
    return m_splitter->restoreState(state);
}

QWidget* SplitterWidget::widget(int index) const
{
    return m_splitter->widget(index);
}

bool SplitterWidget::hasChildren()
{
    return !m_children.isEmpty();
}

void SplitterWidget::addWidget(QWidget* newWidget)
{
    auto* widget = qobject_cast<Widget*>(newWidget);
    if(!widget) {
        return;
    }
    if(m_children.isEmpty()) {
        m_dummy->hide();
    }
    int index = static_cast<int>(m_children.count());
    m_children.append(widget);
    return m_splitter->insertWidget(index, widget);
}

void SplitterWidget::removeWidget(QWidget* widget)
{
    int index = findIndex(widget);
    if(index != -1) {
        widget->deleteLater();
        m_children.remove(index);
    }
    if(m_children.isEmpty()) {
        m_dummy->show();
    }
}

int SplitterWidget::findIndex(QWidget* widgetToFind)
{
    for(int i = 0; i < m_children.size(); ++i) {
        QWidget* widget = m_children.value(i);
        if(widget == widgetToFind) {
            return i;
        }
    }
    return -1;
}

QList<Widget*> SplitterWidget::children()
{
    return m_children;
}

QString SplitterWidget::name() const
{
    return "Splitter";
}

void SplitterWidget::layoutEditingMenu(QMenu* menu)
{
    auto* addMenu = new QMenu("Add", this);
    auto* changeSplitter = new QAction("Change Splitter", this);

    QAction::connect(changeSplitter, &QAction::triggered, this, [this] {
        setOrientation(m_splitter->orientation() == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal);
        setObjectName(QString("%1 Splitter").arg(EnumHelper::toString(m_splitter->orientation())));
    });

    m_widgetProvider->addMenuActions(addMenu, this);

    menu->addAction(changeSplitter);
    menu->addMenu(addMenu);
}

void SplitterWidget::saveSplitter(QJsonObject& object, QJsonArray& splitterArray)
{
    QJsonArray array;

    for(const auto& widget : children()) {
        auto* childSplitter = qobject_cast<SplitterWidget*>(widget);
        if(childSplitter) {
            childSplitter->saveSplitter(object, array);
        }
        else {
            widget->saveLayout(array);
        }
    }
    QString state = QString::fromUtf8(saveState().toBase64());

    QJsonObject children;
    children["Type"] = EnumHelper::toString(orientation());
    children["State"] = state;
    children["Children"] = array;

    if(!findParent()) {
        object["Splitter"] = children;
    }
    else {
        QJsonObject object;
        object["Splitter"] = children;
        splitterArray.append(object);
    }
}

void SplitterWidget::loadSplitter(const QJsonArray& array, SplitterWidget* splitter)
{
    for(const auto& widget : array) {
        QJsonObject object = widget.toObject();
        if(object.contains("Splitter")) {
            QJsonObject childSplitterObject = object["Splitter"].toObject();
            auto type = EnumHelper::fromString<Qt::Orientation>(childSplitterObject["Type"].toString());
            //            auto widgetType = type == Qt::Vertical ? Widgets::WidgetType::VerticalSplitter
            //                                                   : Widgets::WidgetType::HorizontalSplitter;

            QJsonArray splitterArray = childSplitterObject["Children"].toArray();
            QByteArray splitterState = QByteArray::fromBase64(childSplitterObject["State"].toString().toUtf8());

            auto* childSplitter = m_widgetProvider->createSplitter(type, this);
            splitter->addWidget(childSplitter);
            loadSplitter(splitterArray, childSplitter);
            childSplitter->restoreState(splitterState);
        }
        else if(object.contains("Filter")) {
            const QJsonObject filterObject = object["Filter"].toObject();
            auto filterType = EnumHelper::fromString<Filters::FilterType>(filterObject["Type"].toString());
            m_widgetProvider->createFilter(filterType, splitter);
        }
        else {
            m_widgetProvider->createWidget(widget.toString(), splitter);
        }
    }
}
