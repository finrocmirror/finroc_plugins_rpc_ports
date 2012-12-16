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
/*!\file    plugins/rpc_ports/tClientPort.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 * \brief   Contains tClientPort
 *
 * \b tClientPort
 *
 * Client RPC port.
 * Can be used to call functions on connnected server port.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tClientPort_h__
#define __plugins__rpc_ports__tClientPort_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <functional>
#include "core/port/tPortWrapperBase.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tPortCreationInfo.h"
#include "plugins/rpc_ports/tResponseHandler.h"
#include "plugins/rpc_ports/tRPCException.h"
#include "plugins/rpc_ports/tRPCInterfaceType.h"
#include "plugins/rpc_ports/internal/tRPCMessage.h"
#include "plugins/rpc_ports/internal/tRPCPort.h"
#include "plugins/rpc_ports/internal/tRPCRequest.h"

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
//! Client RPC port
/*!
 * Client RPC port.
 * Can be used to call functions on connnected server port.
 *
 * \tparam T RPC Interface type (any class with some functions derived from tRPCInterface)
 */
template <typename T>
class tClientPort : public core::tPortWrapperBase
{
  static_assert(std::is_base_of<tRPCInterface, T>::value, "T must be subclass of tRPCInterface");

  template <typename U>
  static U& MakeU();

  /*! Helper struct to extract return type from function type */
  template <typename TFunction>
  struct tReturnType
  {
    template <typename RETURN, typename ... TArgs>
    static RETURN ExtractReturnType(RETURN(T::*function_pointer)(TArgs...));

    typedef decltype(ExtractReturnType(MakeU<TFunction>())) type;
  };

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Constructor takes variadic argument list... just any properties you want to assign to port.
   *
   * The first string is interpreted as port name, the second possibly as config entry (relevant for parameters only).
   * A framework element pointer is interpreted as parent.
   * tFrameworkElementFlag arguments are interpreted as flags.
   * tAbstractPortCreationInfo argument is copied. This is only allowed as first argument.
   */
  template <typename ... ARGS>
  tClientPort(ARGS && ... args)
  {
    tPortCreationInfo<T> creation_info(args...);
    creation_info.data_type = tRPCInterfaceType<T>();
    creation_info.flags |= core::tFrameworkElement::tFlag::EMITS_DATA | core::tFrameworkElement::tFlag::OUTPUT_PORT;
    this->SetWrapped(new internal::tRPCPort(creation_info, NULL));
  }


  /*!
   * Calls specified function ignoring any return value or exception
   * (in other words: send message)
   *
   * \param function Function to call
   * \param args Arguments for function call
   */
  template <typename TFunction, typename ... TArgs>
  void Call(TFunction function, TArgs && ... args)
  {
    internal::tRPCPort* server_port = GetWrapped()->GetServer(true);
    if (server_port)
    {
      tRPCInterface* server_interface = server_port->GetCallHandler();
      if (server_interface)
      {
        try
        {
          (static_cast<T*>(server_interface)->*function)(args...);
        }
        catch (const tRPCException& e)
        {
          FINROC_LOG_PRINT(DEBUG, e);
        }
      }
      else
      {
        typename internal::tCallStorage::tPointer call_storage = internal::tCallStorage::GetUnused();
        call_storage->Emplace<internal::tRPCMessage<TArgs...>>(tRPCInterfaceType<T>::GetFunctionID(function), std::forward<TArgs>(args)...);
        server_port->SendCall(call_storage);
      }
    }
  }


  /*!
   * Calls specified function asynchronously
   * Result of function call is forwarded to the return handler provided.
   *
   * \param response_handler Return handler to receive results
   * \param function Function to call
   * \param args Arguments for function call
   */
  template <typename TFunction, typename ... TArgs>
  void CallAsynchronous(tResponseHandler<typename tReturnType<TFunction>::type>& response_handler, TFunction function, TArgs && ... args)
  {
    typedef typename tReturnType<TFunction>::type tReturn;
    static_assert(!std::is_same<tReturn, void>::value, "Call plain Call() for functions without return value");

    internal::tRPCPort* server_port = GetWrapped()->GetServer(true);
    if (!server_port)
    {
      response_handler.HandleException(tFutureStatus::NO_CONNECTION);
      return;
    }
    tRPCInterface* server_interface = server_port->GetCallHandler();
    if (server_interface)
    {
      try
      {
        response_handler.HandleResponse((static_cast<T*>(server_interface)->*function)(args...));
      }
      catch (const tRPCException& e)
      {
        response_handler.HandleException(e.GetType());
      }
      return;
    }

    // prepare storage object
    typename internal::tCallStorage::tPointer call_storage = internal::tCallStorage::GetUnused();
    internal::tRPCRequest<tReturn, TArgs...>& request =
      call_storage->Emplace<internal::tRPCRequest<tReturn, TArgs...>>(tRPCInterfaceType<T>::GetFunctionID(function), std::forward<TArgs>(args)...);

    request.SetResponseHandler(response_handler);
    server_port->SendCall(call_storage);
  }


  /*!
   * Calls specified function
   * This blocks until return value is available or timeout expires
   * Throws a tRPCException if port is not connected, the timeout expires or parameters are invalid
   *
   * \param timeout Timeout for function call
   * \param function Function to call
   * \param args Arguments for function call
   * \return Result of function call
   */
  template <typename TFunction, typename ... TArgs>
  typename tReturnType<TFunction>::type CallSynchronous(rrlib::time::tDuration timeout, TFunction function, TArgs && ... args)
  {
    typedef typename tReturnType<TFunction>::type tReturn;
    static_assert(!std::is_same<tReturn, void>::value, "Call plain Call() for functions without return value");

    internal::tRPCPort* server_port = GetWrapped()->GetServer(true);
    if (!server_port)
    {
      throw tRPCException(tFutureStatus::NO_CONNECTION);
    }
    tRPCInterface* server_interface = server_port->GetCallHandler();
    if (server_interface)
    {
      return (static_cast<T*>(server_interface)->*function)(args...);
    }

    // prepare storage object
    typename internal::tCallStorage::tPointer call_storage = internal::tCallStorage::GetUnused();
    internal::tRPCRequest<tReturn, TArgs...>& request =
      call_storage->Emplace<internal::tRPCRequest<tReturn, TArgs...>>(*call_storage, tRPCInterfaceType<T>::GetFunctionID(function), timeout, std::forward<TArgs>(args)...);

    // TODO: add handler that will notify condition variable on return etc.

    // send call and wait for call returning
    tFuture<tReturn> future = request.GetFuture();
    server_port->SendCall(call_storage);
    call_storage.reset();
    return future.Get(timeout);
  }

  /*!
   * Calls specified function and returns a tFuture<RETURN_TYPE>.
   * This tFuture can be used to obtain and possibly wait for the
   * return value when it is needed.
   *
   * \param function Function to call
   * \param args Arguments for function call
   */
  template <typename TFunction, typename ... TArgs>
  tFuture<typename tReturnType<TFunction>::type> FutureCall(TFunction function, TArgs && ... args)
  {
    typedef typename tReturnType<TFunction>::type tReturn;
    static_assert(!std::is_same<tReturn, void>::value, "Call plain Call() for functions without return value");

    // prepare storage object
    typename internal::tCallStorage::tPointer call_storage = internal::tCallStorage::GetUnused();

    internal::tRPCPort* server_port = GetWrapped()->GetServer(true);
    if (!server_port)
    {
      internal::tRPCResponse<tReturn>& response = call_storage->Emplace<internal::tRPCResponse<tReturn>>();
      call_storage->SetException(tFutureStatus::NO_CONNECTION);
      return response.GetFuture();
    }
    tRPCInterface* server_interface = server_port->GetCallHandler();
    if (server_interface)
    {
      internal::tRPCResponse<tReturn>& response = call_storage->Emplace<internal::tRPCResponse<tReturn>>(*call_storage);
      try
      {
        response->SetReturnValue((static_cast<T*>(server_interface)->*function)(args...));
      }
      catch (const tRPCException& e)
      {
        call_storage->SetException(e.GetType());
      }
      return response.GetFuture();
    }

    internal::tRPCRequest<tReturn, TArgs...>& request =
      call_storage->Emplace<internal::tRPCRequest<tReturn, TArgs...>>(tRPCInterfaceType<T>::GetFunctionID(function), std::forward<TArgs>(args)...);
    tFuture<tReturn> future = request.GetFuture();
    server_port->SendCall(call_storage);
    return future;
  }

  /*!
   * \return Wrapped RPC port
   */
  internal::tRPCPort* GetWrapped()
  {
    return static_cast<internal::tRPCPort*>(tPortWrapperBase::GetWrapped());
  }

  /*!
   * Wraps raw port
   * Throws std::runtime_error if port has invalid type
   *
   * \param wrap Type-less port to wrap as tClientPort<T>
   */
  static tClientPort Wrap(core::tAbstractPort& wrap)
  {
    if (wrap.GetDataType().GetRttiName() != typeid(T).name())
    {
      throw std::runtime_error("tClientPort<" + rrlib::rtti::Demangle(typeid(T).name()) + "> cannot wrap port with buffer type '" + wrap.GetDataType().GetName() + "'.");
    }
    tClientPort port;
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
