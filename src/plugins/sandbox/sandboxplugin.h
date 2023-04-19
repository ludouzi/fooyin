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

#include <core/coreplugin.h>
#include <core/plugins/plugin.h>

class QAction;

namespace Fy::Sandbox {
class SandboxDialog;

class SandboxPlugin : public QObject,
                      public Plugins::Plugin,
                      public Core::CorePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.fooyin.plugin" FILE "metadata.json")
    Q_INTERFACES(Fy::Plugins::Plugin)
    Q_INTERFACES(Fy::Core::CorePlugin)

public:
    void initialise(const Core::CorePluginContext& context) override;
    void shutdown() override;

private:
    void openSandbox();

    Utils::ActionManager* m_actionManager;
    Utils::SettingsManager* m_settings;

    QAction* m_showSandbox;

};
} // namespace Fy::Sandbox
