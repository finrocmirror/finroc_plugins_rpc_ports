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
/*!\file    plugins/rpc_ports/tResponseHandler.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-08
 *
 * \brief   Contains tResponseHandler
 *
 * \b tResponseHandler
 *
 * Handles results that are returned by RPC calls.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tResponseHandler_h__
#define __plugins__rpc_ports__tResponseHandler_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <boost/noncopyable.hpp>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/definitions.h"
#include "plugins/rpc_ports/internal/tAbstractResponseHandler.h"

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace finroc
{
namespace rpc_ports
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! RPC call reponse handler
/*!
 * Handles results that are returned by RPC calls.
 */
template <typename T>
class tResponseHandler : public internal::tAbstractResponseHandler
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Called whenever a synchronous RPC caused an exception
   *
   * \param exception_type Type of error that occured
   */
  virtual void HandleException(tFutureStatus exception_type) = 0;

  /*!
   * Called when a result of an RPC call is received
   *
   * \param call_result The result of the call
   */
  virtual void HandleResponse(T& call_result) = 0;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
