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

#include "playlistwidget.h"

#include "gui/guiconstants.h"
#include "gui/guisettings.h"
#include "playlistdelegate.h"
#include "playlistmodel.h"
#include "playlistview.h"

#include <core/library/librarymanager.h>
#include <core/library/musiclibrary.h>
#include <core/player/playermanager.h>
#include <core/playlist/libraryplaylistmanager.h>

#include <utils/actions/actioncontainer.h>
#include <utils/overlaywidget.h>
#include <utils/settings/settingsdialogcontroller.h>
#include <utils/settings/settingsmanager.h>

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>

namespace Fy::Gui::Widgets {
PlaylistWidget::PlaylistWidget(Core::Library::LibraryManager* libraryManager,
                               Core::Playlist::PlaylistManager* playlistHandler,
                               Core::Player::PlayerManager* playerManager, Utils::SettingsManager* settings,
                               QWidget* parent)
    : FyWidget{parent}
    , m_libraryManager{libraryManager}
    , m_library{libraryManager->currentLibrary()}
    , m_playerManager{playerManager}
    , m_settings{settings}
    , m_settingsDialog{settings->settingsDialog()}
    , m_playlistHandler{playlistHandler}
    , m_libraryPlaylistManager{std::make_unique<Core::Playlist::LibraryPlaylistManager>(m_library, playlistHandler)}
    , m_layout{new QHBoxLayout(this)}
    , m_model{new PlaylistModel(m_playerManager, m_library, m_settings, this)}
    , m_playlist{new PlaylistView(this)}
    , m_altRowColours{m_settings->value<Settings::PlaylistAltColours>()}
    , m_changingSelection{false}
    , m_noLibrary{new Utils::OverlayWidget(true, this)}
{
    setObjectName("Playlist");
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_noLibrary->setText(QStringLiteral("No tracks to show - add a library first"));
    m_noLibrary->setButtonText(QStringLiteral("Library Settings"));

    m_playlist->setModel(m_model);
    m_playlist->setItemDelegate(new PlaylistDelegate(this));

    connect(m_noLibrary, &Utils::OverlayWidget::settingsClicked, this, [this] {
        m_settingsDialog->openAtPage(Constants::Page::LibraryGeneral);
    });

    reset();
    setHeaderHidden(m_settings->value<Settings::PlaylistHeader>());
    setScrollbarHidden(m_settings->value<Settings::PlaylistScrollBar>());
    setup();
    setupConnections();
}

void PlaylistWidget::setup()
{
    if(!m_libraryManager->hasLibrary()) {
        m_playlist->hide();
        m_layout->addWidget(m_noLibrary);
        m_noLibrary->show();
        m_noLibrary->raise();
    }
    else {
        m_noLibrary->hide();
        m_layout->addWidget(m_playlist);
        m_playlist->show();
    }
}

void PlaylistWidget::reset()
{
    m_playlist->expandAll();
    m_playlist->scrollToTop();
}

void PlaylistWidget::setupConnections()
{
    connect(m_libraryManager, &Core::Library::LibraryManager::libraryAdded, this, &PlaylistWidget::setup);
    connect(m_libraryManager, &Core::Library::LibraryManager::libraryRemoved, this, &PlaylistWidget::setup);

    m_settings->subscribe<Settings::PlaylistAltColours>(this, &PlaylistWidget::setAltRowColours);
    m_settings->subscribe<Settings::PlaylistAltColours>(m_model, &PlaylistModel::changeRowColours);
    m_settings->subscribe<Settings::SimplePlaylist>(m_model, &PlaylistModel::reset);
    m_settings->subscribe<Settings::SplitDiscs>(m_model, &PlaylistModel::reset);
    m_settings->subscribe<Settings::DiscHeaders>(m_model, &PlaylistModel::reset);
    m_settings->subscribe<Settings::PlaylistHeader>(this, &PlaylistWidget::setHeaderHidden);
    m_settings->subscribe<Settings::PlaylistScrollBar>(this, &PlaylistWidget::setScrollbarHidden);

    connect(m_playlist->header(), &QHeaderView::sectionClicked, this, &PlaylistWidget::switchOrder);

    connect(m_model, &QAbstractItemModel::modelReset, this, &PlaylistWidget::reset);
    connect(m_model, &QAbstractItemModel::modelAboutToBeReset, m_playlist->selectionModel(),
            &QItemSelectionModel::clear);

    connect(m_playlist->header(), &QHeaderView::customContextMenuRequested, this,
            &PlaylistWidget::customHeaderMenuRequested);
    connect(m_playlist->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &PlaylistWidget::selectionChanged);
    connect(m_playlist, &PlaylistView::doubleClicked, this, &PlaylistWidget::playTrack);

    connect(m_playerManager, &Core::Player::PlayerManager::playStateChanged, this, &PlaylistWidget::changeState);
    connect(m_playerManager, &Core::Player::PlayerManager::nextTrack, this, &PlaylistWidget::nextTrack);

    connect(this, &PlaylistWidget::clickedTrack, this, &PlaylistWidget::prepareTracks);

    connect(m_model, &QAbstractItemModel::rowsInserted, this, &PlaylistWidget::expandPlaylist);
}

void PlaylistWidget::setAltRowColours(bool altColours)
{
    m_altRowColours = altColours;
}

bool PlaylistWidget::isHeaderHidden()
{
    return m_playlist->isHeaderHidden();
}

void PlaylistWidget::setHeaderHidden(bool showHeader)
{
    m_playlist->setHeaderHidden(!showHeader);
}

bool PlaylistWidget::isScrollbarHidden()
{
    return m_playlist->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
}

void PlaylistWidget::setScrollbarHidden(bool showScrollBar)
{
    m_playlist->setVerticalScrollBarPolicy(!showScrollBar ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
}

QString PlaylistWidget::name() const
{
    return "Playlist";
}

void PlaylistWidget::layoutEditingMenu(Utils::ActionContainer* menu)
{
    auto* showHeaders = new QAction("Show Header", menu);
    showHeaders->setCheckable(true);
    showHeaders->setChecked(!isHeaderHidden());
    QAction::connect(showHeaders, &QAction::triggered, this, [this] {
        m_settings->set<Settings::PlaylistHeader>(isHeaderHidden());
    });
    menu->addAction(showHeaders);

    auto* showScrollBar = new QAction("Show Scrollbar", menu);
    showScrollBar->setCheckable(true);
    showScrollBar->setChecked(!isScrollbarHidden());
    QAction::connect(showScrollBar, &QAction::triggered, this, [this] {
        m_settings->set<Settings::PlaylistScrollBar>(isScrollbarHidden());
    });
    menu->addAction(showScrollBar);

    auto* altRowColours = new QAction("Alternative Row Colours", menu);
    altRowColours->setCheckable(true);
    altRowColours->setChecked(m_altRowColours);
    QAction::connect(altRowColours, &QAction::triggered, this, [this] {
        m_settings->set<Settings::PlaylistAltColours>(!m_altRowColours);
    });
    menu->addAction(altRowColours);
}

void PlaylistWidget::selectionChanged()
{
    if(m_changingSelection) {
        return;
    }
    m_changingSelection = true;

    const auto selectedIndexes = m_playlist->selectionModel()->selectedIndexes();
    std::deque<QModelIndex> indexes;

    indexes.insert(indexes.end(), selectedIndexes.begin(), selectedIndexes.end());

    while(!indexes.empty()) {
        const auto index = indexes.front();
        indexes.pop_front();
        if(index.isValid()) {
            const auto type = index.data(Playlist::Type).value<PlaylistItem::Type>();
            if(type == PlaylistItem::Track) {
                auto* data = index.data(PlaylistItem::Role::Data).value<Core::Track*>();
                m_selectedTracks.emplace_back(data);
            }
            else {
                const QItemSelection selectedChildren{m_model->index(0, 0, index),
                                                      m_model->index(m_model->rowCount(index) - 1, 0, index)};
                const auto childIndexes = selectedChildren.indexes();
                indexes.insert(indexes.end(), childIndexes.begin(), childIndexes.end());
                m_playlist->selectionModel()->select(selectedChildren, QItemSelectionModel::Select);
            }
        }
    }
    emit selectionWasChanged(m_selectedTracks);
    m_changingSelection = false;
}

void PlaylistWidget::keyPressEvent(QKeyEvent* e)
{
    const auto key = e->key();

    if(key == Qt::Key_Enter || key == Qt::Key_Return) {
        const QModelIndexList indexes = m_playlist->selectionModel()->selectedIndexes();
        if(!indexes.isEmpty()) {
            const QModelIndex index = indexes.constFirst();
            const auto type         = index.data(Playlist::Type).value<PlaylistItem::Type>();

            if(type != PlaylistItem::Track) {
                return;
            }

            auto idx = index.data(PlaylistItem::Role::Index).toInt();

            emit clickedTrack(idx);
            m_model->changeTrackState();
            m_playlist->clearSelection();
        }
    }
    QWidget::keyPressEvent(e);
}

// void PlaylistWidget::spanHeaders(QModelIndex parent)
//{
//     for (int i = 0; i < m_model->rowCount(parent); i++)
//     {
//         auto idx = m_model->index(i, 0, parent);
//         if (m_model->hasChildren(idx))
//         {
//             m_playlist->setFirstColumnSpanned(i, parent, true);
//             spanHeaders(idx);
//         }
//     }
// }

void PlaylistWidget::customHeaderMenuRequested(QPoint pos)
{
    QMenu menu;

    const auto order = m_library->sortOrder();

    QActionGroup sortOrder{&menu};

    QAction yearSort{&menu};
    yearSort.setText("Year");
    yearSort.setData(QVariant::fromValue<Core::Library::SortOrder>(Core::Library::SortOrder::YearDesc));
    yearSort.setCheckable(true);
    yearSort.setChecked(order == Core::Library::SortOrder::YearAsc || order == Core::Library::SortOrder::YearDesc);
    menu.addAction(&yearSort);

    sortOrder.addAction(&yearSort);

    menu.addSeparator();

    menu.setDefaultAction(sortOrder.checkedAction());

    connect(&sortOrder, &QActionGroup::triggered, this, &PlaylistWidget::changeOrder);

    menu.popup(mapToGlobal(pos));
}

void PlaylistWidget::changeOrder(QAction* action)
{
    auto order = action->data().value<Core::Library::SortOrder>();
    m_library->sortTracks(order);
}

void PlaylistWidget::switchOrder()
{
    const auto order = m_library->sortOrder();
    switch(order) {
        case(Core::Library::SortOrder::TitleAsc):
            return m_library->sortTracks(Core::Library::SortOrder::TitleDesc);
        case(Core::Library::SortOrder::TitleDesc):
            return m_library->sortTracks(Core::Library::SortOrder::TitleAsc);
        case(Core::Library::SortOrder::YearAsc):
            return m_library->sortTracks(Core::Library::SortOrder::YearDesc);
        case(Core::Library::SortOrder::YearDesc):
            return m_library->sortTracks(Core::Library::SortOrder::YearAsc);
        case(Core::Library::SortOrder::NoSorting):
            return m_library->sortTracks(Core::Library::SortOrder::TitleAsc);
    }
}

void PlaylistWidget::changeState(Core::Player::PlayState state)
{
    switch(state) {
        case(Core::Player::Playing):
            m_model->changeTrackState();
            return findCurrent();
        case(Core::Player::Stopped):
        case(Core::Player::Paused):
            return m_model->changeTrackState();
    }
}

void PlaylistWidget::playTrack(const QModelIndex& index)
{
    const auto type = index.data(Playlist::Type).value<PlaylistItem::Type>();
    if(type != PlaylistItem::Track) {
        return;
    }

    auto idx = index.data(PlaylistItem::Role::Index).toInt();

    emit clickedTrack(idx);
    m_model->changeTrackState();
    m_playlist->clearSelection();
}

void PlaylistWidget::nextTrack()
{
    m_model->changeTrackState();
}

void PlaylistWidget::findCurrent()
{
    //    const auto* track = m_playerManager->currentTrack();

    //    if(!track) {
    //        return;
    //    }

    //    QModelIndex index;
    //    index = m_>model->match({}, ItemRole::Id, track->id(), 1, {});
    //    index = m_model->indexForId(track->id());

    //    if(index.isValid()) {
    //        m_playlist->scrollTo(index);
    //        setCurrentIndex(index.constFirst());
    //    }
}

void PlaylistWidget::prepareTracks(int idx)
{
    m_libraryPlaylistManager->createPlaylist(m_library->tracks(), idx);
}

void PlaylistWidget::expandPlaylist(const QModelIndex& parent, int first, int last)
{
    while(first <= last) {
        m_playlist->expand(m_model->index(first, 0, parent));
        ++first;
    }
}
} // namespace Fy::Gui::Widgets
