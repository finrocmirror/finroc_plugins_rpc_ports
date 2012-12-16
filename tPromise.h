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
class tPromise : public boost::noncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tPromise() :
    storage(), result_flag(NULL), result_buffer(NULL)
  {

  }

  ~tPromise()
  {
    if (!result_flag->load())
    {
      SetException(tRPCException::tType::);
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to shared storage */
  typename internal::tCallStorage::tPointer storage;

  /*! Buffer with result */
  std::atomic<bool>* result_flag;

  /*! Buffer with result */
  T* result_buffer;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
