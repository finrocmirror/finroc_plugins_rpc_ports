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
/*!\file    plugins/rpc_ports/tPromise.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-09
 *
 * \brief   Contains tPromise
 *
 * \b tPromise
 *
 * Very similar to std::promise, but with some additional functionality
 * to better integrate with rpc ports
 * (tCallStorage objects used as shared memory; can also set atomic<bool>
 * instead of notifying thread etc.)
 *
 * Some irrelevant functionality (reference types, set value at thread exit)
 * is removed as it is not required in the context of RPC ports.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tPromise_h__
#define __plugins__rpc_ports__tPromise_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tResponseHandler.h"
#include "plugins/rpc_ports/internal/tCallStorage.h"
#include "plugins/rpc_ports/internal/tResponseSender.h"

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
namespace internal
{

template <typename TReturn, bool PROMISE, bool SERIALIZABLE>
struct tReturnValueSerialization;

/*! Base class for all promises - to be able to identify them */
class tIsPromise : private rrlib::util::tNoncopyable {};
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Promise, similar to std::promise
/*!
 * Very similar to std::promise, but with some additional functionality
 * to better integrate with rpc ports
 * (tCallStorage objects used as shared memory; can also set atomic<bool>
 * instead of notifying thread etc.)
 *
 * Some irrelevant functionality (reference types, set value at thread exit)
 * is removed as it is not required in the context of RPC ports.
 *
 * Can also be used as return type from RPC calls.
 * This somewhat allows implementing the RAII idiom across RPC ports
 * Automatic unlocking of blackboards is an example.
 * This works regardless of where a call might get lost - even internally
 * (e.g. an timeout could occur, ports could be deleted etc.)
 *
 * Classes can be derived from this. They, however, need to be movable.
 */
template <typename T>
class tPromise : public internal::tIsPromise
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef T tValue;

  tPromise() :
    storage(internal::tCallStorage::GetUnused()),
    result_buffer(&(storage->Emplace<tStorageContents>(*storage)).result_buffer)
  {
    storage->future_status.store(static_cast<int>(tFutureStatus::PENDING));
  }

  // Move constructor
  tPromise(tPromise && other) :
    storage(),
    result_buffer(NULL)
  {
    std::swap(storage, other.storage);
    std::swap(result_buffer, other.result_buffer);
  }

  // Move assignment
  tPromise& operator=(tPromise && other)
  {
    std::swap(storage, other.storage);
    std::swap(result_buffer, other.result_buffer);
    return *this;
  }

  ~tPromise()
  {
    // 'storage' pointer deletion breaks promise as intended
  }

  /*!
   * \return Future to wait for result
   */
  tFuture<T> GetFuture()
  {
    return tFuture<T>(storage->ObtainFuturePointer(), *result_buffer);
  }

  /*!
   * Set promise to exception
   * (see std::promise::set_exception)
   */
  void SetException(tFutureStatus exception_status)
  {
    storage->SetException(exception_status);
  }

  /*!
   * Set promise's value
   * (see std::promise::set_value)
   */
  void SetValue(T && value)
  {
    tFutureStatus current = (tFutureStatus)storage->future_status.load();
    if (current != tFutureStatus::PENDING)
    {
      FINROC_LOG_PRINT(WARNING, "Call already has status ", make_builder::GetEnumString(current), ". Ignoring.");
      return;
    }

    rrlib::thread::tLock lock(storage->mutex);
    *result_buffer = std::move(value);
    storage->future_status.store((int)tFutureStatus::READY);
    storage->condition_variable.Notify(lock);
    if (storage->response_handler)
    {
      lock.Unlock();
      static_cast<tResponseHandler<T>*>(storage->response_handler)->HandleResponse(std::move(*result_buffer));
    }
  }
  void SetValue(T& value)
  {
    SetValue(std::move(value));
  }
  void SetValue(const T& value)
  {
    tFutureStatus current = (tFutureStatus)storage->future_status.load();
    if (current != tFutureStatus::PENDING)
    {
      FINROC_LOG_PRINT(WARNING, "Call already has status ", make_builder::GetEnumString(current), ". Ignoring.");
      return;
    }

    rrlib::thread::tLock lock(storage->mutex);
    *result_buffer = value;
    storage->future_status.store((int)tFutureStatus::READY);
    storage->condition_variable.Notify(lock);
    if (storage->response_handler)
    {
      lock.Unlock();
      static_cast<tResponseHandler<T>*>(storage->response_handler)->HandleResponse(std::move(*result_buffer));
    }
  }


//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename TReturn, bool PROMISE, bool SERIALIZABLE>
  friend struct internal::tReturnValueSerialization;

  class tStorageContents : public internal::tAbstractCall
  {
  public:

    /*! Storage this RPC request was allocated in */
    internal::tCallStorage& storage;

    /*! Buffer with result */
    T result_buffer;

    /*! Index of function in interface */
    uint8_t function_index;

    /*! Id of remote promise - if this is a remote promise */
    internal::tCallId remote_promise_call_id;

    /*! RPC Interface Type */
    rrlib::rtti::tType rpc_interface_type;

    tStorageContents(internal::tCallStorage& storage) :
      storage(storage),
      result_buffer(),
      function_index(0),
      remote_promise_call_id(0),
      rpc_interface_type()
    {}

    virtual void Serialize(rrlib::serialization::tOutputStream& stream) override
    {
      // Deserialized by network transport implementation
      stream << rpc_interface_type;
      stream << function_index;
      stream << remote_promise_call_id;

      // Deserialized by this class
      stream << true; // promise_response
      tFutureStatus status = (tFutureStatus)storage.future_status.load();
      assert(status == tFutureStatus::READY && "only ready responses should be serialized");
      stream << status;
      stream << result_buffer;
    }
  };

  /*! Pointer to shared storage */
  typename internal::tCallStorage::tPointer storage;

  /*! Buffer with result */
  T* result_buffer;


  /*!
   * Mark/init this promise a remote promise
   */
  void SetRemotePromise(uint8_t function_index, internal::tCallId call_id, internal::tResponseSender& response_sender, const rrlib::rtti::tType& rpc_interface_type)
  {
    tStorageContents* contents = static_cast<tStorageContents*>(storage->GetCall());
    contents->function_index = function_index;
    contents->remote_promise_call_id = call_id;
    contents->rpc_interface_type = rpc_interface_type;
    storage->call_ready_for_sending = &(storage->future_status);
    storage->call_type = tCallType::RPC_RESPONSE;
    internal::tCallStorage::tFuturePointer call_pointer = storage->ObtainFuturePointer();
    response_sender.SendResponse(std::move(call_pointer));
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
