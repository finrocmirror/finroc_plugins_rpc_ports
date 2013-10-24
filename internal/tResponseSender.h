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
/*!\file    plugins/rpc_ports/internal/tResponseSender.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-02-26
 *
 * \brief   Contains tResponseSender
 *
 * \b tResponseSender
 *
 * Sends response RPC calls back to a caller.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tResponseSender_h__
#define __plugins__rpc_ports__internal__tResponseSender_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/internal/tRPCPort.h"

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
// Class declaration
//----------------------------------------------------------------------
//! Sends response calls
/*!
 * Sends response RPC calls back to a caller.
 */
class tResponseSender
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef tRPCPort::tCallPointer tCallPointer;

  /*!
   * Called with any responses that need to be returned to caller
   *
   * \param response_to_send Response to send back to caller
   */
  void SendResponse(typename tCallStorage::tPointer& response_to_send)
  {
    assert(tRPCPort::IsFuturePointer(*response_to_send) == false);
    SendResponse(tCallPointer(response_to_send.release()));
  }
  void SendResponse(typename tCallStorage::tFuturePointer && response_to_send)
  {
    assert(tRPCPort::IsFuturePointer(*response_to_send) == true);
    SendResponse(tCallPointer(response_to_send.release()));
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*!
   * Called with any responses that need to be returned to caller
   *
   * \param response_to_send Response to send back to caller
   */
  virtual void SendResponse(tCallPointer && response_to_send) = 0;

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
