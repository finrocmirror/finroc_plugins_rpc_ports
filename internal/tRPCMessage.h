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
/*!\file    plugins/rpc_ports/internal/tRPCMessage.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-08
 *
 * \brief   Contains tRPCMessage
 *
 * \b tRPCMessage
 *
 * This class stores and handles synchronous RPC calls that do not
 * return any value.
 * For calls within the same runtime environment this class is not required.
 * Objects of this class are used to temporarily store such calls in queues
 * for network threads and to serialize them.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tRPCMessage_h__
#define __plugins__rpc_ports__internal__tRPCMessage_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/util/tIntegerSequence.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/internal/tAbstractCall.h"

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
class tClientPort;

template <typename T>
class tRPCInterfaceType;

namespace internal
{

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! RPC message
/*!
 * This class stores and handles RPC calls that do not return any value.
 * For calls within the same runtime environment this class is not required.
 * Objects of this class are used to temporarily store such calls in queues
 * for network threads and to serialize them.
 */
template <typename ... TArgs>
class tRPCMessage : public tAbstractCall
{
  typedef std::tuple<typename std::decay<TArgs>::type...> tParameterTuple;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCMessage(uint8_t function_index, TArgs && ... args) :
    function_index(function_index),
    parameters(args...)
  {}

  template <typename TReturn, typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id)
  {
    try
    {
      typedef TReturn(TInterface::*tFunctionPointer)(TArgs...);
      tParameterTuple parameters;
      stream >> parameters;
      tFunctionPointer function_pointer = tRPCInterfaceType<TInterface>::template GetFunction<tFunctionPointer>(function_id);
      tClientPort<TInterface> client_port = tClientPort<TInterface>::Wrap(port);
      ExecuteCallImplementation<TInterface, tFunctionPointer>(client_port, function_pointer, parameters, typename rrlib::util::tIntegerSequenceGenerator<sizeof...(TArgs)>::type());
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(DEBUG, "Incoming RPC message caused exception: ", e);
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Index of function in interface */
  uint8_t function_index;

  /*! Parameters of RPC call */
  std::tuple<TArgs...> parameters;


  template <typename TInterface, typename TFunction, int ... SEQUENCE>
  static void ExecuteCallImplementation(tClientPort<TInterface>& client_port, TFunction function_pointer, tParameterTuple parameters, rrlib::util::tIntegerSequence<SEQUENCE...> sequence)
  {
    client_port.Call(function_pointer, std::get<SEQUENCE>(parameters)...);
  }

  virtual void Serialize(rrlib::serialization::tOutputStream& stream) // TODO: mark override in gcc 4.7
  {
    stream << function_index;
    stream << parameters;

    // TODO: destination port (in TCP). timeout: here. Local synchronizing info (possibly asynch return handler)
  }

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
