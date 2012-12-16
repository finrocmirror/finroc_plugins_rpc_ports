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
    rrlib::rtti::tType(GetTypeInfo())
  {
    if (GetName().length() == 0)
    {
      FINROC_LOG_PRINT(ERROR, "RPC type needs to be instantiated with name and function once first.");
      abort();
    }
  }


  template <typename ... TFunctions>
  tRPCInterfaceType(const std::string& name, TFunctions ... functions) :
    rrlib::rtti::tType(GetTypeInfo(name))
  {
    if (GetTypeInfo()->methods.size() == 0)
    {
      RegisterFunctions<TFunctions...>(functions...);
    }
    else
    {
      FINROC_LOG_PRINT(ERROR, "Attempt to initialize RPC type twice.");
    }
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

  class tTypeInfo : public rrlib::rtti::tType::tInfo
  {
  public:
    tTypeInfo(const std::string& name)
    {
      this->type = rrlib::rtti::tType::tClassification::OTHER;
      this->name = name;
      this->rtti_name = typeid(T).name();
      this->default_name = false;
      binary = rrlib::rtti::GetBinaryCurrentlyPerformingStaticInitialization();
      if (binary.length() > 0)
      {
        RRLIB_LOG_PRINT_STATIC(DEBUG_VERBOSE_1, "RPC type ", name, " is statically loaded in '", binary, "'.");
      }
    }

    /*!
     * One such type-less entry exists for every registered method
     */
    struct tEntry
    {
      /*! function to deserialize this method from stream */
      internal::tDeserializeMessage deserialize_message;
      internal::tDeserializeRequest deserialize_request;
      internal::tDeserializeResponse deserialize_response;
    };

    /*!
     * Methods registered in this interface type
     */
    std::vector<tEntry> methods;
  };

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

  static tTypeInfo* GetTypeInfo(const std::string& name = "")
  {
    static tTypeInfo type_info(name);
    return &type_info;
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
    GetFunctionIDLookup<TFunction>().push_back(std::pair<TFunction, uint8_t>(function, static_cast<uint8_t>(GetTypeInfo()->methods.size())));
    typename tTypeInfo::tEntry entry =
    {
      GetDeserializeMessageFunction(function),
      GetDeserializeRequestFunction(function),
      GetDeserializeResponseFunction(function)
    };
    GetTypeInfo()->methods.emplace_back(entry);
  }

  template <typename TReturn, typename ... TArgs>
  internal::tDeserializeMessage GetDeserializeMessageFunction(TReturn(T::*function_pointer)(TArgs...))
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
  internal::tDeserializeResponse GetDeserializeResponseFunction(TReturn(T::*function_pointer)(TArgs...))
  {
    typedef typename std::conditional<std::is_same<TReturn, void>::value, internal::tNoRPCResponse, internal::tRPCResponse<TReturn>>::type tResponse;
    return &tResponse::template DeserializeAndExecuteCallImplementation<T>;
  }


};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
