/*
 * Copyright 2001-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

# include "incls/_precompiled.incl"
# include "incls/_immutableSpace.cpp.incl"

void ImmutableSpace::initialize(MemRegion mr) {
  HeapWord* bottom = mr.start();
  HeapWord* end    = mr.end();

  assert(Universe::on_page_boundary(bottom) && Universe::on_page_boundary(end),
         "invalid space boundaries");

  _bottom = bottom;
  _end = end;
}

void ImmutableSpace::oop_iterate(OopClosure* cl) {
  HeapWord* obj_addr = bottom();
  HeapWord* t = end();
  // Could call objects iterate, but this is easier.
  while (obj_addr < t) {
    obj_addr += oop(obj_addr)->oop_iterate(cl);
  }
}

void ImmutableSpace::object_iterate(ObjectClosure* cl) {
  HeapWord* p = bottom();
  while (p < end()) {
    cl->do_object(oop(p));
    p += oop(p)->size();
  }
}

#ifndef PRODUCT

void ImmutableSpace::print_short() const {
  tty->print(" space " SIZE_FORMAT "K, 100%% used", capacity_in_bytes() / K);
}

void ImmutableSpace::print() const {
  print_short();
  tty->print_cr(" [%#-6lx,%#-6lx)", bottom(), end());
}

#endif

void ImmutableSpace::verify(bool allow_dirty) {
  HeapWord* p = bottom();
  HeapWord* t = end();
  HeapWord* prev_p = NULL;
  while (p < t) {
    oop(p)->verify();
    prev_p = p;
    p += oop(p)->size();
  }
  guarantee(p == end(), "end of last object must match end of space");
}
