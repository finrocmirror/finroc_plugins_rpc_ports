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

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Creates no wrapped port */
  tServerPort() {}

  /*!
   * Constructor takes variadic argument list... just any properties you want to assign to port.
   *
   * The first string is interpreted as port name, the second possibly as config entry (relevant for parameters only).
   * A framework element pointer is interpreted as parent.
   * tFrameworkElementFlag arguments are interpreted as flags.
   * tAbstractPortCreationInfo argument is copied. This is only allowed as first argument.
   *
   * \param interface Object that handles calls on server side
   */
  template <typename ... TArguments>
  tServerPort(T& interface, const TArguments& ... args)
  {
    tConstructorArguments<core::tAbstractPortCreationInfo> creation_info(args...);
    creation_info.data_type = tRPCInterfaceType<T>();
    creation_info.flags |= core::tFrameworkElement::tFlag::ACCEPTS_DATA;
    this->SetWrapped(new internal::tRPCPort(creation_info, &interface));
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

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
