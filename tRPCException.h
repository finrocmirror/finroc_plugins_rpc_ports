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
/*!\file    plugins/rpc_ports/tRPCException.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-04
 *
 * \brief   Contains tRPCException
 *
 * \b tRPCException
 *
 * Exception possibly thrown when performing synchronous calls on RPC client ports.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tRPCException_h__
#define __plugins__rpc_ports__tRPCException_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <exception>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/definitions.h"

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
//! RPC exception
/*!
 * Exception possibly thrown when performing synchronous calls on RPC client ports.
 * (we do not use different C++ (exception) types, because they are difficult to serialize)
 */
class tRPCException : public std::exception
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCException(tFutureStatus type);

  /*!
   * \return Exception type - reason why exception occured
   */
  tFutureStatus GetType() const
  {
    return type;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Exception type - reason why exception occured */
  tFutureStatus type;

  virtual const char* what() const throw();

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
