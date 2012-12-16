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
#include "plugins/rpc_ports/internal/tAbstractCall.h"

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

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCResponse(tCallStorage& storage, uint8_t function_index) :
    function_index(function_index),
    result_buffer(),
    storage(storage),
    future_obtained(false),
    call_id(std::numeric_limits<tCallId>::max())
  {}


  // TODO: Ensure that tCallStorage* request is not recycled while we're calling this. tFuturePointer or lock?
  template <typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, tCallStorage* request)
  {
    try
    {
      tFutureStatus status;
      stream >> status;
      if (status == tFutureStatus::READY)
      {
        if (request && request->GetCall())
        {
          request->GetCall()->ReturnValue(stream, port);
        }
        else
        {
          // make sure promises are broken
          TReturn returned;
          stream >> returned;
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

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

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

  /*  virtual void HandleException(tFutureStatus exception_type)
    {
      storage.SetException(exception_type);
      storage.call_complete_for_sending.store(true);
    }

    virtual void HandleResponse(TReturn && call_result)
    {
      SetReturnValue(std::forward(call_result));
      storage.call_complete_for_sending.store(true);
    }*/

  virtual void Serialize(rrlib::serialization::tOutputStream& stream) // TODO: mark override in gcc 4.7
  {
    stream << function_index;
    tFutureStatus status = (tFutureStatus)storage.future_status.load();
    stream << status;
    if (status == tFutureStatus::READY)
    {
      // TODO: Releasen??!!
      stream << result_buffer;
    }
  }
};

struct tNoRPCResponse
{
  template <typename TInterface>
  static void DeserializeAndExecuteCallImplementation(rrlib::serialization::tInputStream& stream, tRPCPort& port, tCallStorage* request)
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
