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
/*!\file    plugins/rpc_ports/internal/tAbstractCall.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-04
 *
 * \brief   Contains tAbstractCall
 *
 * \b tAbstractCall
 *
 * This is the base class for all "calls" (requests, responses, pull calls)
 * For calls within the same runtime environment they are not required.
 * They are used to temporarily store such calls in queues for network threads
 * and to serialize calls.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tAbstractCall_h__
#define __plugins__rpc_ports__internal__tAbstractCall_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/serialization/serialization.h"

//----------------------------------------------------------------------
// Internal includes with ""
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
class tRPCPort;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Base class for "calls"
/*!
 * This is the base class for all "calls" (requests, responses, pull calls)
 * For calls within the same runtime environment they are not required.
 * They are used to temporarily store such calls in queues for network threads
 * and to serialize calls.
 */
class tAbstractCall : public boost::noncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tAbstractCall();

  virtual ~tAbstractCall();

  /*!
   * Deserializes/receives return value from stream
   */
  virtual void ReturnValue(rrlib::serialization::tInputStream& stream, tRPCPort& port)
  {
    throw new std::runtime_error("This is a call without return value");
  }

  /*!
   * Serializes call to stream
   */
  virtual void Serialize(rrlib::serialization::tOutputStream& stream) = 0;

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
}


#endif
