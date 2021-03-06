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

#include <QComboBox>
#include <QTableWidget>
#include <QWidget>

namespace Library {
class LibraryManager;
class LibraryInfo;
} // namespace Library

class GeneralPage : public QWidget
{
public:
    explicit GeneralPage(QWidget* parent = nullptr);
    ~GeneralPage() override;

private:
    QComboBox m_deviceList;
};

class LibraryPage : public QWidget
{
public:
    explicit LibraryPage(Library::LibraryManager* libraryManager, QWidget* parent = nullptr);
    ~LibraryPage() override;

private:
    void addLibraryRow(const Library::LibraryInfo& info);
    void addLibrary();
    void removeLibrary();

    Library::LibraryManager* m_libraryManager;
    QTableWidget m_libraryList;
};

class PlaylistPage : public QWidget
{
public:
    explicit PlaylistPage(QWidget* parent = nullptr);
    ~PlaylistPage() override;
};
