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

// Methods of protected closure types

 /*tony 2010-06-14 TransGCDefNew, Tony*/ 
 /*-tony 2010-06-14 */
template <class T>
inline void TransNewGeneration::KeepAliveClosure::do_oop_work(T* p) {
#ifdef ASSERT
  {
    // We never expect to see a null reference being processed
    // as a weak reference.
    assert (!oopDesc::is_null(*p), "expected non-null ref");
    oop obj = oopDesc::load_decode_heap_oop_not_null(p);
    assert (obj->is_oop(), "expected an oop while scanning weak refs");
  }
#endif // ASSERT

  _cl->do_oop_nv(p);

  // Card marking is trickier for weak refs.
  // This oop is a 'next' field which was filled in while we
  // were discovering weak references. While we might not need
  // to take a special action to keep this reference alive, we
  // will need to dirty a card as the field was modified.
  //
  // Alternatively, we could create a method which iterates through
  // each generation, allowing them in turn to examine the modified
  // field.
  //
  // We could check that p is also in an older generation, but
  // dirty cards in the youngest gen are never scanned, so the
  // extra check probably isn't worthwhile.
  if (Universe::heap()->is_in_reserved(p)) {
    oop obj = oopDesc::load_decode_heap_oop_not_null(p);
    _rs->inline_write_ref_field_gc(p, obj);
  }
}

template <class T>
inline void TransNewGeneration::FastKeepAliveClosure::do_oop_work(T* p) {
#ifdef ASSERT
  {
    // We never expect to see a null reference being processed
    // as a weak reference.
    assert (!oopDesc::is_null(*p), "expected non-null ref");
    oop obj = oopDesc::load_decode_heap_oop_not_null(p);
    assert (obj->is_oop(), "expected an oop while scanning weak refs");
  }
#endif // ASSERT

//if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]FastKeepAliveClosure::do_oop_work(%p)",p);}

  _cl->do_oop_nv(p);

  // Optimized for Defnew generation if it's the youngest generation:
  // we set a younger_gen card if we have an older->youngest
  // generation pointer.
  oop obj = oopDesc::load_decode_heap_oop_not_null(p);

//if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:]Now check if we should dirty the card)");}

   /*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
  if (UseTypeGenGC){
   if ( Universe::heap()->is_in_reserved(p) ){
     int field_gid = Universe::heap()->addr_to_gen_id(p);
     int value_gid = Universe::heap()->addr_to_gen_id(obj);

     if ( value_gid < field_gid )
      {  _rs->inline_write_ref_field_gc(p, obj); 
	//	if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:]inline_write_ref_field_gc-gen(field_gid,value_gid)");}

	  }
     else if (field_gid==0 && value_gid==1 )
      {  _rs->inline_write_ref_field_l2r_gc(p, obj);  

		//if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:]inline_write_ref_field_l2r_gc-gen(0,1)");}

	  }
	  //tony-> 2010-08-19 Collecting transEdens
	 else if(field_gid==value_gid&&field_gid==1)//both in remote young
	 	{
	 	//see if they belong to different transEden
	 	int peid=Universe::heap()->get_trans_eden_id(p);
		int oeid=Universe::heap()->get_trans_eden_id(obj);
	 	if(peid!=oeid)
	 		{
			_rs->inline_write_ref_field_l2r_gc(p, obj);	
			//if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:]inline_write_ref_field_l2r_gc-diff-eden(%d,%d)",peid,oeid);}
	 		}
	 	}
	  //<-tony 2010-08-19/
	  
   }
  }
  else{

   /*-tony 02/06/2010 */
   
  if (((HeapWord*)obj < _boundary) && Universe::heap()->is_in_reserved(p)) {
    _rs->inline_write_ref_field_gc(p, obj);
  }

  }
}
