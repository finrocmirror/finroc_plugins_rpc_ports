//
// You received this file as part of Finroc
// A Framework for intelligent robot control
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

tRPCPort::tRPCPort(core::tAbstractPortCreationInfo& creation_info, tRPCInterface* call_handler) :
  core::tAbstractPort(creation_info),
  call_handler(call_handler)
{}

tRPCPort::~tRPCPort()
{}


void tRPCPort::ConnectionAdded(tAbstractPort& partner, bool partner_is_destination)
{
  // Disconnect any server ports we might already be connected to.
  for (auto it = this->OutgoingConnectionsBegin(); it != this->OutgoingConnectionsEnd(); ++it)
  {
    if (&(*it) != &partner)
    {
      FINROC_LOG_PRINT_TO(edges, WARNING, "Port was already connected to a server. Removing connection to '", it->GetQualifiedName(), "' and adding the new one to '", partner.GetQualifiedName(), "'.");
      it->DisconnectFrom(*this);
    }
  }
}

tRPCPort* tRPCPort::GetServer(bool include_network_ports) const
{
  tRPCPort* current = const_cast<tRPCPort*>(this);
  while (true)
  {
    const tRPCPort* last = current;
    for (auto it = this->OutgoingConnectionsBegin(); it != this->OutgoingConnectionsEnd(); ++it)
    {
      current = static_cast<tRPCPort*>(&(*it));
      break;
    }

    if (current == NULL || current == last)
    {
      return NULL;
    }

    if (current->IsServer() || (include_network_ports && current->GetFlag(tFlag::NETWORK_ELEMENT)))
    {
      return current;
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
    FINROC_LOG_PRINTF(WARNING, "Both ports (this and %s) are connected to a server already.", other.GetQualifiedLink().c_str());
  }
  else if (server_port_of_this)
  {
    return tConnectDirection::TO_SOURCE;
  }
  else if (server_port_of_other)
  {
    return tConnectDirection::TO_TARGET;
  }

  return tAbstractPort::InferConnectDirection(other);
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
