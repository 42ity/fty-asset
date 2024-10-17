/*
 *
 * Copyright (C) 2021 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#pragma once

#include "message.h"
#include <functional>
#include <memory>
#include <mutex>

// =====================================================================================================================

namespace messagebus {
class MessageBus;
class Message;
} // namespace messagebus

// =====================================================================================================================

namespace fty::asset {

/// Common message bus temporary wrapper
class MessageBus
{
public:
    static constexpr const char* endpoint = "ipc://@/malamute";

public:
    MessageBus();
    ~MessageBus();

    [[nodiscard]] Expected<void> init(const std::string& actorName);

    [[nodiscard]] Expected<Message> send(const std::string& queue, const Message& msg);
    [[nodiscard]] Expected<void>    publish(const std::string& queue, const Message& msg);
    [[nodiscard]] Expected<void>    reply(const std::string& queue, const Message& req, const Message& answ);
    [[nodiscard]] Expected<Message> receive(const std::string& queue);

    template <typename Func, typename Cls>
    [[nodiscard]] Expected<void> subscribe(const std::string& queue, Func&& fnc, Cls* cls)
    {
        return subscribe(queue, [f = std::move(fnc), c = cls](const messagebus::Message& msg) -> void {
            std::invoke(f, *c, Message(msg));
        });
    }

private:
    Expected<void> subscribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& func);

private:
    std::unique_ptr<messagebus::MessageBus> m_bus{nullptr};
    std::mutex                              m_mutex;
    std::string                             m_actorName;
};

} // namespace fty::asset
