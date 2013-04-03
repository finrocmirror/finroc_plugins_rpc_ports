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
/*!\file    plugins/rpc_ports/internal/tRPCPort.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 * \brief   Contains tRPCPort
 *
 * \b tRPCPort
 *
 * RPC port implementation. Type-less, derived from tAbstractPort.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tRPCPort_h__
#define __plugins__rpc_ports__internal__tRPCPort_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tAbstractPort.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tRPCInterface.h"
#include "plugins/rpc_ports/internal/tCallStorage.h"

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
//! RPC port implementation.
/*!
 * RPC port implementation. Type-less, derived from tAbstractPort.
 *
 * Server is source port.
 * Client is target port.
 * One source may have multiple targets. However, a target may only
 * have one source in order to receive only one return value.
 */
class tRPCPort : public core::tAbstractPort
{

  /*! Deleter to unify two types of call storage pointer for sent calls */
  struct tCallDeleter
  {
    void operator()(tCallStorage* p) const
    {
      if (IsFuturePointer(*p))
      {
        tCallStorage::tFuturePointer::deleter_type deleter;
        deleter(p);
      }
      else
      {
        tCallStorage::tPointer::deleter_type deleter;
        deleter(p);
      }
    }
  };

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Stores calls internally */
  typedef std::unique_ptr<tCallStorage, tCallDeleter> tCallPointer;


  tRPCPort(core::tAbstractPortCreationInfo creation_info, tRPCInterface* call_handler);

  ~tRPCPort();


//  /*!
//   * Deserializes call from stream and executes it
//   *
//   * \param stream Stream to deserialize from
//   * \param call_returner Object to possibly pass future to value to that will be returned
//   */
//  void DeserializeAndExecuteCall(rrlib::serialization::tInputStream& stream)
//  {
//    uint8_t function_index;
//    stream >> function_index;
//    (*GetTypeInfo()->methods[function_index].deserialize_function)(stream, call_returner, GetTypeInfo()->methods[function_index].function_pointer);
//  }

  /*!
   * \return Pointer to object that handles calls on server side
   */
  tRPCInterface* GetCallHandler() const
  {
    return call_handler;
  }

  /*!
   * (Usually called on client ports)
   *
   * \param include_network_ports Also return network ports?
   * \return "Server" Port that handles method call (or null if there is no such port)
   */
  tRPCPort* GetServer(bool include_network_ports = false) const;

  /*!
   * \return Is this a server rpc port?
   */
  bool IsServer() const
  {
    return GetFlag(tFlag::ACCEPTS_DATA) && (!GetFlag(tFlag::EMITS_DATA));
  }

  /*!
   * Sends call to somewhere else
   * (Meant to be called on network ports that forward calls to other runtime environments)
   *
   * \param call_to_send Call that is sent
   */
  void SendCall(typename tCallStorage::tPointer& call_to_send)
  {
    assert(IsFuturePointer(*call_to_send) == false);
    SendCall(tCallPointer(call_to_send.release()));
  }
  void SendCall(typename tCallStorage::tFuturePointer && call_to_send)
  {
    assert(IsFuturePointer(*call_to_send) == true);
    SendCall(tCallPointer(call_to_send.release()));
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tResponseSender;

  /*! Pointer to object that handles calls on server side */
  tRPCInterface* const call_handler;


  virtual void ConnectionAdded(tAbstractPort& partner, bool partner_is_destination); // TODO mark override with gcc 4.7

  virtual tAbstractPort::tConnectDirection InferConnectDirection(const tAbstractPort& other) const; // TODO mark override with gcc 4.7

  static bool IsFuturePointer(tCallStorage& call_storage)
  {
    return call_storage.call_ready_for_sending == &(call_storage.future_status); // slightly ugly... but memory efficient (and we have the assertions)
  }

  /*!
   * To be overridden by network port subclass
   */
  virtual void SendCall(tCallPointer && call_to_send)
  {
    throw std::runtime_error("Not a network port");
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
