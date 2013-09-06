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
/*!\file    plugins/rpc_ports/tProxyPort.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-31
 *
 * \brief   Contains tProxyPort
 *
 * \b tProxyPort
 *
 * Proxy (or "routing") RPC port.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tProxyPort_h__
#define __plugins__rpc_ports__tProxyPort_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tPortWrapperBase.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
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
//! Proxy RPC port
/*!
 * Proxy (or "routing") RPC port (similar to tProxyPort in data_ports plugin)
 */
template <typename T, bool SERVER_PORT>
class tProxyPort : public core::tPortWrapperBase
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Creates no wrapped port */
  tProxyPort() {}

  /*!
   * Constructor takes variadic argument list... just any properties you want to assign to port.
   *
   * The first string is interpreted as port name, the second possibly as config entry (relevant for parameters only).
   * A framework element pointer is interpreted as parent.
   * tFrameworkElementFlag arguments are interpreted as flags.
   * tAbstractPortCreationInfo argument is copied. This is only allowed as first argument.
   */
  template <typename TArg1, typename TArg2, typename ... TRest>
  tProxyPort(const TArg1& arg1, const TArg2& arg2, const TRest&... args)
  {
    tConstructorArguments<core::tAbstractPortCreationInfo> creation_info(arg1, arg2, args...);
    creation_info.data_type = tRPCInterfaceType<T>();
    creation_info.flags |= core::tFrameworkElement::tFlag::ACCEPTS_DATA | core::tFrameworkElement::tFlag::EMITS_DATA;
    if (!SERVER_PORT)
    {
      creation_info.flags |= core::tFrameworkElement::tFlag::OUTPUT_PORT;
    }
    this->SetWrapped(new internal::tRPCPort(creation_info, NULL));
  }

  // with a single argument, we do not want catch calls for copy construction
  template < typename TArgument1, bool ENABLE = !std::is_base_of<tProxyPort, TArgument1>::value >
  tProxyPort(const TArgument1& argument1, typename std::enable_if<ENABLE, tNoArgument>::type no_argument = tNoArgument())
  {
    // Call the above constructor
    *this = tProxyPort(tFlags(), argument1);
  }

  /*!
   * Wraps raw port
   * Throws std::runtime_error if port to wrap has invalid type or flags.
   *
   * \param wrap Type-less port to wrap as tPort<T>
   */
  static tProxyPort Wrap(core::tAbstractPort& wrap)
  {
    if (wrap.GetDataType().GetRttiName() != typeid(T).name())
    {
      throw std::runtime_error("Port to wrap has invalid type");
    }
    if ((!wrap.GetFlag(core::tFrameworkElement::tFlag::ACCEPTS_DATA)) || (!wrap.GetFlag(core::tFrameworkElement::tFlag::EMITS_DATA)))
    {
      throw std::runtime_error("Port to wrap has invalid flags");
    }
    tProxyPort port;
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
