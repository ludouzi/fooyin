/*
 * Fooyin
 * Copyright © 2024, Luke Taylor <LukeT1@proton.me>
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

#include <utils/timer.h>

namespace Fooyin {
void Timer::reset()
{
    m_start = Clock::now();
}

std::chrono::milliseconds Timer::elapsed() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - m_start);
}

std::string Timer::elapsedFormatted() const
{
    auto elapsedMs     = elapsed();
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsedMs);
    elapsedMs -= minutes;
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedMs);
    elapsedMs -= seconds;
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedMs);
    const auto microseconds = elapsedMs - milliseconds;

    return std::format("{:01}:{:02}.{:03}{:03}", minutes.count(), seconds.count(), milliseconds.count(),
                       microseconds.count());
}
} // namespace Fooyin