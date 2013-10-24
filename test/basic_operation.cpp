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
/*!\file    plugins/rpc_ports/test/basic_operation.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 * Tests basic operation of RPC ports.
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tClientPort.h"
#include "plugins/rpc_ports/tServerPort.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------
using namespace finroc::rpc_ports;

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
class tTestInterface : public tRPCInterface
{
public:
  int Function(double d)
  {
    return 4;
  }

  virtual void Test()
  {
    FINROC_LOG_PRINT(USER, "Test() Called");
  }

  void StringTest(const std::string& string)
  {
    FINROC_LOG_PRINT(USER, "StringTest() called with '", string, "'");
  }
};

tRPCInterfaceType<tTestInterface> cTYPE("Test interface", &tTestInterface::Function, &tTestInterface::Test, &tTestInterface::StringTest);


int main(int, char**)
{
  tTestInterface test_interface;

  tClientPort<tTestInterface> client_port("Client port");
  tServerPort<tTestInterface> server_port(test_interface, "Server port");
  client_port.GetParent()->InitAll();
  client_port.ConnectTo(server_port);

  int m = client_port.CallSynchronous(std::chrono::seconds(2), &tTestInterface::Function, 4);
  FINROC_LOG_PRINT(USER, "Call returned ", m);
  client_port.Call(&tTestInterface::Test);
  client_port.Call(&tTestInterface::StringTest, "a string");
  return 0;
}

// some other experiments...

typedef double(*func)(int);

template <func T>
struct X {};
