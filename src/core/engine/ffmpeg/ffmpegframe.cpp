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

#include "ffmpegframe.h"

#include "ffmpegutils.h"

#include <QDebug>

namespace Fooyin {
struct Frame::Private : QSharedData
{
    FramePtr frame{nullptr};
    AVRational timeBase;

    explicit Private(FramePtr frame_, AVRational timeBase_)
        : frame{std::move(frame_)}
        , timeBase{std::move(timeBase_)}
    { }
};

Frame::Frame() = default;

Frame::Frame(FramePtr frame, AVRational timeBase)
    : p{new Private(std::move(frame), std::move(timeBase))}
{ }

Frame::Frame(const Frame& other)            = default;
Frame& Frame::operator=(const Frame& other) = default;
Frame::Frame(Frame&& other) noexcept        = default;

Frame::~Frame() = default;

bool Frame::isValid() const
{
    return p && p->frame;
}

AVFrame* Frame::avFrame() const
{
    return isValid() ? p->frame.get() : nullptr;
}

int Frame::channelCount() const
{
    if(!isValid()) {
        return AV_SAMPLE_FMT_NONE;
    }

#if OLD_CHANNEL_LAYOUT
    return p->frame->channels;
#else
    return p->frame->ch_layout.nb_channels;
#endif
}

int Frame::sampleRate() const
{
    return isValid() ? p->frame->sample_rate : 0;
}

AVSampleFormat Frame::format() const
{
    if(!isValid()) {
        return AV_SAMPLE_FMT_NONE;
    }
    return static_cast<AVSampleFormat>(p->frame->format);
}

int Frame::sampleCount() const
{
    return isValid() ? p->frame->nb_samples : 0;
}

uint64_t Frame::ptsMs() const
{
    if(!isValid() || p->frame->pts < 0) {
        return 0;
    }
    return av_rescale_q(p->frame->pts, p->timeBase, {1, 1000});
}

uint64_t Frame::durationMs() const
{
    if(!isValid()) {
        return 0;
    }
#if OLD_FRAME
    return av_rescale_q(p->frame->pkt_duration, p->timeBase, {1, 1000});
#else
    return av_rescale_q(p->frame->duration, p->timeBase, {1, 1000});
#endif
}

uint64_t Frame::end() const
{
    if(!isValid()) {
        return 0;
    }
#if OLD_FRAME
    return p->frame->pts + p->frame->pkt_duration;
#else
    return p->frame->pts + p->frame->duration;
#endif
}
} // namespace Fooyin
