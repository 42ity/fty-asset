/*  ========================================================================
    Copyright (C) 2020 Eaton
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    ========================================================================
*/

#include "asset/asset-notifications.h"
#include "asset/asset-configure-inform.h"
#include "messagebus/message-bus.h"
#include "messagebus/message.h"

namespace fty::asset {

Expected<void> sendStreamNotification(const std::string& stream, const std::string& subject, const std::string& payload)
{
    const std::string agentName{generateMlmClientId("asset.notify")};

    Message notification;
    notification.meta.from    = agentName;
    notification.meta.subject = subject;
    notification.meta.status  = Message::Status::Ok;
    notification.userData.append(payload);

    MessageBus bus;
    if (auto ret = bus.init(agentName); !ret) {
        return unexpected(ret.error());
    }

    if (auto ret = bus.publish(stream, notification); !ret) {
        return unexpected(ret.error());
    }

    return {};
}

} // namespace fty::asset
