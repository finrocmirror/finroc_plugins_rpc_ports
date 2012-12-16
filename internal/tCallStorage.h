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
/*!\file    plugins/rpc_ports/internal/tCallStorage.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-04
 *
 * \brief   Contains tCallStorage
 *
 * \b tCallStorage
 *
 * This class is used to temporarily store calls (requests, responses, pull calls)
 * - for instance, to enqueue calls in a network thread's queue.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tCallStorage_h__
#define __plugins__rpc_ports__internal__tCallStorage_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/buffer_pools/tBufferPool.h"
#include "core/log_messages.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/definitions.h"
#include "plugins/rpc_ports/internal/tAbstractCall.h"
#include "plugins/rpc_ports/internal/tAbstractResponseHandler.h"

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
class tFuture;

template <typename T>
class tPromise;

namespace internal
{

template <typename TReturn, typename ... TArgs>
class tRPCRequest;

template <typename TReturn>
class tRPCResponse;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Storage memory for all types of calls
/*!
 * This class is used to temporarily store calls (requests, responses, pull calls)
 * - for instance, to enqueue calls in a network thread's queue.
 */
class tCallStorage :
  public rrlib::concurrent_containers::tQueueable<rrlib::concurrent_containers::tQueueability::MOST_OPTIMIZED>,
  public rrlib::buffer_pools::tBufferManagementInfo
{

  /*! Buffer pool for call storage objects */
  typedef rrlib::buffer_pools::tBufferPool < tCallStorage, rrlib::concurrent_containers::tConcurrency::FULL,
          rrlib::buffer_pools::management::QueueBased, rrlib::buffer_pools::deleting::CollectGarbage > tCallStorageBufferPool;

  /*! Deleter that returns buffer */
  typedef typename tCallStorageBufferPool::tPointer::deleter_type tBufferReturner;

  template <bool FUTURE_POINTER>
  struct tLockReleaser
  {
    void operator()(tCallStorage* p) const
    {
      p->ReleaseLock<FUTURE_POINTER>();
    }
  };

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Smart pointer holding reference to this call storage object
   * If another pointer for use in tFuture is required, call ObtainFuturePointer()
   */
  typedef std::unique_ptr<tCallStorage, tLockReleaser<false>> tPointer;

  /*!
   * Smart pointer holding reference to this call storage object
   * from tFuture object - or some internal construct
   */
  typedef std::unique_ptr<tCallStorage, tLockReleaser<true>> tFuturePointer;

  /*!
   * Storage size in bytes
   * This is the maximum size of call classes stored in this object
   */
  enum { cSTORAGE_SIZE = 256 };

  tCallStorage();

  ~tCallStorage();


  /*!
   * Clear contents of this object
   * If call is currently stored in this object, calls its destructor
   */
  void Clear()
  {
    if (!empty)
    {
      GetCall()->~tAbstractCall();
      empty = true;
      response_handler = NULL;
    }
  }

  /*!
   * Creates specified class in this object
   *
   * \param constructor_arguments Arguments for constructor
   */
  template <typename TCallClass, typename ... TArgs>
  TCallClass& Emplace(TArgs && ... constructor_arguments)
  {
    static_assert(std::is_base_of<tAbstractCall, TCallClass>::value, "Must be subclass of tAbstractCall");
    Clear();
    TCallClass& result = *(new(&storage_memory) TCallClass(std::forward<TArgs>(constructor_arguments)...));
    empty = false;
    assert((&result == &static_cast<tAbstractCall&>(result)) && "tAbstractCall base class needs to be at offset zero");
    return result;
  }

  /*!
   * \return Pointer to call stored in this object - NULL if no call is currently stored
   */
  tAbstractCall* GetCall()
  {
    return empty ? NULL : reinterpret_cast<tAbstractCall*>(&storage_memory);
  }

  /*!
   * \return Unused call storage buffer
   */
  static tPointer GetUnused()
  {
    typename tCallStorageBufferPool::tPointer buffer = call_storage_buffer_pool.GetUnusedBuffer();
    buffer->reference_counter.store(1);
    return tPointer(buffer.release());
  }

  template <bool FUTURE_POINTER>
  void ReleaseLock()
  {
    int old = reference_counter.fetch_sub(1);
    assert(old >= 1);
    if (old == 1)
    {
      Clear();
      tBufferReturner returner;
      returner(this);
    }
    else if (!FUTURE_POINTER) // there is some future still holding on to this buffer
    {
      // Release promise
      if (future_status.load() == (int)tFutureStatus::PENDING)
      {
        SetException(tFutureStatus::BROKEN_PROMISE);
      }
    }
  }

  /*!
   * Indicates and notifies any futures/response handlers that RPC call
   * caused an exception
   *
   * \param new_status Type of exception
   */
  void SetException(tFutureStatus new_status)
  {
    tFutureStatus current = (tFutureStatus)future_status.load();
    if (current != tFutureStatus::PENDING)
    {
      FINROC_LOG_PRINT(WARNING, "Exception cannot be set twice. Ignoring.");
      return;
    }

    if (new_status == tFutureStatus::PENDING || new_status == tFutureStatus::READY)
    {
      throw std::runtime_error("Invalid value for exception");
    }

    rrlib::thread::tLock lock(mutex);
    future_status.store((int)new_status);
    condition_variable.notify_one();
    if (response_handler)
    {
      response_handler->HandleException(new_status);
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename T>
  friend class rpc_ports::tFuture;

  template <typename T>
  friend class rpc_ports::tPromise;

  template <typename TReturn, typename ... TArgs>
  friend class tRPCRequest;

  template <typename TReturn>
  friend class tRPCResponse;

  /*! Global buffer pool with storage objects (TODO: possibly optimize if this becomes a bottle-neck) */
  static tCallStorageBufferPool call_storage_buffer_pool;

  /*! Is currently a call stored in this object? */
  bool empty;

  /*! Mutex for thread synchronization */
  rrlib::thread::tMutex mutex;

  /*! Condition variable for thread synchronization */
  std::condition_variable condition_variable;

  /*! True while thread is waiting on condition variable */
  bool waiting;

  /*! Status for future */
  //std::atomic<tFutureStatus> future_status; // TODO: not supported by gcc 4.6 yet
  std::atomic<int> future_status;

  /*!
   * Signals that call is complete now and can be sent
   * (it is possible to enqueue incomplete calls in network send queue)
   */
  std::atomic<bool> call_complete_for_sending;

  /*! Reference counter on this storage */
  std::atomic<int> reference_counter;

  /*! Pointer to (optional) response handler */
  tAbstractResponseHandler* response_handler;

  /*! Call class storage memory */
  std::array<unsigned char, 256> storage_memory;


  /*!
   * \return Smart pointer to use inside tFuture
   * (ensures that access is safe as long as this pointer exists)
   */
  tFuturePointer ObtainFuturePointer()
  {
    __attribute__((unused))
    int old = reference_counter.fetch_add(1);
    assert(old >= 1 && "Obtained pointer to object without reference.");
    future_status.store((int)tFutureStatus::PENDING);
    return tFuturePointer(this);
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
