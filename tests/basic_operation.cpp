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
/*!\file    plugins/rpc_ports/tests/basic_operation.cpp
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
#include "rrlib/util/tUnitTestSuite.h"

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
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
static bool test_called = false;
static std::string string_test_called_with = "";

class tTestInterface : public tRPCInterface
{
public:
  int Function(double d) const
  {
    return 4 * d;
  }

  virtual void Test()
  {
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Test() Called");
    test_called = true;
  }

  void StringTest(const std::string& string)
  {
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "StringTest() called with '", string, "'");
    string_test_called_with = string;
  }
};

tRPCInterfaceType<tTestInterface> cTYPE("Test interface", &tTestInterface::Function, &tTestInterface::Test, &tTestInterface::StringTest);


class BasicOperationTest : public rrlib::util::tUnitTestSuite
{
  RRLIB_UNIT_TESTS_BEGIN_SUITE(BasicOperationTest);
  RRLIB_UNIT_TESTS_ADD_TEST(Test);
  RRLIB_UNIT_TESTS_END_SUITE;

  void Test()
  {
    tTestInterface test_interface;

    tClientPort<tTestInterface> client_port("Client port");
    tServerPort<tTestInterface> server_port(test_interface, "Server port");
    client_port.GetParent()->InitAll();
    client_port.ConnectTo(server_port);

    int m = client_port.CallSynchronous(std::chrono::seconds(2), &tTestInterface::Function, 4);
    RRLIB_UNIT_TESTS_EQUALITY(m, 16);
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Call returned ", m);
    client_port.Call(&tTestInterface::Test);
    RRLIB_UNIT_TESTS_ASSERT(test_called);
    client_port.Call(&tTestInterface::StringTest, "a string");
    RRLIB_UNIT_TESTS_EQUALITY(string_test_called_with, std::string("a string"));
  }
};

RRLIB_UNIT_TESTS_REGISTER_SUITE(BasicOperationTest);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
