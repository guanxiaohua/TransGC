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

inline OopsInGenClosure::OopsInGenClosure(Generation* gen) :
  OopClosure(gen->ref_processor()), _orig_gen(gen), _rs(NULL),_lower_boundary(gen->reserved().start()),_upper_boundary(gen->reserved().end())
 {
 
  set_generation(gen);
}

inline void OopsInGenClosure::set_generation(Generation* gen) {
  _gen = gen;
  _gen_boundary = _gen->reserved().start();

//	if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]OopsInGenClosure::set_generation(%d), _lower_boundary=%p,_upper_boundary=%p",gen->level(),_lower_boundary,_upper_boundary);}
 
  // Barrier set for the heap, must be set after heap is initialized
  if (_rs == NULL) {
    GenRemSet* rs = SharedHeap::heap()->rem_set();
    assert(rs->rs_kind() == GenRemSet::CardTable, "Wrong rem set kind");
    _rs = (CardTableRS*)rs;
  }
}

inline  void OopsInGenClosure::reset_generation() { _gen = _orig_gen; 
 }


template <class T> inline void OopsInGenClosure::do_barrier(T* p) {
	
//note: generation() =_gen, it's the generation that the pointer comes from
//_g, or the (lower_boundary,upper_boundary) is the space that we are scavenging. //by tony 2010-08-26
if(!generation()->is_in_reserved(p)&&DebugTypeGenGC)
  	{
		gclog_or_tty->print_cr("[TONY]p(%p) should be in gen(%d), but it actually is in gen(%d)", p,generation()->level(),Universe::heap()->addr_to_gen_id(p));
	}
  assert(generation()->is_in_reserved(p), "expected ref in generation");

  assert(!oopDesc::is_null(*p), "expected non-null object");
  oop obj = oopDesc::load_decode_heap_oop_not_null(p);
  
// if(DebugTypeGenGC)
 //	{gclog_or_tty->print_cr("[OopsInGenClosure::do_barrier]-enter gen(%d)->gen(%d)",Universe::heap()->addr_to_gen_id(p),Universe::heap()->addr_to_gen_id(obj));}


  // If p points to a younger generation, mark the card. 
  if ((HeapWord*)obj < _gen_boundary) {
    _rs->inline_write_ref_field_gc(p, obj);
	//if(DebugTypeGenGC){gclog_or_tty->print_cr("[OopsInGenClosure::do_barrier]old->young");}

  }

 /*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
  // If p is in local young space (gen0), and it points to remote young space (gen1), mark the card. Feng
  if (UseTypeGenGC){

	    if(generation()->level()==0 && Universe::heap()->addr_to_gen_id(obj)==1)
    		 {   _rs->inline_write_ref_field_l2r_gc(p, obj);  
		//	if(DebugTypeGenGC){gclog_or_tty->print_cr("[OopsInGenClosure::do_barrier] gen0->gen1");}
				}
		if(generation()->level()==1 && Universe::heap()->addr_to_gen_id(obj)==1)
			{	//see if they belong to different transEden
			 	int peid=Universe::heap()->get_trans_eden_id(p);
				int oeid=Universe::heap()->get_trans_eden_id(obj);
			 	if(peid!=oeid)
		 		{
				_rs->inline_write_ref_field_l2r_gc(p, obj);	
			//	if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:]inline_write_ref_field_l2r_gc(%d,%d)",peid,oeid);}
			//	if(DebugTypeGenGC){gclog_or_tty->print_cr("[OopsInGenClosure::do_barrier] transEden->transEden");}

		 		}
			}
	  //<-tony 2010-08-19/
	  

  }
}

inline void OopsInGenClosure::par_do_barrier(oop* p) {
  assert(generation()->is_in_reserved(p), "expected ref in generation");
  assert(false,"OopsInGenClosure::par_do_barrier gets called, trace!");//by tony 2010-08-24
  oop obj = *p;
  assert(obj != NULL, "expected non-null object");
  // If p points to a younger generation, mark the card.
  if ((HeapWord*)obj < gen_boundary()) {
    rs()->write_ref_field_gc_par(p, obj);
  }
}

// NOTE! Any changes made here should also be made
// in FastScanClosure::do_oop_work()
template <class T> inline void ScanClosure::do_oop_work(T* p) {
  T heap_oop = oopDesc::load_heap_oop(p);
  // Should we copy the obj?
  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);
//    if ((HeapWord*)obj < _boundary) {
//  if( _g->is_in_reserved(obj) ){ //by tony
 if((HeapWord*)obj>=lower_boundary()&&(HeapWord*)obj<upper_boundary()){ //by tony 2010-08-26
/*tony 2010-06-14 TransGCDefNew, Tony*/ 
//      assert(!_g->to()->is_in_reserved(obj), "Scanning field twice?");
//      oop new_obj = obj->is_forwarded() ? obj->forwardee()
  //                                      : _g->copy_to_survivor_space(obj);
		 oop new_obj=NULL;
		 bool forwarded=obj->is_forwarded();
			
		if(_g->kind()!=Generation::TransNew)
			{
			// defNew
		      assert(!((DefNewGeneration*)_g)->to()->is_in_reserved(obj), "Scanning field twice?");

			
			  new_obj = obj->is_forwarded() ? obj->forwardee()
                                        : ((DefNewGeneration*)_g)->copy_to_survivor_space(obj);
/*
			  	if(DebugTypeGenGC)
			   	{
			   	if(!forwarded)
				   	gclog_or_tty->print_cr("[TONY]defNew-ScanClosure:copied the object(%p) -pointed BY %p from gen(%d)-to %p gen(%d)!", obj,p,Universe::heap()->addr_to_gen_id(p),new_obj,Universe::heap()->addr_to_gen_id(new_obj));
				}
				*/
			}
		else
			{
			//transNew
			  //assert(!((TransNewGeneration*)_g)->to()->is_in_reserved(obj), "Scanning field twice?");
			   //We don't have to space any more, but we still do the copy_to_survivor_space, since it's promotion actually.

			
			  new_obj = obj->is_forwarded() ? obj->forwardee()
                                        : ((TransNewGeneration*)_g)->copy_to_survivor_space(obj);
/*
			  if(DebugTypeGenGC)
			   	{
			   	if(!forwarded)
				   	gclog_or_tty->print_cr("[TONY]trans-ScanClosure: copied the object(%p) in transEden(%d) -pointed BY %p from gen(%d)-to %p gen(%d)!", obj,Universe::heap()->get_trans_eden_id(obj),p,Universe::heap()->addr_to_gen_id(p),new_obj,Universe::heap()->addr_to_gen_id(new_obj));
				}
	*/		
			
			}
/*-tony 2010-06-14 */

      oopDesc::encode_store_heap_oop_not_null(p, new_obj);
 	}
    if (_gc_barrier) {
      // Now call parent closure
      do_barrier(p);
    }
  }
}

inline void ScanClosure::do_oop_nv(oop* p)       { ScanClosure::do_oop_work(p); }
inline void ScanClosure::do_oop_nv(narrowOop* p) { ScanClosure::do_oop_work(p); }

// NOTE! Any changes made here should also be made
// in ScanClosure::do_oop_work()
template <class T> inline void FastScanClosure::do_oop_work(T* p) {
  T heap_oop = oopDesc::load_heap_oop(p);
  // Should we copy the obj?
  oop new_obj=NULL;
  if (!oopDesc::is_null(heap_oop)){
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);
//    if ((HeapWord*)obj < _boundary) {
//  if( _g->is_in_reserved(obj) ){ //by tony
if((HeapWord*)obj>=lower_boundary()&&(HeapWord*)obj<upper_boundary()){ 
	//by tony 2010-08-26, p should not in the same mem area as the obj
//if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]FastScanClosure::do_oop_work,p=%p,lower_boundary=%p,upper_boundary=%p,gen.start=%p,gen.end=%p,_g.start=%p,_g.end=%p",p,lower_boundary(),upper_boundary(),generation()->reserved().start(),generation()->reserved().end(),_g->reserved().start(),_g->reserved().start());}

/*tony 2010-06-14 TransGCDefNew, Tony*/ 
//      assert(!_g->to()->is_in_reserved(obj), "Scanning field twice?");
//      oop new_obj = obj->is_forwarded() ? obj->forwardee()
  //                                      : _g->copy_to_survivor_space(obj);
	  bool forwarded=obj->is_forwarded();
			
		if(_g->kind()!=Generation::TransNew)
			{
			// defNew
		      assert(!((DefNewGeneration*)_g)->to()->is_in_reserved(obj), "Scanning field twice?");
			  new_obj = obj->is_forwarded() ? obj->forwardee()
                                        : ((DefNewGeneration*)_g)->copy_to_survivor_space(obj);			  
/*
			  if(DebugTypeGenGC)
			   	{
			   	if(!forwarded){
				   	gclog_or_tty->print_cr("[TONY]FSC:copyin the object(%p) -pointed BY %p from gen(%d)-to %p gen(%d)!", obj,p,Universe::heap()->addr_to_gen_id(p),new_obj,Universe::heap()->addr_to_gen_id(new_obj));
				   	gclog_or_tty->print_cr("[TONY]DefNew-FSC:copyied object-pointed from gen(%d) to gen(%d)!", Universe::heap()->addr_to_gen_id(p),Universe::heap()->addr_to_gen_id(new_obj));
			   		}
				}
				*/
			}
		else
			{
			//transNew
			  //assert(!((TransNewGeneration*)_g)->to()->is_in_reserved(obj), "Scanning field twice?");
			   //We don't have to space any more, but we still do the copy_to_survivor_space, since it's promotion actually.

			  new_obj = obj->is_forwarded() ? obj->forwardee()
                                        : ((TransNewGeneration*)_g)->copy_to_survivor_space(obj);
			  if(DebugTypeGenGC)
			   	{
			   	if(!forwarded){
					//tony-> 2011/01/16 check the residuals' demographics. 
			  	 if(ResidualDemographics){
						 ( (TransNewGeneration*)_g)->getTransEden(Universe::heap()->get_trans_eden_id(obj))->residualStat(obj,(HeapWord*)p);
			  	 	}
				  //<-tony 2011-1-16
				  #if 0
				  // 	gclog_or_tty->print_cr("[TONY]FastScanClosure:copyin the object(%p) in transEden(%d) -pointed BY %p from gen(%d)-to %p gen(%d)!", obj,Universe::heap()->get_trans_eden_id(obj),p,Universe::heap()->addr_to_gen_id(p),new_obj,Universe::heap()->addr_to_gen_id(new_obj));
				    int transedenid=Universe::heap()->get_trans_eden_id(obj);

				   	//gclog_or_tty->print_cr("[TONY]Trans-FSC:copied object in transEden(%d)pointed from gen(%d)!", transedenid,Universe::heap()->addr_to_gen_id(p));


					int id=Universe::heap()->get_trans_eden_id(p);
//					( (TransNewGeneration*)_g)->getTransEden(transedenid)
					if(id!=-1)
					   	gclog_or_tty->print_cr("[TONY]Trans-FSC:pointed from transEden(%d)!", id);
					#endif
			   		}				
			  	}
			
			
			}
/*-tony 2010-06-14 */

	
      oopDesc::encode_store_heap_oop_not_null(p, new_obj);
      if (_gc_barrier) {
        // Now call parent closure
        do_barrier(p);
      	}
    }

	    //tony-> 2010-08-31 Fixing transEdens collector
	    //check the barrier again!
      if (_gc_barrier) {
        // Now call parent closure
        do_barrier(p);
      	}
	 //<-tony 2010-08-31
  }
}

inline void FastScanClosure::do_oop_nv(oop* p)       { FastScanClosure::do_oop_work(p); }
inline void FastScanClosure::do_oop_nv(narrowOop* p) { FastScanClosure::do_oop_work(p); }

// Note similarity to ScanClosure; the difference is that
// the barrier set is taken care of outside this closure.
template <class T> inline void ScanWeakRefClosure::do_oop_work(T* p) {
  assert(!oopDesc::is_null(*p), "null weak reference?");
  oop obj = oopDesc::load_decode_heap_oop_not_null(p);
  // weak references are sometimes scanned twice; must check
  // that to-space doesn't already contain this object  

   //tony-> 2010-08-26 Fixing transEdens collector
		 oop new_obj=NULL;
		 bool forwarded=obj->is_forwarded();
			
		if(_g->kind()!=Generation::TransNew){
			if((HeapWord*)obj < _boundary&&!((DefNewGeneration*)_g)->to()->is_in_reserved(obj))
			{
			// defNew
			
			  new_obj = obj->is_forwarded() ? obj->forwardee()
                                        : ((DefNewGeneration*)_g)->copy_to_survivor_space(obj);
/*
				if(DebugTypeGenGC)
			   	{
			   	
			   	if(!forwarded)
				   	gclog_or_tty->print_cr("[TONY]ScanWeakRefClosure:copyin the object(%p) -pointed BY %p from gen(%d)-to %p gen(%d)!", obj,p,Universe::heap()->addr_to_gen_id(p),new_obj,Universe::heap()->addr_to_gen_id(new_obj));
				}

	*/		
			      oopDesc::encode_store_heap_oop_not_null(p, new_obj);	

			}
			}
		else
			{
			//transNew
			  //assert(!((TransNewGeneration*)_g)->to()->is_in_reserved(obj), "Scanning field twice?");
			   //We don't have to space any more, but we still do the copy_to_survivor_space, since it's promotion actually.
			  if((HeapWord*)obj>=_lower_boundary&&(HeapWord*)obj<_upper_boundary){ //by tony 2010-08-26

			
			  new_obj = obj->is_forwarded() ? obj->forwardee()
                                        : ((TransNewGeneration*)_g)->copy_to_survivor_space(obj);

/*
			  if(DebugTypeGenGC)
			   	{
			   	if(!forwarded)
				   	gclog_or_tty->print_cr("[TONY]ScanWeakRefClosure: copyin the object(%p) in transEden(%d) -pointed BY %p from gen(%d)-to %p gen(%d)!", obj,Universe::heap()->get_trans_eden_id(obj),p,Universe::heap()->addr_to_gen_id(p),new_obj,Universe::heap()->addr_to_gen_id(new_obj));
				}
*/
				      oopDesc::encode_store_heap_oop_not_null(p, new_obj);
			  }
			
			
			}
	/*-tony 2010-06-14 */


}

 //<-tony 2010-08-26





/*


  /////////////
if(_g->kind()!=Generation::TransNew)
{
  if ((HeapWord*)obj < _boundary && !((DefNewGeneration*)_g)->to()->is_in_reserved(obj)) {
    oop new_obj = obj->is_forwarded() ? obj->forwardee()
                                      : ((DefNewGeneration*)_g)->copy_to_survivor_space(obj);
    oopDesc::encode_store_heap_oop_not_null(p, new_obj);
  }
}
else {
	 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
//  if ((HeapWord*)obj < _boundary && !((TransNewGeneration*)_g)->to()->is_in_reserved(obj)) {
  if ((HeapWord*)obj < _boundary) {
 //<-tony 2010-06-28/

oop new_obj = obj->is_forwarded() ? obj->forwardee()
                                      : ((TransNewGeneration*)_g)->copy_to_survivor_space(obj);
    oopDesc::encode_store_heap_oop_not_null(p, new_obj);
  	}

	
}
*/
  /*-tony 2010-06-14 */


inline void ScanWeakRefClosure::do_oop_nv(oop* p)       { ScanWeakRefClosure::do_oop_work(p); }
inline void ScanWeakRefClosure::do_oop_nv(narrowOop* p) { ScanWeakRefClosure::do_oop_work(p); }
