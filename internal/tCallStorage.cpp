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
/*!\file    plugins/rpc_ports/internal/tCallStorage.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-04
 *
 */
//----------------------------------------------------------------------
#include "plugins/rpc_ports/internal/tCallStorage.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------

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
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
typename tCallStorage::tCallStorageBufferPool tCallStorage::call_storage_buffer_pool;

tCallStorage::tCallStorage() :
  empty(true),
  mutex(),
  condition_variable(),
  waiting(false),
  future_status((int)tFutureStatus::PENDING),
  call_ready_for_sending(NULL),
  reference_counter(0),
  response_handler(NULL),
  response_timeout(std::chrono::seconds(0)),
  call_id(0),
  call_type(tCallType::UNSPECIFIED),
  local_port_handle(0),
  remote_port_handle(0),
  storage_memory()
{}

tCallStorage::~tCallStorage()
{
  Clear();
}

typename tCallStorage::tPointer tCallStorage::GetUnused()
{
  typename tCallStorageBufferPool::tPointer buffer = call_storage_buffer_pool.GetUnusedBuffer();
  if (!buffer)
  {
    std::unique_ptr<tCallStorage> new_buffer(new tCallStorage());
    buffer = call_storage_buffer_pool.AddBuffer(std::move(new_buffer));
  }
  buffer->reference_counter.store(1);
  buffer->call_ready_for_sending = NULL;
  buffer->response_timeout = std::chrono::seconds(0);
  return tPointer(buffer.release());
}

void tCallStorage::SetException(tFutureStatus new_status)
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
    lock.Unlock();
    response_handler->HandleException(new_status);
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
