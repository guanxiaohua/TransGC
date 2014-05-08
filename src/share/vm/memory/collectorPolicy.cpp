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
# include "incls/_collectorPolicy.cpp.incl"

// CollectorPolicy methods.
class TransGenerationSpec;

void CollectorPolicy::initialize_flags() {
  if (PermSize > MaxPermSize) {
    MaxPermSize = PermSize;
  }
  PermSize = MAX2(min_alignment(), align_size_down_(PermSize, min_alignment()));
  MaxPermSize = align_size_up(MaxPermSize, max_alignment());

  MinPermHeapExpansion = MAX2(min_alignment(), align_size_down_(MinPermHeapExpansion, min_alignment()));
  MaxPermHeapExpansion = MAX2(min_alignment(), align_size_down_(MaxPermHeapExpansion, min_alignment()));

  MinHeapDeltaBytes = align_size_up(MinHeapDeltaBytes, min_alignment());

  SharedReadOnlySize = align_size_up(SharedReadOnlySize, max_alignment());
  SharedReadWriteSize = align_size_up(SharedReadWriteSize, max_alignment());
  SharedMiscDataSize = align_size_up(SharedMiscDataSize, max_alignment());

  assert(PermSize    % min_alignment() == 0, "permanent space alignment");
  assert(MaxPermSize % max_alignment() == 0, "maximum permanent space alignment");
  assert(SharedReadOnlySize % max_alignment() == 0, "read-only space alignment");
  assert(SharedReadWriteSize % max_alignment() == 0, "read-write space alignment");
  assert(SharedMiscDataSize % max_alignment() == 0, "misc-data space alignment");
  if (PermSize < M) {
    vm_exit_during_initialization("Too small initial permanent heap");
  }
}

void CollectorPolicy::initialize_size_info() {
  // User inputs from -mx and ms are aligned
  set_initial_heap_byte_size(Arguments::initial_heap_size());
  if (initial_heap_byte_size() == 0) {
    set_initial_heap_byte_size(NewSize + OldSize);
  }
  set_initial_heap_byte_size(align_size_up(_initial_heap_byte_size,
                                           min_alignment()));

  set_min_heap_byte_size(Arguments::min_heap_size());
  if (min_heap_byte_size() == 0) {
    set_min_heap_byte_size(NewSize + OldSize);
  }
  set_min_heap_byte_size(align_size_up(_min_heap_byte_size,
                                       min_alignment()));

  set_max_heap_byte_size(align_size_up(MaxHeapSize, max_alignment()));

  // Check heap parameter properties
  if (initial_heap_byte_size() < M) {
    vm_exit_during_initialization("Too small initial heap");
  }
  // Check heap parameter properties
  if (min_heap_byte_size() < M) {
    vm_exit_during_initialization("Too small minimum heap");
  }
  if (initial_heap_byte_size() <= NewSize) {
     // make sure there is at least some room in old space
    vm_exit_during_initialization("Too small initial heap for new size specified");
  }
  if (max_heap_byte_size() < min_heap_byte_size()) {
    vm_exit_during_initialization("Incompatible minimum and maximum heap sizes specified");
  }
  if (initial_heap_byte_size() < min_heap_byte_size()) {
    vm_exit_during_initialization("Incompatible minimum and initial heap sizes specified");
  }
  if (max_heap_byte_size() < initial_heap_byte_size()) {
    vm_exit_during_initialization("Incompatible initial and maximum heap sizes specified");
  }

  if (PrintGCDetails && Verbose) {
    gclog_or_tty->print_cr("Minimum heap " SIZE_FORMAT "  Initial heap "
      SIZE_FORMAT "  Maximum heap " SIZE_FORMAT,
      min_heap_byte_size(), initial_heap_byte_size(), max_heap_byte_size());
  }
}

void CollectorPolicy::initialize_perm_generation(PermGen::Name pgnm) {
  _permanent_generation =
    new PermanentGenerationSpec(pgnm, PermSize, MaxPermSize,
                                SharedReadOnlySize,
                                SharedReadWriteSize,
                                SharedMiscDataSize,
                                SharedMiscCodeSize);
  if (_permanent_generation == NULL) {
    vm_exit_during_initialization("Unable to allocate gen spec");
  }
}


GenRemSet* CollectorPolicy::create_rem_set(MemRegion whole_heap,
                                           int max_covered_regions) {
  switch (rem_set_name()) {
  case GenRemSet::CardTable: {

 //tony-> 2010-08-26 Fixing transEdens collector
    if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]CollectorPolicy::create_rem_set:create_rem_set over whole_heap(%p,%p)", whole_heap.start(),whole_heap.end());}
 //<-tony 2010-08-26	

    CardTableRS* res = new CardTableRS(whole_heap, max_covered_regions);
    return res;
  }
  default:
    guarantee(false, "unrecognized GenRemSet::Name");
    return NULL;
  }
}

// GenCollectorPolicy methods.

size_t GenCollectorPolicy::scale_by_NewRatio_aligned(size_t base_size) {
  size_t x = base_size / (NewRatio+1);
  size_t new_gen_size = x > min_alignment() ?
                     align_size_down(x, min_alignment()) :
                     min_alignment();
  return new_gen_size;
}

size_t GenCollectorPolicy::bound_minus_alignment(size_t desired_size,
                                                 size_t maximum_size) {
  size_t alignment = min_alignment();
  size_t max_minus = maximum_size - alignment;
  return desired_size < max_minus ? desired_size : max_minus;
}


void GenCollectorPolicy::initialize_size_policy(size_t init_eden_size,
                                                size_t init_promo_size,
                                                size_t init_survivor_size) {
  const double max_gc_minor_pause_sec = ((double) MaxGCMinorPauseMillis)/1000.0;
  _size_policy = new AdaptiveSizePolicy(init_eden_size,
                                        init_promo_size,
                                        init_survivor_size,
                                        max_gc_minor_pause_sec,
                                        GCTimeRatio);
}

size_t GenCollectorPolicy::compute_max_alignment() {
  // The card marking array and the offset arrays for old generations are
  // committed in os pages as well. Make sure they are entirely full (to
  // avoid partial page problems), e.g. if 512 bytes heap corresponds to 1
  // byte entry and the os page size is 4096, the maximum heap size should
  // be 512*4096 = 2MB aligned.
  size_t alignment = GenRemSet::max_alignment_constraint(rem_set_name());

  // Parallel GC does its own alignment of the generations to avoid requiring a
  // large page (256M on some platforms) for the permanent generation.  The
  // other collectors should also be updated to do their own alignment and then
  // this use of lcm() should be removed.
  if (UseLargePages && !UseParallelGC) {
      // in presence of large pages we have to make sure that our
      // alignment is large page aware
      alignment = lcm(os::large_page_size(), alignment);
  }

  return alignment;
}

void GenCollectorPolicy::initialize_flags() {
  // All sizes must be multiples of the generation granularity.
  set_min_alignment((uintx) Generation::GenGrain);
  set_max_alignment(compute_max_alignment());
  assert(max_alignment() >= min_alignment() &&
         max_alignment() % min_alignment() == 0,
         "invalid alignment constraints");

  CollectorPolicy::initialize_flags();

  // All generational heaps have a youngest gen; handle those flags here.

  // Adjust max size parameters
  if (NewSize > MaxNewSize) {
    MaxNewSize = NewSize;
  }
  NewSize = align_size_down(NewSize, min_alignment());
  MaxNewSize = align_size_down(MaxNewSize, min_alignment());

  // Check validity of heap flags
  assert(NewSize     % min_alignment() == 0, "eden space alignment");
  assert(MaxNewSize  % min_alignment() == 0, "survivor space alignment");

  if (NewSize < 3*min_alignment()) {
     // make sure there room for eden and two survivor spaces
    vm_exit_during_initialization("Too small new size specified");
  }
  if (SurvivorRatio < 1 || NewRatio < 1) {
    vm_exit_during_initialization("Invalid heap ratio specified");
  }
}

void TwoGenerationCollectorPolicy::initialize_flags() {
  GenCollectorPolicy::initialize_flags();

  OldSize = align_size_down(OldSize, min_alignment());
  if (NewSize + OldSize > MaxHeapSize) {
    MaxHeapSize = NewSize + OldSize;
  }
  MaxHeapSize = align_size_up(MaxHeapSize, max_alignment());

  always_do_update_barrier = UseConcMarkSweepGC;
  BlockOffsetArrayUseUnallocatedBlock =
      BlockOffsetArrayUseUnallocatedBlock || ParallelGCThreads > 0;

  // Check validity of heap flags
  assert(OldSize     % min_alignment() == 0, "old space alignment");
  assert(MaxHeapSize % max_alignment() == 0, "maximum heap alignment");
}

// Values set on the command line win over any ergonomically
// set command line parameters.
// Ergonomic choice of parameters are done before this
// method is called.  Values for command line parameters such as NewSize
// and MaxNewSize feed those ergonomic choices into this method.
// This method makes the final generation sizings consistent with
// themselves and with overall heap sizings.
// In the absence of explicitly set command line flags, policies
// such as the use of NewRatio are used to size the generation.
void GenCollectorPolicy::initialize_size_info() {
  CollectorPolicy::initialize_size_info();

  // min_alignment() is used for alignment within a generation.
  // There is additional alignment done down stream for some
  // collectors that sometimes causes unwanted rounding up of
  // generations sizes.

  // Determine maximum size of gen0

  size_t max_new_size = 0;
  if (FLAG_IS_CMDLINE(MaxNewSize)) {
    if (MaxNewSize < min_alignment()) {
      max_new_size = min_alignment();
    } else if (MaxNewSize >= max_heap_byte_size()) {
      max_new_size = align_size_down(max_heap_byte_size() - min_alignment(),
                                     min_alignment());
      warning("MaxNewSize (" SIZE_FORMAT "k) is equal to or "
        "greater than the entire heap (" SIZE_FORMAT "k).  A "
        "new generation size of " SIZE_FORMAT "k will be used.",
        MaxNewSize/K, max_heap_byte_size()/K, max_new_size/K);
    } else {
      max_new_size = align_size_down(MaxNewSize, min_alignment());
    }

  // The case for FLAG_IS_ERGO(MaxNewSize) could be treated
  // specially at this point to just use an ergonomically set
  // MaxNewSize to set max_new_size.  For cases with small
  // heaps such a policy often did not work because the MaxNewSize
  // was larger than the entire heap.  The interpretation given
  // to ergonomically set flags is that the flags are set
  // by different collectors for their own special needs but
  // are not allowed to badly shape the heap.  This allows the
  // different collectors to decide what's best for themselves
  // without having to factor in the overall heap shape.  It
  // can be the case in the future that the collectors would
  // only make "wise" ergonomics choices and this policy could
  // just accept those choices.  The choices currently made are
  // not always "wise".
  } else {
    max_new_size = scale_by_NewRatio_aligned(max_heap_byte_size());
    // Bound the maximum size by NewSize below (since it historically
    // would have been NewSize and because the NewRatio calculation could
    // yield a size that is too small) and bound it by MaxNewSize above.
    // Ergonomics plays here by previously calculating the desired
    // NewSize and MaxNewSize.
    max_new_size = MIN2(MAX2(max_new_size, NewSize), MaxNewSize);
  }
  assert(max_new_size > 0, "All paths should set max_new_size");

  // Given the maximum gen0 size, determine the initial and
  // minimum sizes.

  if (max_heap_byte_size() == min_heap_byte_size()) {
    // The maximum and minimum heap sizes are the same so
    // the generations minimum and initial must be the
    // same as its maximum.
    set_min_gen0_size(max_new_size);
    set_initial_gen0_size(max_new_size);
    set_max_gen0_size(max_new_size);
  } else {
    size_t desired_new_size = 0;
    if (!FLAG_IS_DEFAULT(NewSize)) {
      // If NewSize is set ergonomically (for example by cms), it
      // would make sense to use it.  If it is used, also use it
      // to set the initial size.  Although there is no reason
      // the minimum size and the initial size have to be the same,
      // the current implementation gets into trouble during the calculation
      // of the tenured generation sizes if they are different.
      // Note that this makes the initial size and the minimum size
      // generally small compared to the NewRatio calculation.
      _min_gen0_size = NewSize;
      desired_new_size = NewSize;
      max_new_size = MAX2(max_new_size, NewSize);
    } else {
      // For the case where NewSize is the default, use NewRatio
      // to size the minimum and initial generation sizes.
      // Use the default NewSize as the floor for these values.  If
      // NewRatio is overly large, the resulting sizes can be too
      // small.
      _min_gen0_size = MAX2(scale_by_NewRatio_aligned(min_heap_byte_size()),
                          NewSize);
      desired_new_size =
        MAX2(scale_by_NewRatio_aligned(initial_heap_byte_size()),
             NewSize);
    }

    assert(_min_gen0_size > 0, "Sanity check");
    set_initial_gen0_size(desired_new_size);
    set_max_gen0_size(max_new_size);

    // At this point the desirable initial and minimum sizes have been
    // determined without regard to the maximum sizes.

    // Bound the sizes by the corresponding overall heap sizes.
    set_min_gen0_size(
      bound_minus_alignment(_min_gen0_size, min_heap_byte_size()));
    set_initial_gen0_size(
      bound_minus_alignment(_initial_gen0_size, initial_heap_byte_size()));
    set_max_gen0_size(
      bound_minus_alignment(_max_gen0_size, max_heap_byte_size()));

    // At this point all three sizes have been checked against the
    // maximum sizes but have not been checked for consistency
    // among the three.

    // Final check min <= initial <= max
    set_min_gen0_size(MIN2(_min_gen0_size, _max_gen0_size));
    set_initial_gen0_size(
      MAX2(MIN2(_initial_gen0_size, _max_gen0_size), _min_gen0_size));
    set_min_gen0_size(MIN2(_min_gen0_size, _initial_gen0_size));
  }

  if (PrintGCDetails && Verbose) {
    gclog_or_tty->print_cr("Minimum gen0 " SIZE_FORMAT "  Initial gen0 "
      SIZE_FORMAT "  Maximum gen0 " SIZE_FORMAT,
      min_gen0_size(), initial_gen0_size(), max_gen0_size());
  }
}

// Call this method during the sizing of the gen1 to make
// adjustments to gen0 because of gen1 sizing policy.  gen0 initially has
// the most freedom in sizing because it is done before the
// policy for gen1 is applied.  Once gen1 policies have been applied,
// there may be conflicts in the shape of the heap and this method
// is used to make the needed adjustments.  The application of the
// policies could be more sophisticated (iterative for example) but
// keeping it simple also seems a worthwhile goal.
bool TwoGenerationCollectorPolicy::adjust_gen0_sizes(size_t* gen0_size_ptr,
                                                     size_t* gen1_size_ptr,
                                                     size_t heap_size,
                                                     size_t min_gen0_size) {
  bool result = false;
  if ((*gen1_size_ptr + *gen0_size_ptr) > heap_size) {
    if (((*gen0_size_ptr + OldSize) > heap_size) &&
       (heap_size - min_gen0_size) >= min_alignment()) {
      // Adjust gen0 down to accomodate OldSize
      *gen0_size_ptr = heap_size - min_gen0_size;
      *gen0_size_ptr =
        MAX2((uintx)align_size_down(*gen0_size_ptr, min_alignment()),
             min_alignment());
      assert(*gen0_size_ptr > 0, "Min gen0 is too large");
      result = true;
    } else {
      *gen1_size_ptr = heap_size - *gen0_size_ptr;
      *gen1_size_ptr =
        MAX2((uintx)align_size_down(*gen1_size_ptr, min_alignment()),
                       min_alignment());
    }
  }
  return result;
}

// Minimum sizes of the generations may be different than
// the initial sizes.  An inconsistently is permitted here
// in the total size that can be specified explicitly by
// command line specification of OldSize and NewSize and
// also a command line specification of -Xms.  Issue a warning
// but allow the values to pass.

void TwoGenerationCollectorPolicy::initialize_size_info() {
  GenCollectorPolicy::initialize_size_info();

  // At this point the minimum, initial and maximum sizes
  // of the overall heap and of gen0 have been determined.
  // The maximum gen1 size can be determined from the maximum gen0
  // and maximum heap size since not explicit flags exits
  // for setting the gen1 maximum.
  _max_gen1_size = max_heap_byte_size() - _max_gen0_size;
  _max_gen1_size =
    MAX2((uintx)align_size_down(_max_gen1_size, min_alignment()),
         min_alignment());
  // If no explicit command line flag has been set for the
  // gen1 size, use what is left for gen1.
  if (FLAG_IS_DEFAULT(OldSize) || FLAG_IS_ERGO(OldSize)) {
    // The user has not specified any value or ergonomics
    // has chosen a value (which may or may not be consistent
    // with the overall heap size).  In either case make
    // the minimum, maximum and initial sizes consistent
    // with the gen0 sizes and the overall heap sizes.
    assert(min_heap_byte_size() > _min_gen0_size,
      "gen0 has an unexpected minimum size");
    set_min_gen1_size(min_heap_byte_size() - min_gen0_size());
    set_min_gen1_size(
      MAX2((uintx)align_size_down(_min_gen1_size, min_alignment()),
           min_alignment()));
    set_initial_gen1_size(initial_heap_byte_size() - initial_gen0_size());
    set_initial_gen1_size(
      MAX2((uintx)align_size_down(_initial_gen1_size, min_alignment()),
           min_alignment()));

  } else {
    // It's been explicitly set on the command line.  Use the
    // OldSize and then determine the consequences.
    set_min_gen1_size(OldSize);
    set_initial_gen1_size(OldSize);

    // If the user has explicitly set an OldSize that is inconsistent
    // with other command line flags, issue a warning.
    // The generation minimums and the overall heap mimimum should
    // be within one heap alignment.
    if ((_min_gen1_size + _min_gen0_size + min_alignment()) <
           min_heap_byte_size()) {
      warning("Inconsistency between minimum heap size and minimum "
          "generation sizes: using minimum heap = " SIZE_FORMAT,
          min_heap_byte_size());
    }
    if ((OldSize > _max_gen1_size)) {
      warning("Inconsistency between maximum heap size and maximum "
          "generation sizes: using maximum heap = " SIZE_FORMAT
          " -XX:OldSize flag is being ignored",
          max_heap_byte_size());
  }
    // If there is an inconsistency between the OldSize and the minimum and/or
    // initial size of gen0, since OldSize was explicitly set, OldSize wins.
    if (adjust_gen0_sizes(&_min_gen0_size, &_min_gen1_size,
                          min_heap_byte_size(), OldSize)) {
      if (PrintGCDetails && Verbose) {
        gclog_or_tty->print_cr("Minimum gen0 " SIZE_FORMAT "  Initial gen0 "
              SIZE_FORMAT "  Maximum gen0 " SIZE_FORMAT,
              min_gen0_size(), initial_gen0_size(), max_gen0_size());
      }
    }
    // Initial size
    if (adjust_gen0_sizes(&_initial_gen0_size, &_initial_gen1_size,
                         initial_heap_byte_size(), OldSize)) {
      if (PrintGCDetails && Verbose) {
        gclog_or_tty->print_cr("Minimum gen0 " SIZE_FORMAT "  Initial gen0 "
          SIZE_FORMAT "  Maximum gen0 " SIZE_FORMAT,
          min_gen0_size(), initial_gen0_size(), max_gen0_size());
      }
    }
  }
  // Enforce the maximum gen1 size.
  set_min_gen1_size(MIN2(_min_gen1_size, _max_gen1_size));

  // Check that min gen1 <= initial gen1 <= max gen1
  set_initial_gen1_size(MAX2(_initial_gen1_size, _min_gen1_size));
  set_initial_gen1_size(MIN2(_initial_gen1_size, _max_gen1_size));

  if (PrintGCDetails && Verbose) {
    gclog_or_tty->print_cr("Minimum gen1 " SIZE_FORMAT "  Initial gen1 "
      SIZE_FORMAT "  Maximum gen1 " SIZE_FORMAT,
      min_gen1_size(), initial_gen1_size(), max_gen1_size());
  }
}

 /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
HeapWord* TwoGenerationCollectorPolicy::mem_allocate_work(size_t size,
                                                      //    bool is_large_noref,
                                                          bool is_tlab, 
 														bool* gc_overhead_limit_was_exceeded
 														//by tony 2010-2-10
                                                          ) {

  GenCollectedHeap *gch = GenCollectedHeap::heap();

  debug_only(gch->check_for_valid_allocation_state());
  assert(gch->no_gc_in_progress(), "Allocation during gc not allowed");
  HeapWord* result = NULL;

  // Loop until the allocation is satisified,
  // or unsatisfied after GC.
  for (int try_count = 1; /* return or throw */; try_count += 1) {
   int gc_count_before;  // read inside the Heap_lock locked region
   bool trigger_gc = false;

   if(!NormalGCInvoke && ( (1 + Universe::total_obj_count())%GCFreq ==0 ) ){
	trigger_gc = true;	//----------Added by Feng
                   //We trigger GC every GCFreq allocations; do nothing and invoke GC
   }

   if(!trigger_gc){//Added by Feng
    Generation *gen0 = gch->get_gen(0);
    // First allocation attempt is lock-free.
    //Generation *gen0 = gch->get_gen(0);
    assert(gen0->supports_inline_contig_alloc(),
      "Otherwise, must do alloc within heap lock");
    if (gen0->should_allocate(size, is_tlab)) {
      result = gen0->par_allocate(size, is_tlab);
      if (result != NULL) {
        assert(gch->is_in(result), "result not in heap");
        return result;
      }
    }
    // int gc_count_before;  // read inside the Heap_lock locked region
    {
      MutexLocker ml(Heap_lock);
    
      /* --[ Neo 07/14/2006 ]-----------------------------------------------------
      if (PrintGC && Verbose) {
        gclog_or_tty->print_cr("TwoGenerationCollectorPolicy::mem_allocate_work:"
                      " attempting locked slow path allocation");
      }
      ------------------------------------------------------------------------- */

      // Note that only large objects get a shot at being
      // allocated in later generations.  If jvmpi slow allocation
      // is enabled, allocate in later generations (since the
      // first generation is always full.
      bool first_only = ! should_try_older_generation_allocation(size);

      result = gch->attempt_allocation(size, 
                                     //  is_large_noref, 
                                       is_tlab, 
                                       first_only);
      if (result != NULL) {
        assert(gch->is_in(result), "result not in heap");
        return result;
      }
    }
      // Read the gc count while the heap lock is held.
      gc_count_before = Universe::heap()->total_collections();
  } //------------Added by Feng
      
    VM_GenCollectForAllocation op(size,
                                //  is_large_noref,
                                  is_tlab,
                                  gc_count_before);
    VMThread::execute(&op);
    if (op.prologue_succeeded()) {
      result = op.result();
      assert(result == NULL || gch->is_in(result), "result not in heap");
      return result;
    }

    // Give a warning if we seem to be looping forever.
    if ((QueuedAllocationWarningCount > 0) &&
        (try_count % QueuedAllocationWarningCount == 0)) {
          warning("TwoGenerationCollectorPolicy::mem_allocate_work retries %d times \n\t"
		  " size=%d %s", try_count, size, is_tlab ? "(TLAB)" : "");
    }
  }
}


 /*-tony 01/18/2010 */


 

 /*tony 11/16/2009 Porting Type Based GC, Tony*/ 
//Added by Feng, Jan, 2006
//fixme: need to check the bool* gc_overhead_limit_was_exceeded in GCs! 2010-2-10
HeapWord* TwoGenerationCollectorPolicy::mem_allocate_with_pretenuring(size_t size,
                                                       //   bool is_large_noref,
                                                          bool is_tlab,
                                                          bool pretenuring
                                                          //bool* gc_overhead_limit_was_exceeded
                                                          ) {
 
  //If the object is not due to pretenuring, it will be allocated in the normal
  //way, or else it will be allocated directly in the mature space.
//  if(!pretenuring) return mem_allocate_work(size, is_large_noref, is_tlab);
 bool	gc_overhead_limit_was_exceeded;
    if(!pretenuring)
		{
		if(TransGC)
			{
			gclog_or_tty->print_cr("TwoGenerationCollectorPolicy::mem_allocate_with_pretenuring() calling mem_allocate_work");
			}
		return mem_allocate_work(size, is_tlab,&gc_overhead_limit_was_exceeded);//,&gc_overhead_limit_was_exceeded);//by tony
    	}
  GenCollectedHeap *gch = GenCollectedHeap::heap();

//  debug_only(gch->check_for_valid_allocation_state());

  assert(gch->no_gc_in_progress(), "Allocation during gc not allowed");
  HeapWord* result = NULL;

  for (int try_count = 1; ; try_count += 1) {
   bool trigger_gc=false;
   int gc_count_before;

   if(!NormalGCInvoke && ( (1 + Universe::total_obj_count())%GCFreq ==0 ) ){
        trigger_gc = true;      //We trigger GC every GCFreq allocations; do nothing and invoke GC
   }
   if(!trigger_gc){      
        MutexLocker ml(Heap_lock);

        // Note that only large objects get a shot at being
        // allocated in later generations.  If jvmpi slow allocation
        // is enabled, allocate in later generations (since the
        // first generation is always full.
      
        for (int i = 1; i < gch->n_gens(); i++) {
	  Generation *geni = gch->get_gen(i);  
//          if (geni->should_allocate(size, is_large_noref, is_tlab)) {
if (geni->should_allocate(size, is_tlab)) { //by tony
//               result = geni->allocate(size, is_large_noref, is_tlab);
               result = geni->allocate(size, is_tlab); //by tony out of compatibility
               if (result != NULL) return result;
          }
        }
	  
        if (result != NULL) {
          assert(gch->is_in(result), "result not in heap");
          return result;
        }

        // Read the gc count while the heap lock is held.
        gc_count_before = Universe::heap()->total_collections();      
    }//if (!trigger_gc)
  
    VM_GenCollectForAllocation op(size,
//                                  is_large_noref, //by tony compatibility
                                  is_tlab,
                                  gc_count_before);
    VMThread::execute(&op);
    if (op.prologue_succeeded()) {
      result = op.result();
      assert(result == NULL || gch->is_in(result), "result not in heap");
      return result;
    }

    // Give a warning if we seem to be looping forever.
    if ((QueuedAllocationWarningCount > 0) &&
        (try_count % QueuedAllocationWarningCount == 0)) {
          warning("TwoGenerationCollectorPolicy::mem_allocate_with_pretenuring retries %d times \n\t"
		  " size=%d %s", try_count, size, is_tlab ? "(TLAB)" : "");
    }
  }
}


 /*-tony 11/16/2009*/

 

HeapWord* GenCollectorPolicy::mem_allocate_work(size_t size,
                                        bool is_tlab,
                                        bool* gc_overhead_limit_was_exceeded) {
  GenCollectedHeap *gch = GenCollectedHeap::heap();

  debug_only(gch->check_for_valid_allocation_state());
  assert(gch->no_gc_in_progress(), "Allocation during gc not allowed");
  HeapWord* result = NULL;

  // Loop until the allocation is satisified,
  // or unsatisfied after GC.
  for (int try_count = 1; /* return or throw */; try_count += 1) {
    HandleMark hm; // discard any handles allocated in each iteration

    // First allocation attempt is lock-free.
    Generation *gen0 = gch->get_gen(0);
    assert(gen0->supports_inline_contig_alloc(),
      "Otherwise, must do alloc within heap lock");
    if (gen0->should_allocate(size, is_tlab)) {
      result = gen0->par_allocate(size, is_tlab);
      if (result != NULL) {
        assert(gch->is_in_reserved(result), "result not in heap");
        return result;
      }
    }
    unsigned int gc_count_before;  // read inside the Heap_lock locked region
    {
      MutexLocker ml(Heap_lock);
      if (PrintGC && Verbose) {
        gclog_or_tty->print_cr("TwoGenerationCollectorPolicy::mem_allocate_work:"
                      " attempting locked slow path allocation");
      }
      // Note that only large objects get a shot at being
      // allocated in later generations.
      bool first_only = ! should_try_older_generation_allocation(size);

      result = gch->attempt_allocation(size, is_tlab, first_only);
      if (result != NULL) {
        assert(gch->is_in_reserved(result), "result not in heap");
        return result;
      }

      // There are NULL's returned for different circumstances below.
      // In general gc_overhead_limit_was_exceeded should be false so
      // set it so here and reset it to true only if the gc time
      // limit is being exceeded as checked below.
      *gc_overhead_limit_was_exceeded = false;

      if (GC_locker::is_active_and_needs_gc()) {
        if (is_tlab) {
          return NULL;  // Caller will retry allocating individual object
        }
        if (!gch->is_maximal_no_gc()) {
          // Try and expand heap to satisfy request
          result = expand_heap_and_allocate(size, is_tlab);
          // result could be null if we are out of space
          if (result != NULL) {
            return result;
          }
        }

        // If this thread is not in a jni critical section, we stall
        // the requestor until the critical section has cleared and
        // GC allowed. When the critical section clears, a GC is
        // initiated by the last thread exiting the critical section; so
        // we retry the allocation sequence from the beginning of the loop,
        // rather than causing more, now probably unnecessary, GC attempts.
        JavaThread* jthr = JavaThread::current();
        if (!jthr->in_critical()) {
          MutexUnlocker mul(Heap_lock);
          // Wait for JNI critical section to be exited
          GC_locker::stall_until_clear();
          continue;
        } else {
          if (CheckJNICalls) {
            fatal("Possible deadlock due to allocating while"
                  " in jni critical section");
          }
          return NULL;
        }
      }

      // Read the gc count while the heap lock is held.
      gc_count_before = Universe::heap()->total_collections();
    }

    // Allocation has failed and a collection is about
    // to be done.  If the gc time limit was exceeded the
    // last time a collection was done, return NULL so
    // that an out-of-memory will be thrown.  Clear
    // gc_time_limit_exceeded so that subsequent attempts
    // at a collection will be made.
    if (size_policy()->gc_time_limit_exceeded()) {
      *gc_overhead_limit_was_exceeded = true;
      size_policy()->set_gc_time_limit_exceeded(false);
      return NULL;
    }

    VM_GenCollectForAllocation op(size,
                                  is_tlab,
                                  gc_count_before);
    VMThread::execute(&op);
    if (op.prologue_succeeded()) {
      result = op.result();
      if (op.gc_locked()) {
         assert(result == NULL, "must be NULL if gc_locked() is true");
         continue;  // retry and/or stall as necessary
      }
      assert(result == NULL || gch->is_in_reserved(result),
             "result not in heap");
      return result;
    }

    // Give a warning if we seem to be looping forever.
    if ((QueuedAllocationWarningCount > 0) &&
        (try_count % QueuedAllocationWarningCount == 0)) {
          warning("TwoGenerationCollectorPolicy::mem_allocate_work retries %d times \n\t"
                  " size=%d %s", try_count, size, is_tlab ? "(TLAB)" : "");
    }
  }
}

HeapWord* GenCollectorPolicy::expand_heap_and_allocate(size_t size,
                                                       bool   is_tlab) {
  GenCollectedHeap *gch = GenCollectedHeap::heap();
  HeapWord* result = NULL;
  for (int i = number_of_generations() - 1; i >= 0 && result == NULL; i--) {
    Generation *gen = gch->get_gen(i);
    if (gen->should_allocate(size, is_tlab)) {
      result = gen->expand_and_allocate(size, is_tlab);
    }
  }
  assert(result == NULL || gch->is_in_reserved(result), "result not in heap");
  return result;
}

HeapWord* GenCollectorPolicy::satisfy_failed_allocation(size_t size,
                                                        bool   is_tlab) {
  GenCollectedHeap *gch = GenCollectedHeap::heap();
  GCCauseSetter x(gch, GCCause::_allocation_failure);
  HeapWord* result = NULL;

  assert(size != 0, "Precondition violated");
  if (GC_locker::is_active_and_needs_gc()) {
    // GC locker is active; instead of a collection we will attempt
    // to expand the heap, if there's room for expansion.
    if (!gch->is_maximal_no_gc()) {
      result = expand_heap_and_allocate(size, is_tlab);
    }
    return result;   // could be null if we are out of space
  } else if (!gch->incremental_collection_will_fail()) {
    // The gc_prologues have not executed yet.  The value
    // for incremental_collection_will_fail() is the remanent
    // of the last collection.
    // Do an incremental collection.
    gch->do_collection(false            /* full */,
                       false            /* clear_all_soft_refs */,
                       size             /* size */,
                       is_tlab          /* is_tlab */,
                       number_of_generations() - 1 /* max_level */);
  } else {
    // Try a full collection; see delta for bug id 6266275
    // for the original code and why this has been simplified
    // with from-space allocation criteria modified and
    // such allocation moved out of the safepoint path.
    gch->do_collection(true             /* full */,
                       false            /* clear_all_soft_refs */,
                       size             /* size */,
                       is_tlab          /* is_tlab */,
                       number_of_generations() - 1 /* max_level */);
  }

  result = gch->attempt_allocation(size, is_tlab, false /*first_only*/);

  if (result != NULL) {
    assert(gch->is_in_reserved(result), "result not in heap");
    return result;
  }

  // OK, collection failed, try expansion.
  result = expand_heap_and_allocate(size, is_tlab);
  if (result != NULL) {
    return result;
  }

  // If we reach this point, we're really out of memory. Try every trick
  // we can to reclaim memory. Force collection of soft references. Force
  // a complete compaction of the heap. Any additional methods for finding
  // free memory should be here, especially if they are expensive. If this
  // attempt fails, an OOM exception will be thrown.
  {
    IntFlagSetting flag_change(MarkSweepAlwaysCompactCount, 1); // Make sure the heap is fully compacted

    gch->do_collection(true             /* full */,
                       true             /* clear_all_soft_refs */,
                       size             /* size */,
                       is_tlab          /* is_tlab */,
                       number_of_generations() - 1 /* max_level */);
  }

  result = gch->attempt_allocation(size, is_tlab, false /* first_only */);
  if (result != NULL) {
    assert(gch->is_in_reserved(result), "result not in heap");
    return result;
  }

  // What else?  We might try synchronous finalization later.  If the total
  // space available is large enough for the allocation, then a more
  // complete compaction phase than we've tried so far might be
  // appropriate.
  return NULL;
}

size_t GenCollectorPolicy::large_typearray_limit() {
  return FastAllocateSizeLimit;
}

// Return true if any of the following is true:
// . the allocation won't fit into the current young gen heap
// . gc locker is occupied (jni critical section)
// . heap memory is tight -- the most recent previous collection
//   was a full collection because a partial collection (would
//   have) failed and is likely to fail again
bool GenCollectorPolicy::should_try_older_generation_allocation(
        size_t word_size) const {
  GenCollectedHeap* gch = GenCollectedHeap::heap();
  size_t gen0_capacity = gch->get_gen(0)->capacity_before_gc();
  return    (word_size > heap_word_size(gen0_capacity))
         || (GC_locker::is_active_and_needs_gc())
         || (   gch->last_incremental_collection_failed()
             && gch->incremental_collection_will_fail());
}


//
// MarkSweepPolicy methods
//

MarkSweepPolicy::MarkSweepPolicy() {
  initialize_all();
}

void MarkSweepPolicy::initialize_generations() {
  initialize_perm_generation(PermGen::MarkSweepCompact);
  _generations = new GenerationSpecPtr[number_of_generations()];
  if (_generations == NULL)
    vm_exit_during_initialization("Unable to allocate gen spec");

  if (UseParNewGC && ParallelGCThreads > 0) {
    _generations[0] = new GenerationSpec(Generation::ParNew, _initial_gen0_size, _max_gen0_size);
  } else {
    _generations[0] = new GenerationSpec(Generation::DefNew, _initial_gen0_size, _max_gen0_size);
  }
  _generations[1] = new GenerationSpec(Generation::MarkSweepCompact, _initial_gen1_size, _max_gen1_size);

  if (_generations[0] == NULL || _generations[1] == NULL)
    vm_exit_during_initialization("Unable to allocate gen spec");
}

void MarkSweepPolicy::initialize_gc_policy_counters() {
  // initialize the policy counters - 2 collectors, 3 generations
  if (UseParNewGC && ParallelGCThreads > 0) {
    _gc_policy_counters = new GCPolicyCounters("ParNew:MSC", 2, 3);
  }
  else {
    _gc_policy_counters = new GCPolicyCounters("Copy:MSC", 2, 3);
  }
}



 /*tony 11/16/2009 Porting Type Based GC, Tony*/ 

//TypeGenCollectorPolicy methods. Added by Feng at Feb 1, 2006

TypeGenCollectorPolicy::TypeGenCollectorPolicy(){
  this->TwoGenerationCollectorPolicy::initialize_flags();
  initialize_size_info();
  initialize_generations();
}

void TypeGenCollectorPolicy::initialize_size_info(){
  this->TwoGenerationCollectorPolicy::initialize_size_info();
  if(reverseRemoteRatio){
      _initial_local_gen_size= align_size_down(_initial_gen0_size*RemoteRatio /(RemoteRatio+1),
                                        min_alignment());
      _max_local_gen_size= align_size_down(_max_gen0_size*RemoteRatio /(RemoteRatio+1),
                                        min_alignment());  //???
  }
  else {
      _initial_local_gen_size= align_size_down(_initial_gen0_size / (RemoteRatio+1),
					min_alignment());
      _max_local_gen_size= align_size_down(_max_gen0_size / (RemoteRatio+1),
					min_alignment());  //???
  }

  _initial_remote_gen_size = _initial_gen0_size -  _initial_local_gen_size ;
  _max_remote_gen_size  = _max_gen0_size  -  _max_local_gen_size;	
}

void TypeGenCollectorPolicy::initialize_generations() {
  _generations = new GenerationSpecPtr[number_of_generations()];
  if (_generations == NULL)
    vm_exit_during_initialization("Unable to allocate gen spec");
  
  if (UseParNewGC && ParallelGCThreads > 0) {
    gclog_or_tty->print_cr("Using ParNew");
    _generations[0] = new GenerationSpec(Generation::DefNew, _initial_local_gen_size, _max_local_gen_size);
//    _generations[1] = new GenerationSpec(Generation::TransNew, _initial_remote_gen_size, _max_remote_gen_size);	
   _generations[1] = new TransGenerationSpec(Generation::TransNew, _initial_remote_gen_size, _max_remote_gen_size);	
  } else {
    _generations[0] = new GenerationSpec(Generation::DefNew, _initial_local_gen_size, _max_local_gen_size);
    _generations[1] = new TransGenerationSpec(Generation::TransNew, _initial_remote_gen_size, _max_remote_gen_size);	
  }
  _generations[2] = new GenerationSpec(Generation::MarkSweepCompact, _initial_gen1_size, _max_gen1_size);

  initialize_perm_generation(PermGen::MarkSweepCompact);


  if (_generations[0] == NULL || _generations[1] == NULL || _generations[2] == NULL ||_permanent_generation == NULL)
    vm_exit_during_initialization("Unable to allocate gen spec");
}

void TypeGenCollectorPolicy::initialize_gc_policy_counters() {

  // initialize the policy counters - 2 collectors, 4 generations

  if (UseParNewGC && ParallelGCThreads > 0) {
    _gc_policy_counters = new GCPolicyCounters("ParNew:TypeGen", 2, 4);
  }
  else {
    _gc_policy_counters = new GCPolicyCounters("Copy:TypeGen", 2, 4);
  }
}

 //tony-> 2010-09-21

  HeapWord* TypeGenCollectorPolicy::mem_allocate_work(size_t size,
                             // bool is_large_noref, //by tony
                              bool is_tlab, 
                              bool* gc_overhead_limit_was_exceeded //by tony 2010-2-10
                              ){
	return mem_allocate_with_pretenuring(size,is_tlab,false);
 }
 //<-tony 2010-09-21

//Added by Feng, Feb, 2006. Allocate remote objects directly in Gen[1] and local objects in Gen[0]

//fixme: need to add gc_overhead_limit_was_exceeded as parameter later!!

HeapWord* TypeGenCollectorPolicy::mem_allocate_with_pretenuring(size_t size,
                                                          bool is_tlab,
                                                          bool pretenure) {                                                      
  
  GenCollectedHeap *gch = GenCollectedHeap::heap();
 

  debug_only(gch->check_for_valid_allocation_state());


  assert(gch->no_gc_in_progress(), "Allocation during gc not allowed");


  HeapWord* result = NULL;


  Generation *gen_young = gch->get_gen(0);

 //tony-> 2010-09-21
  bool remote = false;


  Thread* t = get_thread();
  //here, if it's pretenure, the tlab should be false.
  //also, if it's tlab, pretenure should be false.
  

// if pretenure is set, we still need to make sure that only objects allocated by transaction be pretenured.
		 if(t&&t->is_Java_thread()&&((JavaThread*)t)->hasValidTransID())	//by tony 2010-11-09 applying the residual training result
		 	remote=true; //overriding the remote, restricting the remote allocation to transaction thread only.//by tony 2010-07-26
		 else
		 	remote=false; ////by tony 2010-09-16 if there is a validtransID associcated with this thread, allocate into transEden.

   if(PretenureToOld){ //this is a swith to control whether we want to pretenure to Old gen or local young.
		    if(remote&&pretenure&&gch->get_gen(2)->should_allocate(size,is_tlab)) // 1.remote 2.pretenure then we do pretenure.
		  	{
		  	 //tony-> 2011/02/20 Trying to solve the problem for PretenureToOld.
				//result=gch->get_gen(2)->allocate(size,is_tlab);
				result=gch->get_gen(2)->par_allocate(size,is_tlab);
			  //<-tony 2011-02-20
		         if (result != NULL) {
		          assert(gch->is_in(result), "result not in heap");
			      //if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] pretenured!%d",size);}
		          return result;
		         }

		  	}
   	}
    //temporarily disabled this section, to identify the problem, curretly, if we pretenure to old generation, there will be some problem collecting the 
    //transEdens, scanning the old generation will report an error.
    //FIXME:need to solve it later.
        //instead of pretenure into old gen, we allocate the pretenured objects into local young.

  if(pretenure&&remote) //if we are here, the object is not pretenured, if pretenure is set, we should allocate into local.
  	{
	//   if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] pretenure failed!%d",size);}

		 remote=false;
		 //if pretenure fails, we prefer to allocate into local young.
  	}



 //<-tony 2010-09-21
  

//  if(DebugTypeGenGC){gclog_or_tty->print("R:%d",remote);}
  
 //tony-> 2010-08-03 Collecting transEdens  
  if(remote)
  	{
  	//now, check if we need to collect the remote young
  	
  	if(gch->get_gen(1)->as_TransNewGeneration()->getCollectableTransEdenCount()>=1&&gch->get_gen(1)->as_TransNewGeneration()->getAvailableTransEdenCount()<1)
  		{
	        //MutexLocker ml(Heap_lock);

			 //tony-> 2011/02/18 Trying to not stop the world when a trained transaction is over.
			 if(!gch->get_gen(1)->as_TransNewGeneration()->collectResettableTransEden()) //we prefer resetting those trained transEdens first without stopping the threads.
				  		gch->invoke_transEden_collection(2);
			  //<-tony 2011-02-18
			
			
  		}
  	if(!gch->get_gen(1)->should_allocate(size,is_tlab)){//check if we have space for the allocation in remote young
		 	gen_young=gch->get_gen(0); //0-allocate in the normal young
			remote=false;		
			//if(DebugTypeGenGC){gclog_or_tty->print("R->L");}
	   	}
	else{
		 	gen_young=gch->get_gen(1); //0-allocate in the normal young
		// 	if(DebugTypeGenGC){gclog_or_tty->print("R->R");}
		}
  	}

//<-tony 2010-08-03/

  bool first_only=true; //try young generations first
  // Loop until the allocation is satisified,
  // or unsatisfied after GC.
  for (int try_count = 1; /* return or throw */; try_count += 1) {
    bool trigger_gc=false;
    int gc_count_before;

    if(!NormalGCInvoke && ( (1+Universe::local_obj_count()+Universe::remote_obj_count())%GCFreq ==0 ) ){
        trigger_gc = true; //do not attemp an allocation, just simply invoke a full GC
        if(DebugTypeGenGC)
			{gclog_or_tty->print("trigger_gc set to be true!");}
    }

    if(!trigger_gc){
      assert(gen_young->supports_inline_contig_alloc(),
             "Otherwise, must do alloc within heap lock");

	      if (gen_young->should_allocate(size, is_tlab)) {//by tony
			           result = gen_young->par_allocate(size,  is_tlab);//by tony
			           if (result != NULL) {
						 //if(!gch->is_in(result))//						 	if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]allocated address:"INTPTR_FORMAT"remote?%d, gen_young=remote?%d", result,remote,gen_young==gch->get_gen(1));}
			             assert(gch->is_in(result), "result not in heap");
			             return result;
			           	}

					   //	if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]mem_allocate_with_pretenuring, young alloc failed! remote:%d,is_tlab:%d,size:%d, pretenure:%d",remote,is_tlab,size,pretenure);}

					   //the fact that we are here means the allocation failed, now check if it's remote young gen.

					   /*tony 2010-3-2 TransGC, Tony*/ 
						/*
						if(remote&&gen_young==gch->get_gen(1)){
							if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]try the local young if pretenuring failes.");}
							gen_young=gch->get_gen(0); //0-allocate in the normal young
							
							}
						*/
					   /*-tony 2010-3-2 */
			   
	      }

	 
	  
      //int gc_count_before;  // read inside the Heap_lock locked region
      {
        MutexLocker ml(Heap_lock);

        // Note that only large objects get a shot at being
        // allocated in later generations.  If jvmpi slow allocation
        // is enabled, allocate in later generations (since the
        // first generation is always full.
       // bool first_only = ! should_try_older_generation_allocation(size);
		if(try_count>=2)
			first_only=false; //third time, consider the old generation
        result = gch->attempt_type_based_allocation(size, 
//                                       is_large_noref, 
                                       is_tlab, 
                                       first_only,
                                       remote);  
		/*tony 03/01/2010 Fixing Type Based GC, Tony*/ 
		/*-tony 03/01/2010 */
        if (result != NULL) {
          assert(gch->is_in(result), "result not in heap");
//	      if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] satisfied in attempt_type_based_allocation");}
          return result;
        }

		if(remote)
			{
						//still we cannot allocate into remote young or old generation,we have to consider the young generation
						result = gch->attempt_type_based_allocation(size, 
				                                       is_tlab, 
				                                       first_only,
				                                       false); //but this time, change the remote to be false.  
						if (result != NULL) {
				          assert(gch->is_in(result), "result not in heap");
					      //if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] satisfied in attempt_type_based_allocation R->L");}
				          return result;
				        }
						  //if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] Not satisfied in attempt_type_based_allocation R->L is_tlab=%d, size=%d",is_tlab,size);}
		
			}
		
        // Read the gc count while the heap lock is held.
        gc_count_before = Universe::heap()->total_collections();
      	}
    }
    if(!remote||try_count>=3)//by tony 2010-10-11 no gc if remote allocation fails. or even if it's remote, we still need to collect, this case, a full GC may be desirable.
    	{ //we don't do the gc when a remote request is not satisfied,risky?
							    VM_TypeGenCollectForAllocation op(size,
									//                                  is_large_noref,
									                                  is_tlab,
									                                  gc_count_before,
									                                  remote);
									    VMThread::execute(&op);
									    if (op.prologue_succeeded()) {
									      result = op.result();
									      assert(result == NULL || gch->is_in(result), "result not in heap");
									      return result;
									    }
						    // Give a warning if we seem to be looping forever.
						    if ((QueuedAllocationWarningCount > 0) &&
						        (try_count % QueuedAllocationWarningCount == 0)) {
						          warning("TypeGenCollectorPolicy::mem_allocate_work_with_pretenuring retries %d times \n\t"
								  " size=%d %s", try_count, size, is_tlab ? "(TLAB)" : "");
						    }
	    }
  	}
}


HeapWord* TypeGenCollectorPolicy::satisfy_failed_type_based_allocation(size_t size,
                                                             //     bool   is_large_noref,
                                                                  bool   is_tlab,
                                                               //   bool&  notify_ref_lock,
                                                                  bool remote) {
  GenCollectedHeap *gch = GenCollectedHeap::heap();
  GCCauseSetter x(gch, GCCause::_allocation_failure);
  HeapWord* result = NULL;

 if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]satisfy_failed_type_based_allocationremote:%d,is_tlab:%d,size:%d",remote,is_tlab,size);}
  
  if (!gch->incremental_collection_will_fail()||!remote) {//by tony 2010-10-11 no gc if remote allocation fails.
    // Do an incremental collection.

	/*tony 01/18/2010 Porting Type Based GC, Tony*/ 

	
	gch->do_type_based_collection(false,
                                     false,
                                     size,
                                     is_tlab,
                                     number_of_generations() - 1,
                                    false);////by tony 2010-12-27 pass false to remote for ever.
  /*-tony 01/18/2010 */
} else {
    // The incremental_collection_will_fail flag is set if the
    // next incremental collection will not succeed (e.g., the
    // DefNewGeneration didn't think it had space to promote all
    // its objects). However, that last incremental collection
    // continued, allowing all older generations to collect (and
    // perhaps change the state of the flag).
    // 
    // If we reach here, we know that an incremental collection of
    // all generations left us in the state where incremental collections
    // will fail, so we just try allocating the requested space. 
    // If the allocation fails everywhere, force a full collection.
    // We're probably very close to being out of memory, so forcing many
    // collections now probably won't help.
    if (PrintGC && Verbose) {
      gclog_or_tty->print_cr("TypeGenCollectorPolicy::satisfy_failed_type_based_allocation:"
                    " attempting allocation anywhere before full collection");
    }
    bool trigger_gc=false;

/*
    if(!NormalGCInvoke && ( (1+Universe::local_obj_count()+Universe::remote_obj_count())%GCFreq ==0 ) ){
        trigger_gc = true; //do not attemp an allocation, just simply invoke a full GC
    }
*/
    if(!trigger_gc){
       result = gch->attempt_type_based_allocation(size, 
                                   //  is_large_noref, 
                                     is_tlab, 
                                     false /* first_only */,
                                     remote);
       if (result != NULL) {
          assert(gch->is_in(result), "result not in heap");
  	  //    if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] satisfied in attempt_type_based_allocation");}
          return result;
       }
    }

    // Allocation request hasn't yet been met; try a full collection.
    gch->do_type_based_collection(true             /* full */, 
                       false            /* clear_all_soft_refs */, 
                       size             /* size */, 
                       is_tlab          /* is_tlab */,
                       number_of_generations() - 1 /* max_level */, 
                       remote);
  }
  
  result = gch->attempt_type_based_allocation(size, is_tlab, false /*first_only*/, remote);
  
  if (result != NULL) {
  	
    assert(gch->is_in(result), "result not in heap");
    //if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] satisfied in attempt_type_based_allocation");}
    return result;
  }
  
  // OK, collection failed, try expansion. There are some bugs in this piece of code. Feng, Feb, 2006
  for (int i = number_of_generations() - 1 ; i>= 0; i--) {

    Generation *gen = gch->get_gen(i);

 /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
 //  if (gen->should_allocate(size, is_large_noref, is_tlab)) {
   //   result = gen->expand_and_allocate(size, is_large_noref, is_tlab);

    if (gen->should_allocate(size,  is_tlab)) {
      result = gen->expand_and_allocate(size, is_tlab);
 
 /*-tony 01/18/2010 */
 
      if (result != NULL) {
        assert(gch->is_in(result), "result not in heap");
  //     if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] satisfied after expanding gen(%d)",i);}
        return result;
      }
    }
  }
  
  // If we reach this point, we're really out of memory. Try every trick
  // we can to reclaim memory. Force collection of soft references. Force
  // a complete compaction of the heap. Any additional methods for finding
  // free memory should be here, especially if they are expensive. If this
  // attempt fails, an OOM exception will be thrown.
  {
    IntFlagSetting flag_change(MarkSweepAlwaysCompactCount, 1); // Make sure the heap is fully compacted

    gch->do_type_based_collection(true             /* full */,
                       true             /* clear_all_soft_refs */,
                       size             /* size */,
                       is_tlab          /* is_tlab */,
                       number_of_generations() - 1 /* max_level */,
                       remote);
  }

//  result = gch->attempt_type_based_allocation(size, is_large_noref, is_tlab, false /* first_only */, remote);
  result = gch->attempt_type_based_allocation(size, is_tlab, false /* first_only */, remote);
  if (result != NULL) {
    assert(gch->is_in(result), "result not in heap");
//    if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY] satisfied in attempt_type_based_allocation after a full GC");}
    return result;
  }
  
  // What else?  We might try synchronous finalization later.  If the total
  // space available is large enough for the allocation, then a more
  // complete compaction phase than we've tried so far might be
  // appropriate.
  return NULL;
}


bool TypeGenCollectorPolicy::should_try_older_generation_allocation(size_t word_size) const {
  return (word_size > heap_word_size(_max_local_gen_size)) || (word_size > heap_word_size(_max_remote_gen_size));
//    || Universe::jvmpi_slow_allocation(); //by tony
}


 /*-tony 11/16/2009*/
 
