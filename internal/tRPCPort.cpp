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
/*!\file    plugins/rpc_ports/internal/tRPCPort.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 */
//----------------------------------------------------------------------
#include "plugins/rpc_ports/internal/tRPCPort.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tPortFactory.h"

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

class tRpcPortFactory : public core::tPortFactory
{
  virtual core::tAbstractPort& CreatePortImplementation(const std::string& port_name, core::tFrameworkElement& parent,
      const rrlib::rtti::tType& type, core::tFrameworkElement::tFlags flags) override
  {
    core::tAbstractPortCreationInfo creation_info;
    creation_info.flags = flags | core::tFrameworkElement::tFlag::ACCEPTS_DATA | core::tFrameworkElement::tFlag::EMITS_DATA;
    creation_info.data_type = type;
    creation_info.parent = &parent;
    creation_info.name = port_name;
    return *(new tRPCPort(creation_info, nullptr));
  }

  virtual bool HandlesDataType(const rrlib::rtti::tType& type) override
  {
    return IsRPCType(type);
  }
};

tRpcPortFactory default_rpc_port_factory;

static core::tAbstractPortCreationInfo ProcessPortCreationInfo(core::tAbstractPortCreationInfo& info)
{
  info.flags.Set(core::tFrameworkElement::tFlag::PUSH_STRATEGY, false); // unsets push strategy flag so that port is not erroneously identified as data port
  return info;
}


tRPCPort::tRPCPort(core::tAbstractPortCreationInfo creation_info, tRPCInterface* call_handler) :
  core::tAbstractPort(ProcessPortCreationInfo(creation_info)),
  call_handler(call_handler)
{}

tRPCPort::~tRPCPort()
{}


tRPCPort* tRPCPort::GetServer(bool include_network_ports) const
{
  tRPCPort* current = const_cast<tRPCPort*>(this);
  while (true)
  {
    if (current->IsServer() || (include_network_ports && current->GetFlag(tFlag::NETWORK_ELEMENT)))
    {
      return current;
    }

    const tRPCPort* last = current;
    for (auto it = last->OutgoingConnectionsBegin(); it != last->OutgoingConnectionsEnd(); ++it)
    {
      current = &static_cast<tRPCPort&>(it->Destination());
      break;
    }

    if (current == nullptr || current == last)
    {
      return nullptr;
    }
  }
}

core::tAbstractPort::tConnectDirection tRPCPort::InferConnectDirection(const tAbstractPort& other) const
{
  // Check whether one of the two ports is connected to a server
  const tRPCPort& other_interface_port = static_cast<const tRPCPort&>(other);
  const tRPCPort* server_port_of_this = (IsServer() || GetFlag(tFlag::NETWORK_ELEMENT)) ? this : GetServer();
  const tRPCPort* server_port_of_other = (other_interface_port.IsServer() || other_interface_port.GetFlag(tFlag::NETWORK_ELEMENT)) ? &other_interface_port : other_interface_port.GetServer();
  if (server_port_of_this && server_port_of_other)
  {
    FINROC_LOG_PRINT(WARNING, "Both ports (this and '", other, "') are connected to a server already.");
  }
  else if (server_port_of_this)
  {
    return tConnectDirection::TO_SOURCE;
  }
  else if (server_port_of_other)
  {
    return tConnectDirection::TO_DESTINATION;
  }

  return tAbstractPort::InferConnectDirection(other);
}

void tRPCPort::OnConnect(tAbstractPort& partner, bool partner_is_destination)
{
  // Disconnect any server ports we might already be connected to.
  if (partner_is_destination)
  {
    for (auto it = this->OutgoingConnectionsBegin(); it != this->OutgoingConnectionsEnd(); ++it)
    {
      if (&it->Destination() != &partner)
      {
        FINROC_LOG_PRINT_TO(edges, WARNING, "Port was already connected to a server. Removing connection to '", it->Destination(), "' and adding the new one to '", partner, "'.");
        it->Disconnect();
      }
    }
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
