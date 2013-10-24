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
/*!\file    plugins/rpc_ports/tFuture.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-09
 *
 * \brief   Contains tFuture
 *
 * \b tFuture
 *
 * Somewhat similar to std::future, but tailored to RPC port usage
 * (tCallStorage objects used as shared memory).
 *
 * Some irrelevant functionality (reference types, shared futures) is
 * removed as it is not required in the context of RPC ports.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tFuture_h__
#define __plugins__rpc_ports__tFuture_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tResponseHandler.h"
#include "plugins/rpc_ports/tRPCException.h"
#include "plugins/rpc_ports/internal/tCallStorage.h"

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
template <typename TReturn, typename ... TArgs>
class tRPCRequest;

template <typename TReturn>
class tRPCResponse;

/*! Base class for all futures - to be able to identify them */
class tIsFuture : private rrlib::util::tNoncopyable {};
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Future, similar to std::future
/*!
 * Somewhat similar to std::future, but tailored to RPC port usage
 * (tCallStorage objects used as shared memory).
 *
 * Some irrelevant functionality (reference types, shared futures) is
 * removed as it is not required in the context of RPC ports.
 */
template <typename T>
class tFuture : public internal::tIsFuture
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef T tValue;

  tFuture() : storage(), result_buffer(NULL), callback_set(false) {}

  /*! Move constructor */
  tFuture(tFuture && other) : storage(), result_buffer(NULL), callback_set(false)
  {
    std::swap(storage, other.storage);
    std::swap(result_buffer, other.result_buffer);
    std::swap(callback_set, other.callback_set);
  }

  /*! Move assignment */
  tFuture& operator=(tFuture && other)
  {
    std::swap(storage, other.storage);
    std::swap(result_buffer, other.result_buffer);
    std::swap(callback_set, other.callback_set);
    return *this;
  }

  ~tFuture()
  {
    if (callback_set)
    {
      assert(storage);
      rrlib::thread::tLock lock(storage->mutex);
      storage->response_handler = NULL;
    }
  }

  /*!
   * Obtains value from future.
   * It it is not available blocks for the specified amount of time.
   * If call fails, throws an tRPCException.
   *
   * \param timeout Timeout. If this expires, a tRPCException(tFutureStatus::TIMEOUT) is thrown
   * \return Value obtained from call
   */
  T Get(const rrlib::time::tDuration& timeout = std::chrono::seconds(5))
  {
    if (!Valid())
    {
      throw tRPCException(tFutureStatus::INVALID_FUTURE);
    }
    tFutureStatus status = (tFutureStatus)storage->future_status.load();
    if (status == tFutureStatus::PENDING)
    {
      rrlib::thread::tLock lock(storage->mutex);
      status = (tFutureStatus)storage->future_status.load();
      if (status == tFutureStatus::PENDING)
      {
        if (storage->waiting)
        {
          RRLIB_LOG_PRINT(ERROR, "There's already a thread waiting on this object");
          throw tRPCException(tFutureStatus::INVALID_CALL);
        }
        storage->waiting = true;
        std::cv_status cv_status = storage->condition_variable.wait_for(lock.GetSimpleLock(), timeout);
        storage->waiting = false;
        if (cv_status == std::cv_status::timeout)
        {
          throw tRPCException(tFutureStatus::TIMEOUT);
        }
        status = (tFutureStatus)storage->future_status.load();
        if (status == tFutureStatus::PENDING)
        {
          throw tRPCException(tFutureStatus::INTERNAL_ERROR);
        }
      }
    }

    if (status != tFutureStatus::READY)
    {
      throw tRPCException(status);
    }

    T result = std::move(*result_buffer);
    storage.reset();
    result_buffer = NULL;
    return std::move(result);
  }

  /*!
   * \return True when value is available
   */
  bool Ready() const
  {
    if (!Valid())
    {
      return false;
    }
    return storage->future_status.load() != (int)tFutureStatus::PENDING;
  }

  /*!
   * Sets callback which is called when future receives value
   * If future already has value, callback is never called
   * Callback is removed when this future is destructed
   *
   * \param callback Callback
   */
  void SetCallback(tResponseHandler<T>& callback)
  {
    if ((!storage) || callback_set)
    {
      throw std::runtime_error("Cannot set callback");
    }
    storage->response_handler = &callback;
    callback_set = true;
  }

  /*! see std::future::valid() */
  bool Valid() const
  {
    return storage.get();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename TReturn, typename ... TArgs>
  friend class internal::tRPCRequest;

  template <typename TReturn>
  friend class internal::tRPCResponse;

  template <typename TReturn>
  friend class tPromise;


  /*! Pointer to shared storage */
  typename internal::tCallStorage::tFuturePointer storage;

  /*! Buffer with result */
  T* result_buffer;

  /*! True, if a callback for this future was set */
  bool callback_set;


  tFuture(typename internal::tCallStorage::tFuturePointer && storage, T& result_buffer) :
    storage(std::move(storage)),
    result_buffer(&result_buffer),
    callback_set(false)
  {}
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
