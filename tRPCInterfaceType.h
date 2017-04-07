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
/*!\file    plugins/rpc_ports/tRPCInterfaceType.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 * \brief   Contains tRPCInterfaceType
 *
 * \b tRPCInterfaceType
 *
 * RPC interface type.
 * Need to be initialized once so that rrlib::rtti knows it
 * (similar to rrlib::rtti::tDataType).
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tRPCInterfaceType_h__
#define __plugins__rpc_ports__tRPCInterfaceType_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tClientPort.h"
#include "plugins/rpc_ports/internal/tRPCInterfaceTypeInfo.h"
#include "plugins/rpc_ports/internal/tRPCMessage.h"
#include "plugins/rpc_ports/internal/tRPCRequest.h"
#include "plugins/rpc_ports/internal/tRPCResponse.h"

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
//! RPC interface type
/*!
 * RPC interface type.
 * Need to be initialized once so that rrlib::rtti knows this interface type
 * (similar to rrlib::rtti::tDataType).
 */
template <typename T>
class tRPCInterfaceType : public rrlib::rtti::tType
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRPCInterfaceType() :
    rrlib::rtti::tType(&cTYPE_INFO)
  {
    if (GetName().length() == 0)
    {
      FINROC_LOG_PRINT(ERROR, "RPC type needs to be instantiated with name and function once first.");
      abort();
    }
  }


  template <size_t Tchars, typename ... TFunctions>
  tRPCInterfaceType(const char(&name)[Tchars], TFunctions ... functions) :
    rrlib::rtti::tType(&cTYPE_INFO)
  {
    GetSharedInfo().SetName(rrlib::util::tManagedConstCharPointer(name, false), &cTYPE_INFO);
    RegisterFunctions<TFunctions...>(functions...);
  }

  template <typename ... TFunctions>
  tRPCInterfaceType(const std::string& name, TFunctions ... functions) :
    rrlib::rtti::tType(&cTYPE_INFO)
  {
    GetSharedInfo().SetName(rrlib::util::tManagedConstCharPointer(name.c_str(), true), &cTYPE_INFO);
    RegisterFunctions<TFunctions...>(functions...);
  }

  /*!
   * \function_id Id of function
   * \return Returns function with specified id
   */
  template <typename TFunction>
  static TFunction GetFunction(uint8_t function_id)
  {
    std::vector<std::pair<TFunction, uint8_t>>& lookup = GetFunctionIDLookup<TFunction>();
    for (auto it = lookup.begin(); it != lookup.end(); ++it)
    {
      if (it->second == function_id)
      {
        return it->first;
      }
    }
    throw std::runtime_error("Function lookup failed: no such function");
  }

  /*!
   * Looks up function id for specified function
   *
   * \param function Function whose id is to be looked up
   * \return Function id in this RPC interface type
   */
  template <typename TFunction>
  static uint8_t GetFunctionID(TFunction function)
  {
    std::vector<std::pair<TFunction, uint8_t>>& lookup = GetFunctionIDLookup<TFunction>();
    for (auto it = lookup.begin(); it != lookup.end(); ++it)
    {
      if (it->first == function)
      {
        return it->second;
      }
    }
    throw std::runtime_error("Function is not part of tRPCInterfaceType<T>");
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Shared Type Info for RPC Type */
  static internal::tRPCInterfaceTypeInfo shared_info;

  /*! Type info for RPC type */
  static constexpr rrlib::rtti::detail::tTypeInfo cTYPE_INFO = { typeid(T), rrlib::rtti::TypeTraitsVector<T>::value | rrlib::rtti::trait_flags::cIS_RPC_TYPE, &shared_info, sizeof(T) };

  typedef typename internal::tRPCInterfaceTypeInfo::tEntry tEntry;

  tRPCInterfaceType(bool) : rrlib::rtti::tType(&cTYPE_INFO)
  {
  }

  /*!
   * \return Vector for methods registered in this interface type
   */
  static std::vector<internal::tRPCInterfaceTypeInfo::tEntry>& GetMethodsVector()
  {
    static std::vector<internal::tRPCInterfaceTypeInfo::tEntry> vector;
    return vector;
  }

  /*!
   * \return Function ids for specified function type.
   * List contains all registered functions of this type - together with their id
   */
  template <typename TFunction>
  static std::vector<std::pair<TFunction, uint8_t>>& GetFunctionIDLookup()
  {
    static std::vector<std::pair<TFunction, uint8_t>> lookup;
    return lookup;
  }

  void RegisterFunctions() {}

  template <typename TFunction, typename ... TFunctions>
  void RegisterFunctions(TFunction function, TFunctions ... functions)
  {
    RegisterFunction<TFunction>(function);
    RegisterFunctions(functions...);
  }

  template <typename TFunction>
  void RegisterFunction(TFunction function)
  {
    GetFunctionIDLookup<TFunction>().push_back(std::pair<TFunction, uint8_t>(function, static_cast<uint8_t>(GetMethodsVector().size())));
    tEntry entry =
    {
      GetDeserializeMessageFunction(function),
      GetDeserializeRequestFunction(function),
      GetDeserializeResponseFunction(function)
    };
    GetMethodsVector().emplace_back(entry);
  }

  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeMessage GetDeserializeMessageFunction(TReturn(T::*function_pointer)(TArgs...))
  {
    return &internal::tRPCMessage<TArgs...>::template DeserializeAndExecuteCallImplementation<TReturn, T>;
  }
  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeMessage GetDeserializeMessageFunction(TReturn(T::*function_pointer)(TArgs...) const)
  {
    return &internal::tRPCMessage<TArgs...>::template DeserializeAndExecuteCallImplementation<TReturn, T>;
  }

  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeRequest GetDeserializeRequestFunction(TReturn(T::*function_pointer)(TArgs...))
  {
    typedef typename std::conditional<std::is_same<TReturn, void>::value, internal::tNoRPCRequest, internal::tRPCRequest<TReturn, TArgs...>>::type tRequest;
    return &tRequest::template DeserializeAndExecuteCallImplementation<T>;
  }
  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeRequest GetDeserializeRequestFunction(TReturn(T::*function_pointer)(TArgs...) const)
  {
    typedef typename std::conditional<std::is_same<TReturn, void>::value, internal::tNoRPCRequest, internal::tRPCRequest<TReturn, TArgs...>>::type tRequest;
    return &tRequest::template DeserializeAndExecuteCallImplementation<T>;
  }

  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeResponse GetDeserializeResponseFunction(TReturn(T::*function_pointer)(TArgs...))
  {
    typedef typename std::conditional<std::is_same<TReturn, void>::value, internal::tNoRPCResponse, internal::tRPCResponse<TReturn>>::type tResponse;
    return &tResponse::DeserializeAndExecuteCallImplementation;
  }
  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeResponse GetDeserializeResponseFunction(TReturn(T::*function_pointer)(TArgs...) const)
  {
    typedef typename std::conditional<std::is_same<TReturn, void>::value, internal::tNoRPCResponse, internal::tRPCResponse<TReturn>>::type tResponse;
    return &tResponse::DeserializeAndExecuteCallImplementation;
  }

};

template <typename T>
constexpr rrlib::rtti::detail::tTypeInfo tRPCInterfaceType<T>::cTYPE_INFO;

template <typename T>
internal::tRPCInterfaceTypeInfo tRPCInterfaceType<T>::shared_info(&tRPCInterfaceType<T>::cTYPE_INFO, &tRPCInterfaceType<typename rrlib::rtti::UnderlyingType<T>::type>::cTYPE_INFO, rrlib::rtti::TypeName<T>::value, tRPCInterfaceType<T>::GetMethodsVector(), tRPCInterfaceType<T>(true));

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
