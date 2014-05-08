/*
 * Copyright 2001-2009 Sun Microsystems, Inc.  All Rights Reserved.
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

 /*tony 07/19/2009 Learning about Threads, Tony*/ 

#include <sys/types.h> //maybe we cannot put includes in the inline files.
#include <sys/syscall.h> //tony for the syscall(SYS_gettid)


 /*-tony 07/19/2009*/
 

// Inline allocation implementations.

void CollectedHeap::post_allocation_setup_common(KlassHandle klass,
                                                 HeapWord* obj,
                                                 size_t size) {
  post_allocation_setup_no_klass_install(klass, obj, size);
  post_allocation_install_obj_klass(klass, oop(obj), (int) size);
}

void CollectedHeap::post_allocation_setup_no_klass_install(KlassHandle klass,
                                                           HeapWord* objPtr,
                                                           size_t size) {
  oop obj = (oop)objPtr;

  assert(obj != NULL, "NULL object pointer");
  if (UseBiasedLocking && (klass() != NULL)) {
    obj->set_mark(klass->prototype_header());
  } else {
    // May be bootstrapping
    obj->set_mark(markOopDesc::prototype());
  }
}

void CollectedHeap::post_allocation_install_obj_klass(KlassHandle klass,
                                                   oop obj,
                                                   int size) {
  // These asserts are kind of complicated because of klassKlass
  // and the beginning of the world.
  assert(klass() != NULL || !Universe::is_fully_initialized(), "NULL klass");
  assert(klass() == NULL || klass()->is_klass(), "not a klass");
  assert(klass() == NULL || klass()->klass_part() != NULL, "not a klass");
  assert(obj != NULL, "NULL object pointer");
  obj->set_klass(klass());
  assert(!Universe::is_fully_initialized() || obj->blueprint() != NULL,
         "missing blueprint");
}

// Support for jvmti and dtrace
inline void post_allocation_notify(KlassHandle klass, oop obj) {
  // support low memory notifications (no-op if not enabled)
  LowMemoryDetector::detect_low_memory_for_collected_pools();

  // support for JVMTI VMObjectAlloc event (no-op if not enabled)
   //tony-> 2010-12-01 printing out the allocation site of the residuals.
  if(!TransGC)
	  JvmtiExport::vm_object_alloc_event_collector(obj);
  else
  	{
  	//we only tag those objects allocated into transNewGen.
  	if(OnlyTagObjInTrans)
	 {	
	 			if(Universe::heap()->addr_to_gen_id((HeapWord*)obj)==1)
		 		{  
		 		JvmtiExport::vm_object_alloc_event_collector(obj);
			 	}
  		}
	 else
	 	{
	 			JvmtiExport::vm_object_alloc_event_collector(obj);
	 	}
		

  	}
   //<-tony 2010-12-01

   
  if (DTraceAllocProbes) {
    // support for Dtrace object alloc event (no-op most of the time)
    if (klass() != NULL && klass()->klass_part()->name() != NULL) {
      SharedRuntime::dtrace_object_alloc(obj);
    }
  }
}

void CollectedHeap::post_allocation_setup_obj(KlassHandle klass,
                                              HeapWord* obj,
                                              size_t size) {
  post_allocation_setup_common(klass, obj, size);
  assert(Universe::is_bootstrapping() ||
         !((oop)obj)->blueprint()->oop_is_array(), "must not be an array");
  // notify jvmti and dtrace
  post_allocation_notify(klass, (oop)obj);
  #if 0
 //tony-> 2011/02/09 make sure that we captuer all the allocations.
  {
  ResourceMark rm;
  HandleMark hm; //release handles created in the domain.
  Klass* ik = Klass::cast(klass()); 
  if(ik->oop_is_instance()&&strcmp(ik->external_name(),"client.Pi")==0)
  {
    tty->print_cr("clien.Pi inited here!");
  	//assert(false,"clien.Pi created here!");
  }
 }
  #endif
  
  //<-tony 2011-02-09
	

}

void CollectedHeap::post_allocation_setup_array(KlassHandle klass,
                                                HeapWord* obj,
                                                size_t size,
                                                int length) {
  // Set array length before setting the _klass field
  // in post_allocation_setup_common() because the klass field
  // indicates that the object is parsable by concurrent GC.
  assert(length >= 0, "length should be non-negative");
  ((arrayOop)obj)->set_length(length);
  post_allocation_setup_common(klass, obj, size);
  assert(((oop)obj)->blueprint()->oop_is_array(), "must be an array");
  // notify jvmti and dtrace (must be after length is set for dtrace)
  post_allocation_notify(klass, (oop)obj);
}

 /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
HeapWord* CollectedHeap::common_mem_allocate_noinit(KlassHandle klass, size_t size, bool is_noref, TRAPS){

//HeapWord* CollectedHeap::common_mem_allocate_noinit(size_t size, bool is_noref, TRAPS) {


 /*-tony 01/18/2010 */
 

  // Clear unhandled oops for memory allocation.  Memory allocation might
  // not take out a lock if from tlab, so clear here.
  CHECK_UNHANDLED_OOPS_ONLY(THREAD->clear_unhandled_oops();)

  
  if (HAS_PENDING_EXCEPTION) {
    NOT_PRODUCT(guarantee(false, "Should not allocate with exception pending"));
    return NULL;  // caller does a CHECK_0 too
  }

  // We may want to update this, is_noref objects might not be allocated in TLABs.
  HeapWord* result = NULL;

//by tony 2011/01/13 check the residuals.
  juint ac=Klass::cast(klass())->allocation_counts();
  Klass::cast(klass())->set_allocation_counts(ac+1);
#if 0
//temparorily added the assertion 2011-2-7
{
  ResourceMark rm;
  HandleMark hm; //release handles created in the domain.
  Klass* ik = Klass::cast(klass()); 
  if(ik->oop_is_instance()&&(strcmp(ik->external_name(),"client.Pi")==0))
  {
    tty->print_cr("clien.Pi created here! counts:%d",Klass::cast(klass())->allocation_counts());
  	//assert(false,"clien.Pi created here!");

  }
}
#endif

 //tony-> 2010-10-04 instrumenting park(),get rid of the need to monitor method calls.

 #if 1
 if(TransGC&&UseParkUnpark)
 	{//by tony 2010-12-16 Recovering Park/Unpark detection.
 		 Thread* t = THREAD;  
	  	 if(t&&t->is_Java_thread())
			 	{	 	
			  JavaThread *jt = (JavaThread *)t;
			  //monitored- it's the direct son of the transaction parent
			  //isPooled- it's previously pooled, so this is the first allocation request by it after re-activated
			  //!hasValidTransID - we should allocate one for it.
		      if(jt->isMonitored()&&jt->isPooled()&&!jt->hasValidTransID())
				{
				jt->setTransID(Universe::getNewTransID());
				jt->resetPooled();
				jt->reset_thread_counts_in_transaction();
				jt->setTransEdenID(-1);
				if(DebugTypeGenGC){
					gclog_or_tty->print_cr("[TRANS-START-UNPARK]A new transaction(%d) identified by the pooled thread %d.",jt->transID(),t->osthread()->thread_id());
							  tty->print_cr("[TRANS-START-UNPARK]A new transaction(%d) identified by the pooled thread %d.",jt->transID(),t->osthread()->thread_id());
					}
		      	}
	  	 	}
 	}
#endif		 
 //<-tony 2010-10-04
//here we need to check klass to see if we should pretenure it!		
 //tony-> 2010-11-04 analysing the residuals

  bool pretenure=false;

  if(TransGC&&APPLY_WHILE_TRAINING) //use the training result as soon as possible
		 	pretenure=Klass::cast(klass())->is_residual();
  else
  	{//wait for the training is done.
		  	if(Universe::isTrainingDone())
			 	pretenure=Klass::cast(klass())->is_residual();	//FIXME:if this transaction is not trained,we don't do 
			 	//pretenure for it, it will be collected as regular after all.
  	}

	if(!Pretenure)
		pretenure=false; //by tony 2011-1-4, a switch to turn off the pretenure.

  //if (UseTLAB) {
  	if (UseTLAB&&!pretenure) {
   //<-tony 2010-11-04	

  #if 0
	  ResourceMark rm;
	  HandleMark hm; //release handles created in the domain.
	  Klass* ik = Klass::cast(klass()); 
	  if(ik->oop_is_instance()&&strcmp(ik->external_name(),"client.Pi")==0)
	  {
	    tty->print_cr("clien.Pi created to TLAB()! counts:%d",Klass::cast(klass())->allocation_counts());
	  	//assert(false,"clien.Pi created here!");

	  }
  #endif
   
    result = CollectedHeap::allocate_from_tlab(THREAD, size);
    if (result != NULL) {
      assert(!HAS_PENDING_EXCEPTION,
             "Unexpected exception, will result in uninitialized storage");
      return result;
    }
  }
	
  bool gc_overhead_limit_was_exceeded = false;

/*tony 01/18/2010 Porting Type Based GC, Tony*/ 
if(UseTypeGenGC) {

	/*
        if(klass() && Universe::is_fully_initialized()){
				if (klass()->is_klass())  //Added by Feng
		 	          {  remote = klass()->klass_part()->is_remote(); }    
			        if (t->is_under_remote_call()) 
		        	  {   
					  remote = true;
					   }
	    }
  bool remote = false;
  
  JavaThread* t = (JavaThread *)get_thread();
  int bytesize = size * wordSize;  
  
  if(t){
	if(t->get_pthread_id()<0)
		{
	Thread* tonythread=(Thread*)t;
	int ostid=tonythread->osthread()->thread_id();
    t->set_pthread_id(ostid);  
 	tty->print_cr("an unseen thread is requesting allocation,tid=%d",ostid);
	}
  	}

	 if(!t->hasValidTransID())
		 	remote=false; //overriding the remote, restricting the remote allocation to transaction thread only.//by tony 2010-07-26
		 else
		 	remote=true; ////by tony 2010-09-16 if there is a validtransID associcated with this thread, allocate into transEden.

//	     if(remote)gclog_or_tty->print("transID(%d) Remote Alloc!",t->transID());
	    */
		 
         result = Universe::heap()->mem_allocate_with_pretenuring(size, false, pretenure); //here we need to check the klass type after the training.
/*
			if(remote){
               Universe::add_remote_obj_count(1);
               Universe::add_remote_obj_bytes(bytesize);
            }
            else{
               Universe::add_local_obj_count(1);
               Universe::add_local_obj_bytes(bytesize);
            }
            */
   }else
   {
  // gclog_or_tty->print_cr("CollectedHeap::common_mem_allocate_noinit calling Universe::heap()->mem_allocate");
  result = Universe::heap()->mem_allocate(size,
                                          is_noref,
                                          false,
                                          &gc_overhead_limit_was_exceeded);
   	}
   

/*-tony 01/18/2010 */


  if (result != NULL) {
    NOT_PRODUCT(Universe::heap()->
      check_for_non_bad_heap_word_value(result, size));
    assert(!HAS_PENDING_EXCEPTION,
           "Unexpected exception, will result in uninitialized storage");
    return result;
  }
   

  if (!gc_overhead_limit_was_exceeded) {
    // -XX:+HeapDumpOnOutOfMemoryError and -XX:OnOutOfMemoryError support
    report_java_out_of_memory("Java heap space");

    if (JvmtiExport::should_post_resource_exhausted()) {
      JvmtiExport::post_resource_exhausted(
        JVMTI_RESOURCE_EXHAUSTED_OOM_ERROR | JVMTI_RESOURCE_EXHAUSTED_JAVA_HEAP,
        "Java heap space");
    }

    THROW_OOP_0(Universe::out_of_memory_error_java_heap());
  } else {
    // -XX:+HeapDumpOnOutOfMemoryError and -XX:OnOutOfMemoryError support
    report_java_out_of_memory("GC overhead limit exceeded");

    if (JvmtiExport::should_post_resource_exhausted()) {
      JvmtiExport::post_resource_exhausted(
        JVMTI_RESOURCE_EXHAUSTED_OOM_ERROR | JVMTI_RESOURCE_EXHAUSTED_JAVA_HEAP,
        "GC overhead limit exceeded");
    }

    THROW_OOP_0(Universe::out_of_memory_error_gc_overhead_limit());
  }
}


  // Like allocate_init, but the block returned by a successful allocation
  // is guaranteed initialized to zeros.


  /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
  //adding klass to the parameter
//HeapWord* CollectedHeap::common_mem_allocate_init(size_t size, bool is_noref, TRAPS) {
HeapWord* CollectedHeap::common_mem_allocate_init(KlassHandle klass, size_t size, bool is_noref, TRAPS){

//  HeapWord* obj = common_mem_allocate_noinit(size, is_noref, CHECK_NULL);
  HeapWord* obj = common_mem_allocate_noinit(klass,size, is_noref, CHECK_NULL);

  /*-tony 01/18/2010 */
  init_obj(obj, size);
  return obj;
}

// Need to investigate, do we really want to throw OOM exception here?
HeapWord* CollectedHeap::common_permanent_mem_allocate_noinit(size_t size, TRAPS) {
  if (HAS_PENDING_EXCEPTION) {
    NOT_PRODUCT(guarantee(false, "Should not allocate with exception pending"));
    return NULL;  // caller does a CHECK_NULL too
  }

#ifdef ASSERT
  if (CIFireOOMAt > 0 && THREAD->is_Compiler_thread() &&
      ++_fire_out_of_memory_count >= CIFireOOMAt) {
    // For testing of OOM handling in the CI throw an OOM and see how
    // it does.  Historically improper handling of these has resulted
    // in crashes which we really don't want to have in the CI.
    THROW_OOP_0(Universe::out_of_memory_error_perm_gen());
  }
#endif

  HeapWord* result = Universe::heap()->permanent_mem_allocate(size);
  if (result != NULL) {
    NOT_PRODUCT(Universe::heap()->
      check_for_non_bad_heap_word_value(result, size));
    assert(!HAS_PENDING_EXCEPTION,
           "Unexpected exception, will result in uninitialized storage");
    return result;
  }
  // -XX:+HeapDumpOnOutOfMemoryError and -XX:OnOutOfMemoryError support
  report_java_out_of_memory("PermGen space");

  if (JvmtiExport::should_post_resource_exhausted()) {
    JvmtiExport::post_resource_exhausted(
        JVMTI_RESOURCE_EXHAUSTED_OOM_ERROR,
        "PermGen space");
  }

  THROW_OOP_0(Universe::out_of_memory_error_perm_gen());
}

HeapWord* CollectedHeap::common_permanent_mem_allocate_init(size_t size, TRAPS) {
  HeapWord* obj = common_permanent_mem_allocate_noinit(size, CHECK_NULL);
  init_obj(obj, size);
  return obj;
}

HeapWord* CollectedHeap::allocate_from_tlab(Thread* thread, size_t size) {
  assert(UseTLAB, "should use UseTLAB");

  HeapWord* obj = thread->tlab().allocate(size);
  if (obj != NULL) {
    return obj;
  }
  // Otherwise...
  return allocate_from_tlab_slow(thread, size);
}

void CollectedHeap::init_obj(HeapWord* obj, size_t size) {
  assert(obj != NULL, "cannot initialize NULL object");
  const size_t hs = oopDesc::header_size();
  assert(size >= hs, "unexpected object size");
  ((oop)obj)->set_klass_gap(0);
  Copy::fill_to_aligned_words(obj + hs, size - hs);
}

oop CollectedHeap::obj_allocate(KlassHandle klass, int size, TRAPS) {
  debug_only(check_for_valid_allocation_state());
  assert(!Universe::heap()->is_gc_active(), "Allocation during gc not allowed");
  assert(size >= 0, "int won't convert to size_t");


   /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
//  HeapWord* obj = common_mem_allocate_init(size, false, CHECK_NULL);

  HeapWord* obj = common_mem_allocate_init(klass,size, false, CHECK_NULL);
	//pass the klass to allocate, tony

   /*-tony 01/18/2010 */
   
  post_allocation_setup_obj(klass, obj, size);
  NOT_PRODUCT(Universe::heap()->check_for_bad_heap_word_value(obj, size));
  return (oop)obj;
}

oop CollectedHeap::array_allocate(KlassHandle klass,
                                  int size,
                                  int length,
                                  TRAPS) {
  debug_only(check_for_valid_allocation_state());
  assert(!Universe::heap()->is_gc_active(), "Allocation during gc not allowed");
  assert(size >= 0, "int won't convert to size_t");
//  HeapWord* obj = common_mem_allocate_init(size, false, CHECK_NULL);

/*tony 01/18/2010 Porting Type Based GC, Tony*/ 
	//pass the klass to allocate
	HeapWord* obj = common_mem_allocate_init(klass,size, false, CHECK_NULL);
/*-tony 01/18/2010 */

  post_allocation_setup_array(klass, obj, size, length);
  NOT_PRODUCT(Universe::heap()->check_for_bad_heap_word_value(obj, size));
  return (oop)obj;
}

oop CollectedHeap::large_typearray_allocate(KlassHandle klass,
                                            int size,
                                            int length,
                                            TRAPS) {
  debug_only(check_for_valid_allocation_state());
  assert(!Universe::heap()->is_gc_active(), "Allocation during gc not allowed");
  assert(size >= 0, "int won't convert to size_t");

//  HeapWord* obj = common_mem_allocate_init(size, true, CHECK_NULL);
  
/*tony 01/18/2010 Porting Type Based GC, Tony*/ 
	//pass the klass to allocate
	HeapWord* obj = common_mem_allocate_init(klass,size, true, CHECK_NULL);
/*-tony 01/18/2010 */

  post_allocation_setup_array(klass, obj, size, length);
  NOT_PRODUCT(Universe::heap()->check_for_bad_heap_word_value(obj, size));
  return (oop)obj;
}

oop CollectedHeap::permanent_obj_allocate(KlassHandle klass, int size, TRAPS) {
  oop obj = permanent_obj_allocate_no_klass_install(klass, size, CHECK_NULL);
  post_allocation_install_obj_klass(klass, obj, size);
  NOT_PRODUCT(Universe::heap()->check_for_bad_heap_word_value((HeapWord*) obj,
                                                              size));
  return obj;
}

oop CollectedHeap::permanent_obj_allocate_no_klass_install(KlassHandle klass,
                                                           int size,
                                                           TRAPS) {
  debug_only(check_for_valid_allocation_state());
  assert(!Universe::heap()->is_gc_active(), "Allocation during gc not allowed");
  assert(size >= 0, "int won't convert to size_t");
  HeapWord* obj = common_permanent_mem_allocate_init(size, CHECK_NULL);
  post_allocation_setup_no_klass_install(klass, obj, size);
  NOT_PRODUCT(Universe::heap()->check_for_bad_heap_word_value(obj, size));
  return (oop)obj;
}

oop CollectedHeap::permanent_array_allocate(KlassHandle klass,
                                            int size,
                                            int length,
                                            TRAPS) {
  debug_only(check_for_valid_allocation_state());
  assert(!Universe::heap()->is_gc_active(), "Allocation during gc not allowed");
  assert(size >= 0, "int won't convert to size_t");
  HeapWord* obj = common_permanent_mem_allocate_init(size, CHECK_NULL);
  post_allocation_setup_array(klass, obj, size, length);
  NOT_PRODUCT(Universe::heap()->check_for_bad_heap_word_value(obj, size));
  return (oop)obj;
}

// Returns "TRUE" if "p" is a method oop in the
// current heap with high probability. NOTE: The main
// current consumers of this interface are Forte::
// and ThreadProfiler::. In these cases, the
// interpreter frame from which "p" came, may be
// under construction when sampled asynchronously, so
// the clients want to check that it represents a
// valid method before using it. Nonetheless since
// the clients do not typically lock out GC, the
// predicate is_valid_method() is not stable, so
// it is possible that by the time "p" is used, it
// is no longer valid.
inline bool CollectedHeap::is_valid_method(oop p) const {
  return
    p != NULL &&

    // Check whether it is aligned at a HeapWord boundary.
    Space::is_aligned(p) &&

    // Check whether "method" is in the allocated part of the
    // permanent generation -- this needs to be checked before
    // p->klass() below to avoid a SEGV (but see below
    // for a potential window of vulnerability).
    is_permanent((void*)p) &&

    // See if GC is active; however, there is still an
    // apparently unavoidable window after this call
    // and before the client of this interface uses "p".
    // If the client chooses not to lock out GC, then
    // it's a risk the client must accept.
    !is_gc_active() &&

    // Check that p is a methodOop.
    p->klass() == Universe::methodKlassObj();
}


#ifndef PRODUCT

inline bool
CollectedHeap::promotion_should_fail(volatile size_t* count) {
  // Access to count is not atomic; the value does not have to be exact.
  if (PromotionFailureALot) {
    const size_t gc_num = total_collections();
    const size_t elapsed_gcs = gc_num - _promotion_failure_alot_gc_number;
    if (elapsed_gcs >= PromotionFailureALotInterval) {
      // Test for unsigned arithmetic wrap-around.
      if (++*count >= PromotionFailureALotCount) {
        *count = 0;
        return true;
      }
    }
  }
  return false;
}

inline bool CollectedHeap::promotion_should_fail() {
  return promotion_should_fail(&_promotion_failure_alot_count);
}

inline void CollectedHeap::reset_promotion_should_fail(volatile size_t* count) {
  if (PromotionFailureALot) {
    _promotion_failure_alot_gc_number = total_collections();
    *count = 0;
  }
}

inline void CollectedHeap::reset_promotion_should_fail() {
  reset_promotion_should_fail(&_promotion_failure_alot_count);
}
#endif  // #ifndef PRODUCT
