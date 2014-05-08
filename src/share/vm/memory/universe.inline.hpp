/*
 * Copyright 1997-2003 Sun Microsystems, Inc.  All Rights Reserved.
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

// Check whether an element of a typeArrayOop with the given type must be
// aligned 0 mod 8.  The typeArrayOop itself must be aligned at least this
// strongly.

inline bool Universe::element_type_should_be_aligned(BasicType type) {
  return type == T_DOUBLE || type == T_LONG;
}

// Check whether an object field (static/non-static) of the given type must be aligned 0 mod 8.

inline bool Universe::field_type_should_be_aligned(BasicType type) {
  return type == T_DOUBLE || type == T_LONG;
}
inline bool Universe::isFoundTransClass(){return _trans_class_found;}
inline void Universe::setFoundTransClass(){ _trans_class_found=true;}
inline bool Universe::isFoundTCPAcceptThread(){return _transParentThreadID!=-1;}
 
inline int Universe::getTCPAcceptThreadID() {return _transParentThreadID;}

inline void Universe::setTCPAcceptThreadID(int id){ if(id>-1)_transParentThreadID=id;}

inline int Universe::getNewTransID(){
	//return ++_newTransID; //this is ungarded version, now we need to be more stable.
	
	//we need to increase the newTransID by 1
	  do {
    //HeapWord* obj = top();
    int oldTransID=_newTransID;
    int newTransID=oldTransID+1;
    int result=Atomic::cmpxchg(newTransID, &_newTransID,oldTransID);
     if (result == oldTransID) {
        return oldTransID+1;
      }
  } while (true);

};//tony FIXME:need a lock-free update.
  
inline void Universe::add_remote_obj_count(int val)   { _remote_obj_count += val ;_total_obj_count += val;  }
inline void Universe::add_local_obj_count(int val)    { _local_obj_count += val ;_total_obj_count += val; }
inline int  Universe::remote_obj_count()              { return _remote_obj_count; }
inline int  Universe::local_obj_count()               { return _local_obj_count; }

inline void Universe::add_remote_obj_bytes(int val)   { _remote_obj_bytes += val ;_total_obj_bytes += val;}
inline void Universe::add_local_obj_bytes(int val)    { _local_obj_bytes += val ;_total_obj_bytes += val;}
inline unsigned long long  Universe::remote_obj_bytes()              { return _remote_obj_bytes; }
inline unsigned long long  Universe::local_obj_bytes()               { return _local_obj_bytes; }

inline void Universe::add_total_obj_bytes(int val) { _total_obj_bytes += val; }
inline void Universe::add_total_obj_count(int val) { _total_obj_count += val; }
inline unsigned long long  Universe::total_obj_bytes() { return _total_obj_bytes;  }
inline int  Universe::total_obj_count() { return _total_obj_count; }

inline void Universe::add_total_residual_count(int val) { _total_obj_bytes += val; }
inline int Universe::total_residual_count() { return _total_obj_bytes ; }

inline bool Universe::isTrainingDone(){return _training_done;}
inline void Universe::setTrainingDone(){_training_done=true;}

/*-tony 01/18/2010 */
 

