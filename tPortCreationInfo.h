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
/*!\file    plugins/rpc_ports/tPortCreationInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-01
 *
 * \brief   Contains tPortCreationInfo
 *
 * \b tPortCreationInfo
 *
 * This class bundles various parameters for the creation of RPC ports.
 *
 * Instead of providing suitable constructors for all types of sensible
 * combinations of the numerous (often optional) construction parameters,
 * there is only one constructor taking a single argument of this class.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__rpc_ports__tPortCreationInfo_h__
#define __plugins__rpc_ports__tPortCreationInfo_h__

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

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Bundle of port creation parameters
/*!
 * This class bundles various parameters for the creation of RPC ports.
 *
 * Instead of providing suitable constructors for all types of sensible
 * combinations of the numerous (often optional) construction parameters,
 * there is only one constructor taking a single argument of this class.
 */
template <typename T>
class tPortCreationInfo : public core::tAbstractPortCreationInfo
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Pointer to object that handles calls on server side */
  T* call_handler;

  tPortCreationInfo() {}

  /*!
   * Constructor takes variadic argument list... just any properties you want to assign to port.
   *
   * The first string is interpreted as port name, the second possibly as config entry (relevant for parameters only).
   * A framework element pointer is interpreted as parent.
   * tFrameworkElementFlag arguments are interpreted as flags.
   * tAbstractPortCreationInfo argument is copied. This is only allowed as first argument.
   * T& is interpreted as object that handles calls on server side
   */
  template <typename ARG1, typename ... TArgs>
  explicit tPortCreationInfo(ARG1 && arg1, TArgs && ... rest) :
    call_handler(NULL)
  {
    ProcessFirstArg<ARG1>(arg1);
    ProcessArgs(rest...);
  }

  /*! Various set methods for different port properties */
  void Set(core::tFrameworkElement* parent)
  {
    this->parent = parent;
  }

  void Set(const char* c)
  {
    this->name = c;
  }

  void Set(const tString& s)
  {
    this->name = s;
  }

  void Set(core::tFrameworkElement::tFlags flags)
  {
    this->flags |= flags;
  }

  void Set(const rrlib::rtti::tType& dt)
  {
    this->data_type = dt;
  }

  void Set(T& call_handler)
  {
    this->call_handler = &call_handler;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*!
   * Processes first argument (only here tPortCreationInfoBase argument is allowed)
   */
  template <typename A>
  void ProcessFirstArg(typename std::enable_if<std::is_base_of<tAbstractPortCreationInfo, A>::value, A>::type && a)
  {
    (*this) = a;
  }

  template <typename A>
  void ProcessFirstArg(typename std::enable_if < !std::is_base_of<tAbstractPortCreationInfo, A>::value, A >::type && a)
  {
    ProcessArg<A>(a);
  }

  /*! Process constructor arguments */
  void ProcessArgs() {}

  template <typename A, typename ... ARest>
  void ProcessArgs(A && arg, ARest && ... args)
  {
    ProcessArg<A>(arg);
    ProcessArgs(args...);
  }

  /*! Process single constructor argument */
  template <typename A>
  void ProcessArg(A && arg)
  {
    // standard case
    Set(arg);
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
