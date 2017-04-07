//
// You received this file as part of Finroc
// A framework for intelligent robot control
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//----------------------------------------------------------------------
/*!\file    plugins/rpc_ports/internal/tRPCInterfaceTypeInfo.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-02-26
 *
 */
//----------------------------------------------------------------------
#include "plugins/rpc_ports/internal/tRPCInterfaceTypeInfo.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/log_messages.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace finroc
{
namespace rpc_ports
{
namespace internal
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

void tRPCInterfaceTypeInfo::DeserializeMessage(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id) const
{
  if (function_id < methods.size())
  {
    (*methods[function_id].deserialize_message)(stream, port, function_id);
  }
  else
  {
    FINROC_LOG_PRINT(ERROR, "Invalid function id");
  }
}

void tRPCInterfaceTypeInfo::DeserializeRequest(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id, tResponseSender& response_sender) const
{
  if (function_id < methods.size())
  {
    (*methods[function_id].deserialize_request)(stream, port, function_id, response_sender);
  }
  else
  {
    FINROC_LOG_PRINT(ERROR, "Invalid function id");
  }
}

void tRPCInterfaceTypeInfo::DeserializeResponse(rrlib::serialization::tInputStream& stream, uint8_t function_id, tResponseSender& response_sender, tCallStorage* request_storage) const
{
  if (function_id < methods.size())
  {
    (*methods[function_id].deserialize_response)(stream, type, function_id, response_sender, request_storage);
  }
  else
  {
    FINROC_LOG_PRINT(ERROR, "Invalid function id");
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
