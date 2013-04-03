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
/*!\file    plugins/rpc_ports/internal/tRPCResponse.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-09
 *
 * \brief   Contains tRPCResponse
 *
 * \b tRPCResponse
 *
 * This call stores and handles a response from an RPC call.
 * For calls within the same runtime environment this class is not required.
 * Objects of this class are used to temporarily store such calls in queues
 * for network threads and to serialize them.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tRPCResponse_h__
#define __plugins__rpc_ports__internal__tRPCResponse_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tPromise.h"
#include "plugins/rpc_ports/internal/tReturnValueSerialization.h"

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
//! RPC response
/*!
 * This call stores and handles a response from an RPC call.
 * For calls within the same runtime environment this class is not required.
 * Objects of this class are used to temporarily store such calls in queues
 * for network threads and to serialize them.
 */
template <typename TReturn>
class tRPCResponse : public tAbstractCall //, public tResponseHandler<TReturn>
{

  enum { cPROMISE_RESULT = std::is_base_of<tIsPromise, TReturn>::value };
  typedef tReturnValueSerialization<TReturn, cPROMISE_RESULT, rrlib::serialization::tIsBinarySerializable<TReturn>::value> tReturnSerialization;
  typedef typename std::conditional<cPROMISE_RESULT, TReturn, tPromise<TReturn>>::type tResponsePromise;
  typedef typename tResponsePromise::tValue tPromiseValue;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCResponse(tCallStorage& storage, const rrlib::rtti::tType& rpc_interface_type, uint8_t function_index) :
    rpc_interface_type(rpc_interface_type),
    function_index(function_index),
    result_buffer(),
    storage(storage),
    future_obtained(false),
    call_id(std::numeric_limits<tCallId>::max())
  {
    storage.response_timeout = cPROMISE_RESULT ? std::chrono::hours(24) : std::chrono::seconds(0); // TODO: put something sensible here
    storage.call_type = tCallType::RPC_RESPONSE;
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Creating Response ", &storage, " ", &storage.call_type);
  }


  // TODO: Ensure that tCallStorage* request is not recycled while we're calling this. tFuturePointer or lock?
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, const rrlib::rtti::tType& rpc_interface_type,
      uint8_t function_id, tResponseSender& response_sender, tCallStorage* request)
  {
    try
    {
      bool promise_response;
      stream >> promise_response;
      tFutureStatus status;
      stream >> status;
      FINROC_LOG_PRINT_STATIC(DEBUG_VERBOSE_1, promise_response, " ", make_builder::GetEnumString(status));
      if (status == tFutureStatus::READY)
      {
        if (request && request->GetCall())
        {
          request->GetCall()->ReturnValue(stream, response_sender);
        }
        else
        {
          if (!promise_response)
          {
            // make sure e.g. promises are broken
            TReturn returned;
            tReturnSerialization::Deserialize(stream, returned, response_sender, function_id, rpc_interface_type);
          }
          else
          {
            // TODO: actually unnecessary - we could also skip
            tPromiseValue returned;
            stream >> returned;
          }
        }
      }
      else
      {
        if (request)
        {
          request->SetException(status);
        }
      }
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(DEBUG, "Incoming RPC response caused exception: ", e);
    }
  }

//  /*!
//   * \return Future to wait for result
//   */
//  tFuture<TReturn> GetFuture()
//  {
//    if (future_obtained)
//    {
//      throw std::runtime_error("Future already obtained");
//    }
//    future_obtained = true;
//    return tFuture<TReturn>(storage.ObtainFuturePointer(), result_buffer);
//  }

  void SetCallId(tCallId call_id)
  {
    this->call_id = call_id;
  }

  /*!
   * Indicates and notifies any futures/response handlers that RPC call
   * has returned a result
   *
   * \param return_value Returned value
   */
  void SetReturnValue(TReturn && return_value)
  {
//    tFutureStatus current = (tFutureStatus)storage.future_status.load();
//    if (current != tFutureStatus::PENDING)
//    {
//      FINROC_LOG_PRINT(WARNING, "Call already has status ", make_builder::GetEnumString(current), ". Ignoring.");
//      return;
//    }
//
//    rrlib::thread::tLock lock(storage.mutex);
    result_buffer = std::move(return_value);
    storage.future_status.store((int)tFutureStatus::READY);
//    storage.condition_variable.notify_one();
//    if (storage.response_handler)
//    {
//      static_cast<tResponseHandler<tReturnInternal>*>(storage.response_handler)->HandleResponse(return_value);
//    }
  }


//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename T>
  friend class tRPCResponse;

  /*! RPC Interface Type */
  rrlib::rtti::tType rpc_interface_type;

  /*! Index of function in interface */
  uint8_t function_index;

  /*! Result will be stored here */
  TReturn result_buffer;

  /*! Storage this RPC request was allocated in */
  tCallStorage& storage;

  /*! Has future been obtained? */
  bool future_obtained;

  /*! Identification of call on client side */
  tCallId call_id;


  virtual void ReturnValue(rrlib::serialization::tInputStream& stream, tResponseSender& response_sender) // TODO: mark override in gcc 4.7
  {
    ReturnValueImplementation(stream);
  }

  template <bool ENABLE = cPROMISE_RESULT>
  typename std::enable_if<ENABLE, void>::type ReturnValueImplementation(rrlib::serialization::tInputStream& stream)
  {
    tPromiseValue result;
    stream >> result;
    result_buffer.SetValue(std::move(result));
  }

  template <bool DISABLE = cPROMISE_RESULT>
  typename std::enable_if < !DISABLE, void >::type ReturnValueImplementation(rrlib::serialization::tInputStream& stream)
  {
    throw std::runtime_error("Not a promise response");
  }

  virtual void Serialize(rrlib::serialization::tOutputStream& stream) // TODO: mark override in gcc 4.7
  {
    // Deserialized by network transport implementation
    stream << rpc_interface_type << function_index;
    stream << call_id;

    // Deserialized by this class
    stream << false; // promise_response
    tFutureStatus status = (tFutureStatus)storage.future_status.load();
    stream << status;
    if (status == tFutureStatus::READY)
    {
      tReturnSerialization::Serialize(stream, result_buffer, storage);
    }
  }
};

// Specialization for future return type
template <typename TReturn>
class tRPCResponse<tFuture<TReturn>> : public tRPCResponse<TReturn>
{
  typedef tRPCResponse<TReturn> tBase;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCResponse(tCallStorage& storage, const rrlib::rtti::tType& rpc_interface_type, uint8_t function_index) :
    tBase(storage, rpc_interface_type, function_index),
    response_future()
  {
  }

  /*!
   * (only used for native future functions)
   * Indicates and notifies any futures/response handlers that RPC call
   * has returned a result
   *
   * \param return_value Returned value
   */
  void SetReturnValue(tFuture<TReturn> && return_value)
  {
    this->storage.call_ready_for_sending = &return_value.storage->future_status;
    response_future = std::move(return_value);
    this->storage.future_status.store((int)tFutureStatus::READY);
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! future for response */
  tFuture<TReturn> response_future;


  virtual void Serialize(rrlib::serialization::tOutputStream& stream) // TODO: mark override in gcc 4.7
  {
    // Deserialized by network transport implementation
    stream << this->rpc_interface_type << this->function_index;
    stream << this->call_id;

    // Deserialized by this class
    stream << false; // promise_response
    tFutureStatus status = (tFutureStatus)this->storage.future_status.load();
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, make_builder::GetEnumString(status));
    if (status == tFutureStatus::READY)
    {
      status = (tFutureStatus)this->storage.call_ready_for_sending->load();
    }
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, make_builder::GetEnumString(status));
    stream << status;
    if (status == tFutureStatus::READY)
    {
      assert(response_future.Ready() && "only ready responses should be serialized");
      this->result_buffer = response_future.Get();
      tBase::tReturnSerialization::Serialize(stream, this->result_buffer, this->storage);
    }
  }
};

struct tNoRPCResponse
{
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, const rrlib::rtti::tType&,
      uint8_t function_id, tResponseSender& response_sender, tCallStorage* request)
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
