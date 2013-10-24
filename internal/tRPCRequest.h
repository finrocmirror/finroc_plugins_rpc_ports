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

  enum { cNATIVE_FUTURE_FUNCTION = std::is_base_of<internal::tIsFuture, TReturn>::value };
  typedef typename std::conditional<cNATIVE_FUTURE_FUNCTION, TReturn, tFuture<TReturn>>::type tResponseFuture;
  typedef typename tResponseFuture::tValue tReturnInternal;
  enum { cPROMISE_RESULT = std::is_base_of<internal::tIsPromise, tReturnInternal>::value };
  typedef tReturnValueSerialization<tReturnInternal, cPROMISE_RESULT, rrlib::serialization::tIsBinarySerializable<tReturnInternal>::value> tReturnSerialization;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  template <typename ... TCallArgs>
  tRPCRequest(tCallStorage& storage, tRPCPort& local_rpc_port, uint8_t function_index, const rrlib::time::tDuration& timeout, TCallArgs && ... args) :
    rpc_interface_type(local_rpc_port.GetDataType()),
    function_index(function_index),
    result_buffer(),
    parameters(std::forward<TCallArgs>(args)...),
    storage(storage),
    future_obtained(false)
  {
    storage.local_port_handle = local_rpc_port.GetHandle();
    storage.response_timeout = timeout;
    storage.call_type = tCallType::RPC_REQUEST;
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Creating Request ", &storage, " ", &storage.call_type);
  }

  template <typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id, tResponseSender& response_sender)
  {
    try
    {
      typedef TReturn(TInterface::*tFunctionPointer)(TArgs...);
      tCallId remote_call_id;
      stream >> remote_call_id;
      rrlib::time::tDuration timeout;
      stream >> timeout;
      tParameterTuple parameters;
      stream >> parameters;
      tFunctionPointer function_pointer = tRPCInterfaceType<TInterface>::template GetFunction<tFunctionPointer>(function_id);
      tClientPort<TInterface> client_port = tClientPort<TInterface>::Wrap(port, true);
      ExecuteCallImplementation<cNATIVE_FUTURE_FUNCTION, TInterface, tFunctionPointer>(client_port, response_sender, function_pointer, timeout, parameters, function_id, remote_call_id, typename rrlib::util::tIntegerSequenceGenerator<sizeof...(TArgs)>::type());
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(DEBUG, "Incoming RPC call caused exception: ", e);
    }
  }

  /*!
   * \return Future to wait for result
   */
  tResponseFuture GetFuture()
  {
    if (future_obtained)
    {
      throw std::runtime_error("Future already obtained");
    }
    future_obtained = true;
    storage.future_status.store((int)tFutureStatus::PENDING);
    return tResponseFuture(storage.ObtainFuturePointer(), result_buffer);
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
  void ReturnValue(tReturnInternal && return_value)
  {
    tFutureStatus current = (tFutureStatus)storage.future_status.load();
    if (current != tFutureStatus::PENDING)
    {
      FINROC_LOG_PRINT(WARNING, "Call already has status ", make_builder::GetEnumString(current), ". Ignoring.");
      return;
    }

    rrlib::thread::tLock lock(storage.mutex);
    result_buffer = std::move(return_value);
    storage.future_status.store((int)tFutureStatus::READY);
    storage.condition_variable.notify_one();
    if (storage.response_handler)
    {
      lock.Unlock();
      static_cast<tResponseHandler<tReturnInternal>*>(storage.response_handler)->HandleResponse(std::move(return_value));
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

  /*! RPC Interface Type */
  rrlib::rtti::tType rpc_interface_type;

  /*! Index of function in interface */
  uint8_t function_index;

  /*! Result will be stored here */
  tReturnInternal result_buffer;

  /*! Parameters of RPC call */
  tParameterTuple parameters;

  /*! Storage this RPC request was allocated in */
  tCallStorage& storage;

  /*! Has future been obtained? */
  bool future_obtained;


  template <bool NATIVE_FUTURE_CALL, typename TInterface, typename TFunction, int ... SEQUENCE>
  static void ExecuteCallImplementation(typename std::enable_if < !NATIVE_FUTURE_CALL, tClientPort<TInterface >>::type& client_port, tResponseSender& response_sender, TFunction function_pointer,
                                        const rrlib::time::tDuration& timeout, tParameterTuple& parameters, uint8_t function_id, tCallId call_id, rrlib::util::tIntegerSequence<SEQUENCE...> sequence)
  {
    typename tCallStorage::tPointer call_storage = tCallStorage::GetUnused();
    tRPCResponse<TReturn>& response = call_storage->Emplace<tRPCResponse<TReturn>>(*call_storage, client_port.GetDataType(), function_id);
    response.SetCallId(call_id);
    try
    {
      response.SetReturnValue(client_port.template CallSynchronous<TFunction, typename std::decay<TArgs>::type ...>
                              (timeout, function_pointer, std::move(std::get<SEQUENCE>(parameters))...));
      call_storage->local_port_handle = client_port.GetWrapped()->GetHandle();
    }
    catch (const tRPCException& e)
    {
      call_storage->SetException(e.GetType());
    }
    response_sender.SendResponse(call_storage);
  }

  template <bool NATIVE_FUTURE_CALL, typename TInterface, typename TFunction, int ... SEQUENCE>
  static void ExecuteCallImplementation(typename std::enable_if<NATIVE_FUTURE_CALL, tClientPort<TInterface>>::type& client_port, tResponseSender& response_sender, TFunction function_pointer,
                                        const rrlib::time::tDuration& timeout, tParameterTuple& parameters, uint8_t function_id, tCallId call_id, rrlib::util::tIntegerSequence<SEQUENCE...> sequence)
  {
    typename tCallStorage::tPointer call_storage = tCallStorage::GetUnused();
    tRPCResponse<TReturn>& response = call_storage->Emplace<tRPCResponse<TReturn>>(*call_storage, client_port.GetDataType(), function_id);
    response.SetCallId(call_id);
    try
    {
      response.SetReturnValue(client_port.template NativeFutureCall<TFunction, typename std::decay<TArgs>::type ...>
                              (function_pointer, std::move(std::get<SEQUENCE>(parameters))...));
      call_storage->local_port_handle = client_port.GetWrapped()->GetHandle();
    }
    catch (const tRPCException& e)
    {
      call_storage->SetException(e.GetType());
    }
    response_sender.SendResponse(call_storage);
  }

  virtual void ReturnValue(rrlib::serialization::tInputStream& stream, tResponseSender& response_sender) // TODO: mark override in gcc 4.7
  {
    tReturnInternal result;
    tReturnSerialization::Deserialize(stream, result, response_sender, function_index, rpc_interface_type);
    ReturnValue(std::move(result));
  }

  virtual void Serialize(rrlib::serialization::tOutputStream& stream) // TODO: mark override in gcc 4.7
  {
    // Deserialized by network transport implementation
    stream << rpc_interface_type << function_index;

    // Deserialized by this class
    stream << storage.call_id;
    stream << storage.response_timeout;
    stream << parameters;
  }

};

struct tNoRPCRequest
{
  template <typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, uint8_t function_id, tResponseSender& response_sender)
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
