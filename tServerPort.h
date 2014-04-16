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
/*!\file    plugins/rpc_ports/tServerPort.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 * \brief   Contains tServerPort
 *
 * \b tServerPort
 *
 * Server RPC Port.
 * Accepts and handles function calls from any connected clients.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tServerPort_h__
#define __plugins__rpc_ports__tServerPort_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tPortWrapperBase.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tClientPort.h"
#include "plugins/rpc_ports/tRPCInterfaceType.h"
#include "plugins/rpc_ports/internal/tRPCPort.h"

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
//! RPC Port
/*!
 * Server RPC Port.
 * Accepts and handles function calls from any connected clients.
 *
 * \tparam T RPC Interface type (any class with some functions derived from tRPCInterface)
 */
template <typename T>
class tServerPort : public core::tPortWrapperBase
{
  static_assert(std::is_base_of<tRPCInterface, T>::value, "T must be subclass of tRPCInterface");

  struct tServerPortCreationInfo;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Creates no wrapped port */
  tServerPort() {}

  /*!
   * Constructor takes variadic argument list... just any properties you want to assign to port.
   *
   * It is mandatory to provide a reference to T to the constructor.
   * The first string is interpreted as port name, the second possibly as config entry (relevant for parameters only).
   * A framework element pointer is interpreted as parent.
   * tFrameworkElementFlag arguments are interpreted as flags.
   * tAbstractPortCreationInfo argument is copied. This is only allowed as first argument.
   */
  template <typename TArgument1, typename ... TArguments>
  explicit tServerPort(TArgument1&& arg1, TArguments&& ... args)
  {
    tConstructorArguments<tServerPortCreationInfo> creation_info(std::forward<TArgument1>(arg1), std::forward<TArguments>(args)...);
    creation_info.data_type = tRPCInterfaceType<T>();
    creation_info.flags |= core::tFrameworkElement::tFlag::ACCEPTS_DATA;
    if (!creation_info.server_interface)
    {
      throw std::runtime_error("Server object must be provided");
    }
    if (!(creation_info.flags.Raw() & core::tFrameworkElementFlags(core::tFrameworkElementFlag::DELETED).Raw())) // do not create port, if deleted flag is set
    {
      this->SetWrapped(new internal::tRPCPort(creation_info, creation_info.server_interface));
    }
  }

  /*!
   * Wraps raw port
   * Throws std::runtime_error if port to wrap has invalid type or flags.
   *
   * \param wrap Type-less port to wrap as tPort<T>
   */
  static tServerPort Wrap(core::tAbstractPort& wrap)
  {
    if (wrap.GetDataType().GetRttiName() != typeid(T).name())
    {
      throw std::runtime_error("Port to wrap has invalid type");
    }
    if ((!wrap.GetFlag(core::tFrameworkElement::tFlag::ACCEPTS_DATA)) || (wrap.GetFlag(core::tFrameworkElement::tFlag::EMITS_DATA)))
    {
      throw std::runtime_error("Port to wrap has invalid flags");
    }
    tServerPort port;
    port.SetWrapped(&wrap);
    return port;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Creation info for server ports */
  struct tServerPortCreationInfo : core::tAbstractPortCreationInfo
  {
    typedef core::tAbstractPortCreationInfo tBase;

    /*! Pointer to server interface */
    T* server_interface;

    tServerPortCreationInfo() : server_interface(nullptr) {}

    void Set(T& server_interface)
    {
      this->server_interface = &server_interface;
    }
  };
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
