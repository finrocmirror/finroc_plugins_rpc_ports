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
/*!\file    plugins/rpc_ports/definitions.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-09
 *
 * \brief
 *
 * Definitions relevant for RPC pluginsdf asdfg asdf asdf a
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__definitions_h__
#define __plugins__rpc_ports__definitions_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/rtti/rtti.h"

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

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

/*!
 * Status of call a future is waiting for
 */
enum class tFutureStatus
{
  PENDING, //!< value is yet to be returned
  READY,   //!< value is ready and can be obtained

  // Exceptions
  NO_CONNECTION,         //!< There is no server port connected to client port
  TIMEOUT,               //!< Call timed out
  BROKEN_PROMISE,        //!< Promise was destructed and did not provide any value before
  INVALID_FUTURE,        //!< Called on an invalid future object
  INTERNAL_ERROR,        //!< Internal error; if this occurs, there is a bug in the finroc implementation
  INVALID_CALL,          //!< Function was called that was not allowed
  INVALID_DATA_RECEIVED  //!< Invalid data received from other process (via network)
};

/*!
 * Types of RPC calls
 */
enum class tCallType
{
  RPC_MESSAGE,
  RPC_REQUEST,
  RPC_RESPONSE,
  UNSPECIFIED
};

/*!
 * \param type Data type to check
 * \return Is specified data type a RPC interface type?
 */
inline bool IsRPCType(const rrlib::rtti::tType& type)
{
  return (type.GetSize() == 0) && (type.GetType() == rrlib::rtti::tType::tClassification::OTHER);
}


namespace internal
{
class tRPCPort;
class tCallStorage;
class tResponseSender;

/*!
 * Call id that is attached to requests and responses in order to identify
 * the requests that response belongs to
 */
typedef uint64_t tCallId;

/*! Function that deserializes and executes message from stream */
typedef void (*tDeserializeMessage)(rrlib::serialization::tInputStream&, tRPCPort&, uint8_t);

/*! Function that deserializes and executes call from stream */
typedef void (*tDeserializeRequest)(rrlib::serialization::tInputStream&, tRPCPort&, uint8_t, tResponseSender& response_sender);

/*! Function that deserializes and handles response from stream */
typedef void (*tDeserializeResponse)(rrlib::serialization::tInputStream&, const rrlib::rtti::tType&, uint8_t, tResponseSender&, tCallStorage*);

} // namespace internal


//----------------------------------------------------------------------
// Function declarations
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
