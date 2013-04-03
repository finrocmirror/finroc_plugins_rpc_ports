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
/*!\file    plugins/rpc_ports/internal/tRPCInterfaceTypeInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-02-26
 *
 * \brief   Contains tRPCInterfaceTypeInfo
 *
 * \b tRPCInterfaceTypeInfo
 *
 * Information on RPC interface type.
 * Contains function pointers to deserialization functions.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tRPCInterfaceTypeInfo_h__
#define __plugins__rpc_ports__internal__tRPCInterfaceTypeInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/rtti/tTypeAnnotation.h"

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

template <typename T>
class tRPCInterfaceType;

namespace internal
{

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! RPC interface type info
/*!
 * Information on RPC interface type.
 * Contains function pointers to deserialization functions.
 */
class tRPCInterfaceTypeInfo : public rrlib::rtti::tTypeAnnotation
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCInterfaceTypeInfo();

  /*!
   * Deserializes message
   */
  void DeserializeMessage(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id);

  /*!
   * Deserializes request
   */
  void DeserializeRequest(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id, tResponseSender& response_sender);

  /*!
   * Deserializes response
   */
  void DeserializeResponse(rrlib::serialization::tInputStream& stream, uint8_t function_id, tResponseSender& response_sender, tCallStorage* request_storage);


//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename T>
  friend class finroc::rpc_ports::tRPCInterfaceType;

  /*!
   * One such type-less entry exists for every registered method
   */
  struct tEntry
  {
    /*! function to deserialize this method from stream */
    internal::tDeserializeMessage deserialize_message;
    internal::tDeserializeRequest deserialize_request;
    internal::tDeserializeResponse deserialize_response;
  };

  /*!
   * Methods registered in this interface type
   */
  std::vector<tEntry> methods;

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
