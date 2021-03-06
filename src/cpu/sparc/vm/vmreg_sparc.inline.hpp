/*
 * Copyright 2006-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 */

inline VMReg RegisterImpl::as_VMReg() {
  if( this==noreg ) return VMRegImpl::Bad();
  return VMRegImpl::as_VMReg(encoding() << 1 );
}

inline VMReg FloatRegisterImpl::as_VMReg() { return VMRegImpl::as_VMReg( ConcreteRegisterImpl::max_gpr + encoding() ); }


inline bool VMRegImpl::is_Register() { return value() >= 0 && value() < ConcreteRegisterImpl::max_gpr; }
inline bool VMRegImpl::is_FloatRegister() { return value() >= ConcreteRegisterImpl::max_gpr &&
                                                   value() < ConcreteRegisterImpl::max_fpr; }
inline Register VMRegImpl::as_Register() {

  assert( is_Register() && is_even(value()), "even-aligned GPR name" );
  // Yuk
  return ::as_Register(value()>>1);
}

inline FloatRegister VMRegImpl::as_FloatRegister() {
  assert( is_FloatRegister(), "must be" );
  // Yuk
  return ::as_FloatRegister( value() - ConcreteRegisterImpl::max_gpr );
}

inline   bool VMRegImpl::is_concrete() {
  assert(is_reg(), "must be");
  int v = value();
  if ( v  <  ConcreteRegisterImpl::max_gpr ) {
    return is_even(v);
  }
  // F0..F31
  if ( v <= ConcreteRegisterImpl::max_gpr + 31) return true;
  if ( v <  ConcreteRegisterImpl::max_fpr) {
    return is_even(v);
  }
  assert(false, "what register?");
  return false;
}
