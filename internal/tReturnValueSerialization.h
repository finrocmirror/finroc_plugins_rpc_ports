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
/*!\file    plugins/rpc_ports/internal/tReturnValueSerialization.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-31
 *
 * \brief   Contains tReturnValueSerialization
 *
 * \b tReturnValueSerialization
 *
 * Handles serialization of return values of RPC calls.
 * This is a little complex due to the handling of promises.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__internal__tReturnValueSerialization_h__
#define __plugins__rpc_ports__internal__tReturnValueSerialization_h__

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
namespace internal
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Return value serialization
/*!
 * Handles serialization of return values of RPC calls.
 * This is a little complex due to the handling of promises.
 */
template <typename TReturn, bool PROMISE, bool SERIALIZABLE>
struct tReturnValueSerialization
{
  inline static void Serialize(rrlib::serialization::tOutputStream& stream, TReturn& return_value, tCallStorage& storage)
  {
    stream << return_value;
  }

  inline static void Deserialize(rrlib::serialization::tInputStream& stream, TReturn& return_value, tRPCPort& port, uint8_t function_index)
  {
    stream >> return_value;
  }
};

// Plain promise
template <typename TReturn>
struct tReturnValueSerialization<TReturn, true, false>
{
  inline static void Serialize(rrlib::serialization::tOutputStream& stream, TReturn& return_value, tCallStorage& storage)
  {
    stream << storage.GetCallId();
  }

  inline static void Deserialize(rrlib::serialization::tInputStream& stream, TReturn& return_value, tRPCPort& port, uint8_t function_index)
  {
    tCallId call_id;
    stream >> call_id;
    return_value.SetRemotePromise(function_index, call_id, port);
  }
};

// Class derived from promise
template <typename TReturn>
struct tReturnValueSerialization<TReturn, true, true>
{
  inline static void Serialize(rrlib::serialization::tOutputStream& stream, TReturn& return_value, tCallStorage& storage)
  {
    stream << storage.GetCallId();
    stream << return_value;
  }

  inline static void Deserialize(rrlib::serialization::tInputStream& stream, TReturn& return_value, tRPCPort& port, uint8_t function_index)
  {
    tCallId call_id;
    stream >> call_id;
    return_value.SetRemotePromise(function_index, call_id, port);
    stream >> return_value;
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
