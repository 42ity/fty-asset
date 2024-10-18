/*  ====================================================================================================================
    Copyright (C) 2014 - 2020 Eaton

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
    ====================================================================================================================
*/

#pragma once

#include <fty/expected.h>
#include <fty/translate.h>

namespace fty::asset {

template <typename T>
using AssetExpected = Expected<T, Translate>;

enum class Errors
{
    BadParams,
    InternalError,
    ParamRequired,
    BadRequestDocument,
    ActionForbidden,
    ElementNotFound,
    ElementAlreadyExist,
    ExceptionForElement
};

inline Translate error(Errors err)
{
    switch (err) {
    case Errors::BadParams:
        return "Parameter '{}' has bad value. Received {}. Expected {}."_tr;
    case Errors::InternalError:
        return "Internal Server Error. {}."_tr;
    case Errors::ParamRequired:
        return "Parameter '{}' is required."_tr;
    case Errors::BadRequestDocument:
        return "Request document has invalid '{}' syntax."_tr;
    case Errors::ActionForbidden:
        return "{} is forbidden: {}"_tr;
    case Errors::ElementNotFound:
        return "Element '{}' not found."_tr;
    case Errors::ElementAlreadyExist:
        return "Element '{}' already exist."_tr;
    case Errors::ExceptionForElement:
        return "exception caught {} for element '{}'"_tr;
    }
    return "Unknown error"_tr;
}

} // namespace fty::asset
