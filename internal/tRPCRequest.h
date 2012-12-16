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
/*!\file    plugins/rpc_ports/internal/tRPCRequest.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-08
 *
 * \brief   Contains tRPCRequest
 *
 * \b tRPCRequest
 *
 * This class stores and handles RPC calls that return a value.
 * For calls within the same runtime environment this class is not required.
 * Objects of this class are used to temporarily store such calls in queues
 * for network threads and to serialize them.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tRPCRequest_h__
#define __plugins__rpc_ports__internal__tRPCRequest_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tFuture.h"
#include "plugins/rpc_ports/internal/tAbstractCall.h"
#include "plugins/rpc_ports/internal/tRPCPort.h"
#include "plugins/rpc_ports/internal/tRPCResponse.h"

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
//! RPC request
/*!
 * This class stores and handles RPC calls that return a value.
 * For calls within the same runtime environment this class is not required.
 * Objects of this class are used to temporarily store such calls in queues
 * for network threads and to serialize them.
 */
template <typename TReturn, typename ... TArgs>
class tRPCRequest : public tAbstractCall
{
  typedef std::tuple<typename std::decay<TArgs>::type...> tParameterTuple;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCRequest(tCallStorage& storage, uint8_t function_index, const rrlib::time::tDuration& timeout, TArgs && ... args) :
    function_index(function_index),
    result_buffer(),
    parameters(args...),
    storage(storage),
    future_obtained(false),
    timeout(timeout)
  {}

  template <typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id, tCallId call_id)
  {
    try
    {
      typedef TReturn(TInterface::*tFunctionPointer)(TArgs...);
      tParameterTuple parameters;
      stream >> parameters;
      rrlib::time::tDuration timeout;
      stream >> timeout;
      tFunctionPointer function_pointer = tRPCInterfaceType<TInterface>::template GetFunction<tFunctionPointer>(function_id);
      tClientPort<TInterface> client_port = tClientPort<TInterface>::Wrap(port);
      ExecuteCallImplementation<TInterface, tFunctionPointer>(client_port, function_pointer, timeout, parameters, function_id, call_id, typename rrlib::util::tIntegerSequenceGenerator<sizeof...(TArgs)>::type());
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(DEBUG, "Incoming RPC call caused exception: ", e);
    }
  }

  /*!
   * \return Future to wait for result
   */
  tFuture<TReturn> GetFuture()
  {
    if (future_obtained)
    {
      throw std::runtime_error("Future already obtained");
    }
    future_obtained = true;
    return tFuture<TReturn>(storage.ObtainFuturePointer(), result_buffer);
  }

  /*!
   * \return Result of function call
   */
  TReturn GetResult()
  {
    //result_arrived = false;
    return std::move(result_buffer);
  }

  /*!
   * Indicates and notifies any futures/response handlers that RPC call
   * has returned a result
   *
   * \param return_value Returned value
   */
  void ReturnValue(TReturn && return_value)
  {
    tFutureStatus current = (tFutureStatus)storage.future_status.load();
    if (current != tFutureStatus::PENDING)
    {
      FINROC_LOG_PRINT(WARNING, "Call already has status ", make_builder::GetEnumString(current), ". Ignoring.");
      return;
    }

    rrlib::thread::tLock lock(storage.mutex);
    result_buffer = return_value;
    storage.future_status.store((int)tFutureStatus::READY);
    storage.condition_variable.notify_one();
    if (storage.response_handler)
    {
      static_cast<tResponseHandler<TReturn>*>(storage.response_handler)->HandleResponse(return_value);
    }
  }

//  /*!
//   * \return True after result arrived
//   */
//  bool ResultArrived() const
//  {
//    return result_arrived;
//  }

  void SetResponseHandler(tResponseHandler<TReturn>& response_handler)
  {
    storage.response_handler = &response_handler;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Index of function in interface */
  uint8_t function_index;

  /*! Result will be stored here */
  TReturn result_buffer;

  /*! Parameters of RPC call */
  tParameterTuple parameters;

  /*! Storage this RPC request was allocated in */
  tCallStorage& storage;

  /*! Has future been obtained? */
  bool future_obtained;

  /*! Timeout for call */
  rrlib::time::tDuration timeout;

  template <typename TInterface, typename TFunction, int ... SEQUENCE>
  static void ExecuteCallImplementation(tClientPort<TInterface>& client_port, TFunction function_pointer, const rrlib::time::tDuration& timeout, tParameterTuple& parameters, uint8_t function_id, tCallId call_id, rrlib::util::tIntegerSequence<SEQUENCE...> sequence)
  {
    typename tCallStorage::tPointer call_storage = tCallStorage::GetUnused();
    tRPCResponse<TReturn>& response = call_storage->Emplace<tRPCResponse<TReturn>>(*call_storage, function_id);
    response.SetCallId(call_id);
    try
    {
      response.SetReturnValue(client_port.CallSynchronous(timeout, function_pointer, std::get<SEQUENCE>(parameters)...));
    }
    catch (const tRPCException& e)
    {
      call_storage->SetException(e.GetType());
    }
    core::tAbstractPort* port = client_port.GetWrapped();
    if (port && port->IsReady())
    {
      static_cast<tRPCPort*>(port)->SendCall(call_storage);
    }
    else
    {
      FINROC_LOG_PRINT(DEBUG_WARNING, "Could not return value, because port is no longer available");
    }
  }

  virtual void ReturnValue(rrlib::serialization::tInputStream& stream, tRPCPort& port)
  {
    TReturn result;
    stream >> result;
    ReturnValue(std::move(result));
  }

  virtual void Serialize(rrlib::serialization::tOutputStream& stream) // TODO: mark override in gcc 4.7
  {
    stream << function_index;
    stream << timeout;
    stream << parameters;

    // TODO: destination port (in TCP). timeout: here. Local synchronizing info (possibly asynch return handler)
  }

};

struct tNoRPCRequest
{
  template <typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id, tCallId call_id)
  {
    throw new std::runtime_error("Not supported for functions returning void");
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
