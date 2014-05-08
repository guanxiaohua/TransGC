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
# include "incls/_transNewGeneration.cpp.incl"

//
//TransNewGeneration over  DefNewGeneration functions.tony,2010-6-14

// Methods of protected closure types.

TransNewGeneration::IsAliveClosure::IsAliveClosure(Generation* g) : _g(g),_ed(NULL) {

  assert( g->level()==1 , "Optimized for remote young gen."); //by tony 2010-08-12
   
}
TransNewGeneration::IsAliveClosure::IsAliveClosure(EdenSpace* ed) : _ed(ed),_g(NULL) {

 assert(_ed!=NULL,"we can do nothing if the eden space is NULL.");//by tony 2010-08-12
   
}
void TransNewGeneration::IsAliveClosure::do_object(oop p) {
  assert(false, "Do not call.");
}
bool TransNewGeneration::IsAliveClosure::do_object_b(oop p) {

 /*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
	//  return (HeapWord*)p >= _g->reserved().end() || p->is_forwarded();
if(_g)
    return (!_g->is_in_reserved(p)) || p->is_forwarded();
else
	return (!_ed->is_in_reserved(p))||p->is_forwarded();
 /*-tony 02/06/2010 */
 

}

TransNewGeneration::KeepAliveClosure::
KeepAliveClosure(ScanWeakRefClosure* cl) : _cl(cl) {
  GenRemSet* rs = GenCollectedHeap::heap()->rem_set();
  assert(rs->rs_kind() == GenRemSet::CardTable, "Wrong rem set kind.");
  _rs = (CardTableRS*)rs;
}

void TransNewGeneration::KeepAliveClosure::do_oop(oop* p)       { TransNewGeneration::KeepAliveClosure::do_oop_work(p); }
void TransNewGeneration::KeepAliveClosure::do_oop(narrowOop* p) { TransNewGeneration::KeepAliveClosure::do_oop_work(p); }


TransNewGeneration::FastKeepAliveClosure::
FastKeepAliveClosure(TransNewGeneration* g, ScanWeakRefClosure* cl) :
  TransNewGeneration::KeepAliveClosure(cl) {
  _boundary = g->reserved().end();
}

void TransNewGeneration::FastKeepAliveClosure::do_oop(oop* p)       { TransNewGeneration::FastKeepAliveClosure::do_oop_work(p); }
void TransNewGeneration::FastKeepAliveClosure::do_oop(narrowOop* p) { TransNewGeneration::FastKeepAliveClosure::do_oop_work(p); }

TransNewGeneration::EvacuateFollowersClosure::
EvacuateFollowersClosure(GenCollectedHeap* gch, int level,
                         ScanClosure* cur, ScanClosure* older) :
  _gch(gch), _level(level),
  _scan_cur_or_nonheap(cur), _scan_older(older)
{}

void TransNewGeneration::EvacuateFollowersClosure::do_void() {
  do {
    _gch->oop_since_save_marks_iterate(_level, _scan_cur_or_nonheap,
                                       _scan_older);
  } while (!_gch->no_allocs_since_save_marks(_level));
}

TransNewGeneration::FastEvacuateFollowersClosure::
FastEvacuateFollowersClosure(GenCollectedHeap* gch, int level,
                             TransNewGeneration* gen,
                             FastScanClosure* cur, FastScanClosure* older) :
  _gch(gch), _level(level), _gen(gen),
  _scan_cur_or_nonheap(cur), _scan_older(older)
{}

void TransNewGeneration::FastEvacuateFollowersClosure::do_void() {
  do {
    _gch->oop_since_save_marks_iterate(_level, _scan_cur_or_nonheap,
                                       _scan_older);
  } while (!_gch->no_allocs_since_save_marks(_level));
  guarantee(_gen->promo_failure_scan_stack() == NULL
            || _gen->promo_failure_scan_stack()->length() == 0,
            "Failed to finish scan");
}

 /*tony 2010-06-14 TransGCDefNew, Tony*/ 
  //the following methods are removed because they are already defined in defNewGeneration.cpp
/*
ScanClosure::ScanClosure(Generation* g, bool gc_barrier) :
  OopsInGenClosure(g), _g(g), _gc_barrier(gc_barrier)
{

 //tony 02/06/2010 Fixing Type Based GC, Tony
 assert(_g->level() == 0 ||( TransGC&& g->level()==1 ), "Optimized for youngest generation");
 
 //-tony 02/06/2010 
 
  _boundary = _g->reserved().end();
}

void ScanClosure::do_oop(oop* p)       { ScanClosure::do_oop_work(p); }
void ScanClosure::do_oop(narrowOop* p) { ScanClosure::do_oop_work(p); }

FastScanClosure::FastScanClosure(Generation* g, bool gc_barrier) :
  OopsInGenClosure(g), _g(g), _gc_barrier(gc_barrier)
{

 //tony 02/06/2010 Fixing Type Based GC, Tony
 assert(_g->level() == 0 ||(TransGC && g->level()==1 ), "Optimized for youngest generation");
 
 //-tony 02/06/2010 
 
  _boundary = _g->reserved().end();
}

void FastScanClosure::do_oop(oop* p)       { FastScanClosure::do_oop_work(p); }
void FastScanClosure::do_oop(narrowOop* p) { FastScanClosure::do_oop_work(p); }

ScanWeakRefClosure::ScanWeakRefClosure(Generation* g) :
  OopClosure(g->ref_processor()), _g(g)
{
  assert(_g->level() == 0 ||(TransGC && g->level()==1 ), "Optimized for youngest generation");
  _boundary = _g->reserved().end();
}

void ScanWeakRefClosure::do_oop(oop* p)       { ScanWeakRefClosure::do_oop_work(p); }
void ScanWeakRefClosure::do_oop(narrowOop* p) { ScanWeakRefClosure::do_oop_work(p); }

void FilteringClosure::do_oop(oop* p)       { FilteringClosure::do_oop_work(p); }
void FilteringClosure::do_oop(narrowOop* p) { FilteringClosure::do_oop_work(p); }


 */
 /*-tony 2010-06-14 */
 
TransNewGeneration::TransNewGeneration(ReservedSpace rs,
                                   size_t initial_size,
                                   int level,
                                   const char* policy)
  : Generation(rs, initial_size, level),
    _objs_with_preserved_marks(NULL),
    _preserved_marks_of_objs(NULL),
    _promo_failure_scan_stack(NULL),
    _promo_failure_drain_in_progress(false),
    _should_allocate_from_space(false),
    _age_table(true,false),//by tony 2010-6-22, to avoid repeative age_table name, we add another parameter for the construction of age_table
    _transEdens(NULL),
    _trans_eden_counters(NULL),
    _gcCount(0)
{

  MemRegion cmr((HeapWord*)_virtual_space.low(),
                (HeapWord*)_virtual_space.high());
  
	  
  Universe::heap()->barrier_set()->resize_covered_region(cmr);

 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony

  _transEdens=new TransEden*[MAX_TRANSEDEN];
 assert(_transEdens!=NULL,"failed to allocate transEdens.");
  for(int i=0;i<MAX_TRANSEDEN;i++){
	_transEdens[i]=new TransEden(this);

  }
   _assignedCount=0;
   _collectableCount=0;
   _availableCount=MAX_TRANSEDEN;
  //<-tony 2010-07-04/
 //tony-> 2010-09-26 Removing eden space from TransNewGeneration  
 /*
  if (GenCollectedHeap::heap()->collector_policy()->has_soft_ended_eden()) {
    _eden_space = new ConcEdenSpace(this);
  } else {
    _eden_space = new EdenSpace(this);
  }
  */
   //<-tony 2010-09-21
  
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
	//  _from_space = new ContiguousSpace();
	//  _to_space   = new ContiguousSpace();
 
   // if (_eden_space == NULL || _from_space == NULL || _to_space == NULL)
     // vm_exit_during_initialization("Could not allocate a new gen space");
//<-tony 2010-06-14/
 
  // Compute the maximum eden and survivor space sizes. These sizes
  // are computed assuming the entire reserved space is committed.
  // These values are exported as performance counters.
  uintx alignment = GenCollectedHeap::heap()->collector_policy()->min_alignment();
  uintx size = _virtual_space.reserved_size();
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  //_max_survivor_size = compute_survivor_size(size, alignment);
  //_max_eden_size = size - (2*_max_survivor_size);

	_max_survivor_size = 0;
	_max_eden_size = size/(MAX_TRANSEDEN);

 //<-tony 2010-06-14/
  

  // allocate the performance counters


 	  
  // Generation counters -- generation 0, 3 subspaces
  _gen_counters = new GenerationCounters("new", 1, level, &_virtual_space);   //ordinal=level instead of 0/1, so that we have the choice to put this transNew into other place,//by tony 2010-06-24
  _gc_counters = new CollectorCounters(policy,level);

 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
	 /*
	  _eden_counters = new CSpaceCounters("eden", 0, _max_eden_size, _eden_space,
                                      _gen_counters);
	*/
 //<-tony 2010-09-21
  _trans_eden_counters=new CSpaceCounters*[MAX_TRANSEDEN];
  for(int i=0;i<MAX_TRANSEDEN;i++){
  	_trans_eden_counters[i]=new CSpaceCounters("trans_Eden", i, _max_eden_size, eden(i),
                                      _gen_counters);
  	}
  

 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
/*
  _from_counters = new CSpaceCounters("s0", 1, _max_survivor_size, _from_space,
                                      _gen_counters);
  _to_counters = new CSpaceCounters("s1", 2, _max_survivor_size, _to_space,
                                    _gen_counters);
*/
 //<-tony 2010-06-14/ 
 
  compute_space_boundaries(0, SpaceDecorator::Clear, SpaceDecorator::Mangle);




  update_counters();

  
  _next_gen = NULL;
  _tenuring_threshold = MaxTenuringThreshold;
  _pretenure_size_threshold_words = PretenureSizeThreshold >> LogHeapWordSize;

  
}

void TransNewGeneration::compute_space_boundaries(uintx minimum_eden_size,
                                                bool clear_space,
                                                bool mangle_space) {
  uintx alignment =
    GenCollectedHeap::heap()->collector_policy()->min_alignment();

  // If the spaces are being cleared (only done at heap initialization
  // currently), the survivor spaces need not be empty.
  // Otherwise, no care is taken for used areas in the survivor spaces
  // so check.
  
   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
			//  assert(clear_space || (to()->is_empty() && from()->is_empty()),
		  	//  "Initialization of the survivor spaces assumes these are empty");

 	//<-tony 2010-06-14/
 

  // Compute sizes
  uintx size = _virtual_space.committed_size();
  
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
 /* 
  uintx survivor_size = compute_survivor_size(size, alignment);
  uintx eden_size = size - (2*survivor_size);
  assert(eden_size > 0 && survivor_size <= eden_size, "just checking");
  */
  uintx survivor_size = 0;//no survivors
  uintx eden_size = size/(MAX_TRANSEDEN); //whole space just transeden
/*
  if (eden_size < minimum_eden_size) {
    // May happen due to 64Kb rounding, if so adjust eden size back up
    minimum_eden_size = align_size_up(minimum_eden_size, alignment);
    uintx maximum_survivor_size = (size - minimum_eden_size) / 2;
    uintx unaligned_survivor_size =
      align_size_down(maximum_survivor_size, alignment);
    survivor_size = MAX2(unaligned_survivor_size, alignment);
    eden_size = size - (2*survivor_size);
    assert(eden_size > 0 && survivor_size <= eden_size, "just checking");
    assert(eden_size >= minimum_eden_size, "just checking");
  }
*/
//align it //by tony 2010-07-04
  eden_size=align_size_down(eden_size,alignment);

  char *eden_start = _virtual_space.low();


//  char *from_start = eden_start + eden_size;
 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
/*
  assert(Space::is_aligned((HeapWord*)eden_start), "checking alignment");
  assert(Space::is_aligned((HeapWord*)from_start), "checking alignment");
  //assert(Space::is_aligned((HeapWord*)to_start),   "checking alignment");

  MemRegion edenMR((HeapWord*)eden_start, (HeapWord*)from_start);
//  MemRegion fromMR((HeapWord*)from_start, (HeapWord*)to_start);
//  MemRegion toMR  ((HeapWord*)to_start, (HeapWord*)to_end);

  // A minimum eden size implies that there is a part of eden that
  // is being used and that affects the initialization of any
  // newly formed eden.
  bool live_in_eden = minimum_eden_size > 0;

  // If not clearing the spaces, do some checking to verify that
  // the space are already mangled.
  if (!clear_space) {
    // Must check mangling before the spaces are reshaped.  Otherwise,
    // the bottom or end of one space may have moved into another
    // a failure of the check may not correctly indicate which space
    // is not properly mangled.
    if (ZapUnusedHeapArea) {
      HeapWord* limit = (HeapWord*) _virtual_space.high();
      eden()->check_mangled_unused_area(limit);

//      from()->check_mangled_unused_area(limit);
//        to()->check_mangled_unused_area(limit);
    }
  }

  // Reset the spaces for their new regions.
  eden()->initialize(edenMR,
                     clear_space && !live_in_eden,
                     SpaceDecorator::Mangle);
  // If clear_space and live_in_eden, we will not have cleared any
  // portion of eden above its top. This can cause newly
  // expanded space not to be mangled if using ZapUnusedHeapArea.
  // We explicitly do such mangling here.
  if (ZapUnusedHeapArea && clear_space && live_in_eden && mangle_space) {
    eden()->mangle_unused_area();
  }
  // Set next compaction spaces.
 //  eden()->set_next_compaction_space(from());
    eden()->set_next_compaction_space(NULL);
*/
 //<-tony 2010-09-21


//tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
   // initialize the transEdens
   EdenSpace* lastEden=NULL;

  bool live_in_eden = minimum_eden_size > 0;   
  
  for(int i=0;i<MAX_TRANSEDEN;i++)
  	{
  	   MemRegion edenMR((HeapWord*)eden_start, (HeapWord*)(eden_start+eden_size));
    // Reset the spaces for their new regions.
	   eden(i)->initialize(edenMR,
                     clear_space && !live_in_eden,
                     SpaceDecorator::Mangle);
  if (ZapUnusedHeapArea && clear_space && live_in_eden && mangle_space) {
       eden(i)->mangle_unused_area();
  	}
  	if(lastEden)
      lastEden->set_next_compaction_space(eden(i));//2010-7-12
      
      eden(i)->set_next_compaction_space(NULL);
	  lastEden=eden(i); //2010-7-12
 	  eden_start+=eden_size; 

  }
    //<-tony 2010-07-04/
     
  // The to-space is normally empty before a compaction so need
  // not be considered.  The exception is during promotion
  // failure handling when to-space can contain live objects.
//  from()->set_next_compaction_space(NULL);


   /*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
  //Added by Feng

  if (DebugTypeGenGC) { 
//    gclog_or_tty->print_cr("new_arena %d Eden %p %p",1, eden()->bottom(), eden()->end());
    for(int i=0;i<MAX_TRANSEDEN;i++)
	    gclog_or_tty->print_cr("new_arena %d Trans Eden %p %p",i, eden(i)->bottom(), eden(i)->end());
	
//    tty->print_cr("new_arena %d From %p %p",2, from_start, to_start);
//    tty->print_cr("new_arena %d To %p %p",  3, to_start, to_end);	
  	}

   /*-tony 02/06/2010 */
   
  	}

void TransNewGeneration::swap_spaces() {
   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
		//now we don't need to swap them at all
		return;
   //<-tony 2010-06-14/
    /*
  ContiguousSpace* s = from();
  _from_space        = to();
  _to_space          = s;
  eden()->set_next_compaction_space(from());
  // The to-space is normally empty before a compaction so need
  // not be considered.  The exception is during promotion
  // failure handling when to-space can contain live objects.
  from()->set_next_compaction_space(NULL);

  if (UsePerfData) {
    CSpaceCounters* c = _from_counters;
    _from_counters = _to_counters;
    _to_counters = c;
  }
  */
}

bool TransNewGeneration::expand(size_t bytes) {
	 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
	 return false; //we currently don't support space expansion
	  //<-tony 2010-09-21
	  
  MutexLocker x(ExpandHeap_lock);
  HeapWord* prev_high = (HeapWord*) _virtual_space.high();
  bool success = _virtual_space.expand_by(bytes);
  if (success && ZapUnusedHeapArea) {
    // Mangle newly committed space immediately because it
    // can be done here more simply that after the new
    // spaces have been computed.
    HeapWord* new_high = (HeapWord*) _virtual_space.high();
    MemRegion mangle_region(prev_high, new_high);
    SpaceMangler::mangle_region(mangle_region);
  }

  // Do not attempt an expand-to-the reserve size.  The
  // request should properly observe the maximum size of
  // the generation so an expand-to-reserve should be
  // unnecessary.  Also a second call to expand-to-reserve
  // value potentially can cause an undue expansion.
  // For example if the first expand fail for unknown reasons,
  // but the second succeeds and expands the heap to its maximum
  // value.
  if (GC_locker::is_active()) {
    if (PrintGC && Verbose) {
      gclog_or_tty->print_cr("Garbage collection disabled, "
        "expanded heap instead");
    }
  }

  return success;
}


void TransNewGeneration::compute_new_size() {
  // This is called after a gc that includes the following generation
  // (which is required to exist.)  So from-space will normally be empty.
  // Note that we check both spaces, since if scavenge failed they revert roles.
  // If not we bail out (otherwise we would have to relocate the objects)

 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
 //do we need to change the transNewGen size? no!
 return;
  //<-tony 2010-06-14/
   
/*
  if (!from()->is_empty() || !to()->is_empty()) {
    return;
  }


  int next_level = level() + 1;
  GenCollectedHeap* gch = GenCollectedHeap::heap();
  assert(next_level < gch->_n_gens,
         "TransNewGeneration cannot be an oldest gen");

  Generation* next_gen = gch->_gens[next_level];

  size_t old_size;

  if(UseTypeGenGC)
   { old_size = gch->_gens[2]->capacity(); }
  else
   { old_size = next_gen->capacity(); } 

  size_t new_size_before = _virtual_space.committed_size();
  size_t min_new_size = spec()->init_size();
  size_t max_new_size = reserved().byte_size();
  assert(min_new_size <= new_size_before &&
         new_size_before <= max_new_size,
         "just checking");
  // All space sizes must be multiples of Generation::GenGrain.
  size_t alignment = Generation::GenGrain;

  // Compute desired new generation size based on NewRatio and
  // NewSizeThreadIncrease
  
  size_t desired_new_size = old_size/NewRatio;

  
  int threads_count = Threads::number_of_non_daemon_threads();
  size_t thread_increase_size = threads_count * NewSizeThreadIncrease;
  desired_new_size = align_size_up(desired_new_size + thread_increase_size, alignment);

  // Adjust new generation size
  desired_new_size = MAX2(MIN2(desired_new_size, max_new_size), min_new_size);
  assert(desired_new_size <= max_new_size, "just checking");

  bool changed = false;
  if (desired_new_size > new_size_before) {
    size_t change = desired_new_size - new_size_before;
    assert(change % alignment == 0, "just checking");
    if (expand(change)) {
       changed = true;
    }
    // If the heap failed to expand to the desired size,
    // "changed" will be false.  If the expansion failed
    // (and at this point it was expected to succeed),
    // ignore the failure (leaving "changed" as false).
  }
  if (desired_new_size < new_size_before && eden()->is_empty()) {
    // bail out of shrinking if objects in eden
    size_t change = new_size_before - desired_new_size;
    assert(change % alignment == 0, "just checking");
    _virtual_space.shrink_by(change);
    changed = true;
  }
  if (changed) {
    // The spaces have already been mangled at this point but
    // may not have been cleared (set top = bottom) and should be.
    // Mangling was done when the heap was being expanded.
    compute_space_boundaries(eden()->used(),
                             SpaceDecorator::Clear,
                             SpaceDecorator::DontMangle);
    MemRegion cmr((HeapWord*)_virtual_space.low(),
                  (HeapWord*)_virtual_space.high());
    Universe::heap()->barrier_set()->resize_covered_region(cmr);
    if (Verbose && PrintGC) {
      size_t new_size_after  = _virtual_space.committed_size();
      size_t eden_size_after = eden()->capacity();
      size_t survivor_size_after = from()->capacity();
      gclog_or_tty->print("New generation size " SIZE_FORMAT "K->"
        SIZE_FORMAT "K [eden="
        SIZE_FORMAT "K,survivor=" SIZE_FORMAT "K]",
        new_size_before/K, new_size_after/K,
        eden_size_after/K, survivor_size_after/K);
      if (WizardMode) {
        gclog_or_tty->print("[allowed " SIZE_FORMAT "K extra for %d threads]",
          thread_increase_size/K, threads_count);
      }
      gclog_or_tty->cr();
    }
  }
  */
}

void TransNewGeneration::object_iterate_since_last_GC(ObjectClosure* cl) {
  // $$$ This may be wrong in case of "scavenge failure"?
 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
			//  eden()->object_iterate(cl);
   //<-tony 2010-09-21

   //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony

	for(int i=0;i<MAX_TRANSEDEN;i++){
  	eden(i)->object_iterate(cl);
  	}
	 //<-tony 2010-07-04/
    
}

void TransNewGeneration::younger_refs_iterate(OopsInGenClosure* cl) {
     /*tony 01/18/2010 Porting Type Based GC, Tony*/ 

 cl->set_generation(this);

 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
	//  younger_refs_in_space_iterate(_eden_space, cl);
  //tony-> 2010-09-26 Removing eden space from TransNewGeneration


//  younger_refs_in_space_iterate(_from_space, cl);

 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
	for(int i=0;i<MAX_TRANSEDEN;i++){
  		younger_refs_in_space_iterate(eden(i), cl);
  }
	
   //<-tony 2010-07-04/
  cl->reset_generation();
  
   /*-tony 01/18/2010 */
   
}


size_t TransNewGeneration::capacity() const {
 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  size_t transEdenSize=0;
  for(int i=0;i<MAX_TRANSEDEN;i++){
  	transEdenSize+=eden(i)->capacity();
  	}

//tony-> 2010-09-26 Removing eden space from TransNewGeneration
//  return eden()->capacity()+transEdenSize;
  return transEdenSize;
 //<-tony 2010-09-21
 
   //<-tony 2010-07-04/
   
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony

  //     + from()->capacity();  // to() is only used during scavenge
  //<-tony 2010-06-14/
  
}


size_t TransNewGeneration::used() const {

  //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  size_t transEdenSize=0;
  for(int i=0;i<MAX_TRANSEDEN;i++){
  	transEdenSize+=eden(i)->used();
  	}

   //<-tony 2010-07-04/
   
	
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
//  return eden()->used()+transEdenSize;
  return transEdenSize;
 //<-tony 2010-09-21

//       + from()->used();      // to() is only used during scavenge
 //<-tony 2010-06-14/
 
}


size_t TransNewGeneration::free() const {

	
  //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  size_t transEdenSize=0;
  for(int i=0;i<MAX_TRANSEDEN;i++){
  	transEdenSize+=eden(i)->free();
  	}

   //<-tony 2010-07-04/
   
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  //tony-> 2010-09-26 Removing eden space from TransNewGeneration
//  return eden()->free()+transEdenSize;
  return transEdenSize;
    //<-tony 2010-09-21
//       + from()->free();      // to() is only used during scavenge
 //<-tony 2010-06-14/
 
}

size_t TransNewGeneration::max_capacity() const {
  const size_t alignment = GenCollectedHeap::heap()->collector_policy()->min_alignment();
  const size_t reserved_bytes = reserved().byte_size();
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
//  return reserved_bytes - compute_survivor_size(reserved_bytes, alignment);
	return reserved_bytes;
 //<-tony 2010-06-14/
 
}

//tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
//we should return the minimium free size of all edens
size_t TransNewGeneration::unsafe_max_alloc_nogc() const {

 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony

	size_t size=eden(0)->free();//eden()->free();//by tony 2010-09-26 removing eden space

	for(int i=0;i<MAX_TRANSEDEN;i++){
		if(size>eden(i)->free()) size=eden(i)->free();
	}	
  return size;
}

size_t TransNewGeneration::capacity_before_gc() const {
  return capacity();
}
//tony FIXME:Here we don't have contiguous_space across the edens because we will split the generation into transactions.
size_t TransNewGeneration::contiguous_available() const {
  return free();
}

HeapWord** TransNewGeneration::top_addr() const { 
	assert(MAX_TRANSEDEN>0,"We require that at least one transEden space in our transNewGeneration.");
	return eden(MAX_TRANSEDEN-1)->top_addr();
	}


HeapWord** TransNewGeneration::end_addr() const { 
	assert(MAX_TRANSEDEN>0,"We require that at least one transEden space in our transNewGeneration.");
		return eden(MAX_TRANSEDEN-1)->end_addr();
	}

void TransNewGeneration::object_iterate(ObjectClosure* blk) {
//  eden()->object_iterate(blk);//by tony 2010-09-26 removing eden space
    //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  for(int i=0;i<MAX_TRANSEDEN;i++){
		eden(i)->object_iterate(blk);;
  	}
   //<-tony 2010-07-04/
   
}

//check the transEden spaces, if status is collecting, don't bother to iterate the space
void TransNewGeneration::space_iterate_with_some_excluded(SpaceClosure* blk,
                                     bool usedOnly) {
//  blk->do_space(eden());//by tony 2010-09-26 removing eden space
  for(int i=0;i<MAX_TRANSEDEN;i++){
  	
  	if(getTransEden(i)->state!=TransEden::Collecting){
		 blk->do_space(eden(i));
  		}
  	}
}



void TransNewGeneration::space_iterate(SpaceClosure* blk,
                                     bool usedOnly) {

//  blk->do_space(eden());//by tony 2010-09-26 removing eden space
   //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  for(int i=0;i<MAX_TRANSEDEN;i++){
	 blk->do_space(eden(i));
	 
  	}
   //<-tony 2010-07-04/
  
   
}



// The last collection bailed out, we are running out of heap space,
// so we try to allocate the from-space, too.
HeapWord* TransNewGeneration::allocate_from_space(size_t size) {
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
 //no from space, no allocation in from space either
return NULL;
/*
  HeapWord* result = NULL;
  if (PrintGC && Verbose) {
    gclog_or_tty->print("TransNewGeneration::allocate_from_space(%u):"
                  "  will_fail: %s"
                  "  heap_lock: %s"
                  "  free: " SIZE_FORMAT,
                  size,
               GenCollectedHeap::heap()->incremental_collection_will_fail() ? "true" : "false",
               Heap_lock->is_locked() ? "locked" : "unlocked",
               from()->free());
    }
  if (should_allocate_from_space() || GC_locker::is_active_and_needs_gc()) {
    if (Heap_lock->owned_by_self() ||
        (SafepointSynchronize::is_at_safepoint() &&
         Thread::current()->is_VM_thread())) {
      // If the Heap_lock is not locked by this thread, this will be called
      // again later with the Heap_lock held.
      result = from()->allocate(size);
    } else if (PrintGC && Verbose) {
      gclog_or_tty->print_cr("  Heap_lock is not owned by self");
    }
  } else if (PrintGC && Verbose) {
    gclog_or_tty->print_cr("  should_allocate_from_space: NOT");
  }

  
  if (PrintGC && Verbose) {
    gclog_or_tty->print_cr("  returns %s", result == NULL ? "NULL" : "object");
  }
  return result;
  */
   //<-tony 2010-06-14/
   
}

HeapWord* TransNewGeneration::expand_and_allocate(size_t size,
                                                bool   is_tlab,
                                                bool   parallel) {
  // We don't attempt to expand the young generation (but perhaps we should.)
  return allocate(size, is_tlab);
}


void TransNewGeneration::collect(bool   full,
                               bool   clear_all_soft_refs,
                               size_t size,
                               bool   is_tlab) {


  assert(full || size > 0, "otherwise we don't want to collect");
  

  //we don't do generation based collection for this transNewGen

 gclog_or_tty->print_cr("usually we don't do generation based collection for this transNewGen, but now we are trying to collect only one of the collectable trans edens.");

 for(int i=0;i<MAX_TRANSEDEN;i++)
 	{
 		TransEden* te=getTransEden(i);
 		if(te&&te->state==TransEden::Collectable)
 			{
				collectTransEden(i);		
				return;
 			}
 	}
		

// collectTransEdens();//by tony 2010-09-26
 return;


  GenCollectedHeap* gch = GenCollectedHeap::heap();

   /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
		 //_next_gen = gch->next_gen(this);

		  //Added by Feng. Feb 2006
		  if (gch->collector_policy()->is_typegen_policy() )
		    _next_gen = gch->get_gen(2);
		  else
		    _next_gen = gch->next_gen(this);

   /*-tony 01/18/2010 */
   
  assert(_next_gen != NULL,
    "This must be the youngest gen, and not the only gen");

  // If the next generation is too full to accomodate promotion
  // from this generation, pass on collection; let the next generation
  // do it.
  if (!collection_attempt_is_safe()) {
    gch->set_incremental_collection_will_fail();
	if(DebugTypeGenGC){
		gclog_or_tty->print_cr("Bailing out the transNewGen collection because collection_attempt_is_not_safe");
		}
    return;
  }
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony	
			//  assert(to()->is_empty(), "Else not collection_attempt_is_safe");
 //<-tony 2010-06-14/
 
  init_assuming_no_promotion_failure();

  TraceTime t1("GC", PrintGC && !PrintGCDetails, true, gclog_or_tty);
  // Capture heap used before collection (for printing).
  size_t gch_prev_used = gch->used();

  SpecializationStats::clear();

  // These can be shared for all code paths
  IsAliveClosure is_alive(this);
  ScanWeakRefClosure scan_weak_ref(this);

  age_table()->clear();
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
		//  to()->clear(SpaceDecorator::Mangle);
  //<-tony 2010-06-14/
  
 //tony 02/06/2010 Fixing Type Based GC, Tony//
 //-tony 02/06/2010 //
 
  gch->rem_set()->prepare_for_younger_refs_iterate(false);


  assert(gch->no_allocs_since_save_marks(0),
         "save marks have not been newly set.");

  // Not very pretty.
  CollectorPolicy* cp = gch->collector_policy();

 
  //FastScanClosure fsc_with_no_gc_barrier(this, false);

  FastScanClosure fsc_with_gc_barrier(this, true);

   //tony-> 2010-08-19 Collecting transEdens

	FastScanClosure fsc_with_no_gc_barrier(this, true); //we need gc barrier for every gen
  
   
    //<-tony 2010-08-19/
    

  set_promo_failure_scan_stack_closure(&fsc_with_no_gc_barrier);
  FastEvacuateFollowersClosure evacuate_followers(gch, _level, this,
                                                  &fsc_with_no_gc_barrier,
                                                  &fsc_with_gc_barrier); 
  assert(gch->no_allocs_since_save_marks(0),
         "save marks have not been newly set.");

  gch->gen_process_strong_roots(_level,
                                true, // Process younger gens, if any, as
                                      // strong roots.
                                false,// not collecting permanent generation.
                                SharedHeap::SO_AllClasses,   
                                &fsc_with_gc_barrier,
                                &fsc_with_no_gc_barrier);

 evacuate_followers.do_void();

  FastKeepAliveClosure keep_alive(this, &scan_weak_ref);
  ReferenceProcessor* rp = ref_processor();

  rp->setup_policy(clear_all_soft_refs);


  rp->process_discovered_references(&is_alive, &keep_alive, &evacuate_followers,
                                    NULL);
  if (!promotion_failed()) {
  	
  //tony-> 2010-07-19 Collecting TransNewGen Tony
  //now we clear the eden's
//       eden()->clear(SpaceDecorator::Mangle);

     	for(int i=0;i<MAX_TRANSEDEN;i++)
   		{
		eden(i)->clear(SpaceDecorator::Mangle);
   		}

   //<-tony 2010-07-19/
   

	
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
 /*
    // Swap the survivor spaces.
    eden()->clear(SpaceDecorator::Mangle);
    from()->clear(SpaceDecorator::Mangle);
    if (ZapUnusedHeapArea) {
      // This is now done here because of the piece-meal mangling which
      // can check for valid mangling at intermediate points in the
      // collection(s).  When a minor collection fails to collect
      // sufficient space resizing of the young generation can occur
      // an redistribute the spaces in the young generation.  Mangle
      // here so that unzapped regions don't get distributed to
      // other spaces.
      to()->mangle_unused_area();
    }
    swap_spaces();

    assert(to()->is_empty(), "to space should be empty now");
	*/
 //<-tony 2010-06-14/
 
    // Set the desired survivor size to half the real survivor space
 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
  /*
    _tenuring_threshold =
      age_table()->compute_tenuring_threshold(to()->capacity()/HeapWordSize);
  */
   _tenuring_threshold =
      age_table()->compute_tenuring_threshold(0);
   //<-tony 2010-06-28/
    //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
     //<-tony 2010-07-04/
     
    if (PrintGC && !PrintGCDetails) {
      gch->print_heap_change(gch_prev_used);
    }
  } else {
    assert(HandlePromotionFailure,
      "Should not be here unless promotion failure handling is on");
    assert(_promo_failure_scan_stack != NULL &&
      _promo_failure_scan_stack->length() == 0, "post condition");

    if (PrintGCDetails) {
      gclog_or_tty->print(" (promotion failed)");
    }

    // deallocate stack and it's elements
    delete _promo_failure_scan_stack;
    _promo_failure_scan_stack = NULL;

    remove_forwarding_pointers();
	 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
	/*
    // Add to-space to the list of space to compact
    // when a promotion failure has occurred.  In that
    // case there can be live objects in to-space
    // as a result of a partial evacuation of eden
    // and from-space.
    swap_spaces();   // For the sake of uniformity wrt ParNewGeneration::collect().
    from()->set_next_compaction_space(to());
    gch->set_incremental_collection_will_fail();
	*/
	 //<-tony 2010-06-14/

	
    // Reset the PromotionFailureALot counters.
    NOT_PRODUCT(Universe::heap()->reset_promotion_should_fail();)
  }

   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  // set new iteration safe limit for the survivor spaces

   //from()->set_concurrent_iteration_safe_limit(from()->top());
   // to()->set_concurrent_iteration_safe_limit(to()->top());
  //<-tony 2010-06-14/
   
  SpecializationStats::print();
  update_time_of_last_gc(os::javaTimeMillis());
}

class RemoveForwardPointerClosure: public ObjectClosure {
public:
  void do_object(oop obj) {
    obj->init_mark();
  }
};

void TransNewGeneration::init_assuming_no_promotion_failure() {
  _promotion_failed = false;
   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
		//  from()->set_next_compaction_space(NULL);
    //<-tony 2010-06-14/
    
}

void TransNewGeneration::remove_forwarding_pointers() {
  RemoveForwardPointerClosure rspc;
  
//  eden()->object_iterate(&rspc);//by tony 2010-09-26 removing eden space from TransNewGeneration

   //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  for(int i=0;i<MAX_TRANSEDEN;i++){
		eden(i)->object_iterate(&rspc);;
  	}
  
    //<-tony 2010-07-04/
    
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
	  //from()->object_iterate(&rspc);
  //<-tony 2010-06-14/
  
  // Now restore saved marks, if any.
  if (_objs_with_preserved_marks != NULL) {
    assert(_preserved_marks_of_objs != NULL, "Both or none.");
    assert(_objs_with_preserved_marks->length() ==
           _preserved_marks_of_objs->length(), "Both or none.");
    for (int i = 0; i < _objs_with_preserved_marks->length(); i++) {
      oop obj   = _objs_with_preserved_marks->at(i);
      markOop m = _preserved_marks_of_objs->at(i);
      obj->set_mark(m);
    }
    delete _objs_with_preserved_marks;
    delete _preserved_marks_of_objs;
    _objs_with_preserved_marks = NULL;
    _preserved_marks_of_objs = NULL;
  }
}

void TransNewGeneration::preserve_mark_if_necessary(oop obj, markOop m) {
  if (m->must_be_preserved_for_promotion_failure(obj)) {
    if (_objs_with_preserved_marks == NULL) {
      assert(_preserved_marks_of_objs == NULL, "Both or none.");
      _objs_with_preserved_marks = new (ResourceObj::C_HEAP)
        GrowableArray<oop>(PreserveMarkStackSize, true);
      _preserved_marks_of_objs = new (ResourceObj::C_HEAP)
        GrowableArray<markOop>(PreserveMarkStackSize, true);
    }
    _objs_with_preserved_marks->push(obj);
    _preserved_marks_of_objs->push(m);
  }
}

void TransNewGeneration::handle_promotion_failure(oop old) {
  preserve_mark_if_necessary(old, old->mark());
  // forward to self
  old->forward_to(old);
  _promotion_failed = true;

  push_on_promo_failure_scan_stack(old);

  if (!_promo_failure_drain_in_progress) {
    // prevent recursion in copy_to_survivor_space()
    _promo_failure_drain_in_progress = true;
    drain_promo_failure_scan_stack();
    _promo_failure_drain_in_progress = false;
  }
}

oop TransNewGeneration::copy_to_survivor_space(oop old) {

if(!(is_in_reserved(old) && !old->is_forwarded())){	
	gclog_or_tty->print_cr("copy_to_survivor_space("PTR64_FORMAT")",old);
	gclog_or_tty->print_cr("is_in_reserved(old):%d",is_in_reserved(old));
	gclog_or_tty->print_cr("old->is_forwarded():%d",old->is_forwarded());
	gclog_or_tty->print_cr("it comes from gen[%d]",Universe::heap()->addr_to_gen_id(old));	
}

  assert(is_in_reserved(old) && !old->is_forwarded(),
         "shouldn't be scavenging this oop");
  size_t s = old->size();
  oop obj = NULL;

  // try allocating obj tenured directly //by tony 2010-08-31

    obj = _next_gen->promote(old, s);
    if (obj == NULL) {
					      if (!HandlePromotionFailure) {
					        // A failed promotion likely means the MaxLiveObjectEvacuationRatio flag
					        // is incorrectly set. In any case, its seriously wrong to be here!
					        vm_exit_out_of_memory(s*wordSize, "promotion");
					      }

					      handle_promotion_failure(old);
					      return old;
    	}	  

  // Done, insert forward pointer to obj in this header
  old->forward_to(obj);

  return obj;
}

void TransNewGeneration::push_on_promo_failure_scan_stack(oop obj) {
  if (_promo_failure_scan_stack == NULL) {
    _promo_failure_scan_stack = new (ResourceObj::C_HEAP)
                                    GrowableArray<oop>(40, true);
  }

  _promo_failure_scan_stack->push(obj);
}

void TransNewGeneration::drain_promo_failure_scan_stack() {
  assert(_promo_failure_scan_stack != NULL, "precondition");

  while (_promo_failure_scan_stack->length() > 0) {
     oop obj = _promo_failure_scan_stack->pop();
     obj->oop_iterate(_promo_failure_scan_stack_closure);
  }
}

void TransNewGeneration::save_marks() {
//  eden()->set_saved_mark();//by tony 2010-09-26 removing eden space from TransNewGeneration

     //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
   	for(int i=0;i<MAX_TRANSEDEN;i++)
   		{
   		eden(i)->set_saved_mark();
   		}
    //<-tony 2010-07-04/
    

   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
    /*
  to()->set_saved_mark();
  from()->set_saved_mark();
	  */
   //<-tony 2010-06-14/
   
}


void TransNewGeneration::reset_saved_marks() {
//  eden()->reset_saved_mark();//by tony 2010-09-26 removing eden space from TransNewGeneration
  
     //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
   	for(int i=0;i<MAX_TRANSEDEN;i++)
   		{
   		eden(i)->reset_saved_mark();
   		}
    //<-tony 2010-07-04/
    
   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
    /*
  to()->reset_saved_mark();
  from()->reset_saved_mark();
	  */
   //<-tony 2010-06-14/
   
}


bool TransNewGeneration::no_allocs_since_save_marks() {
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  /*
  assert(eden()->saved_mark_at_top(), "Violated spec - alloc in eden");
  
  assert(from()->saved_mark_at_top(), "Violated spec - alloc in from");
  return to()->saved_mark_at_top();
  */
  
     //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
   	for(int i=0;i<MAX_TRANSEDEN;i++)
   		{
		assert(eden(i)->saved_mark_at_top(), "Violated spec - alloc in eden");
   		}
    //<-tony 2010-07-04/
    
  return true;
 //eden()->saved_mark_at_top();//by tony 2010-09-26 removing eden space from TransNewGeneration

 //<-tony 2010-06-14/
   
}
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  /*
#define DefNew_SINCE_SAVE_MARKS_DEFN(OopClosureType, nv_suffix) \
                                                                \
void TransNewGeneration::                                         \
oop_since_save_marks_iterate##nv_suffix(OopClosureType* cl) {   \
  cl->set_generation(this);                                     \
  eden()->oop_since_save_marks_iterate##nv_suffix(cl);          \
  to()->oop_since_save_marks_iterate##nv_suffix(cl);            \
  from()->oop_since_save_marks_iterate##nv_suffix(cl);          \
  cl->reset_generation();                                       \
  save_marks();                                                 \
}

*/
#define TransNew_SINCE_SAVE_MARKS_DEFN(OopClosureType, nv_suffix) \
                                                                \
void TransNewGeneration::                                         \
oop_since_save_marks_iterate##nv_suffix(OopClosureType* cl) {   \
  cl->set_generation(this);                                     \
  for(int i=0;i<MAX_TRANSEDEN;i++){\
		eden(i)->oop_since_save_marks_iterate##nv_suffix(cl);\
  	}\
  cl->reset_generation();                                       \
  save_marks();                                                 \
}

 //<-tony 2010-06-14/
 

ALL_SINCE_SAVE_MARKS_CLOSURES(TransNew_SINCE_SAVE_MARKS_DEFN)

#undef TransNew_SINCE_SAVE_MARKS_DEFN

void TransNewGeneration::contribute_scratch(ScratchBlock*& list, Generation* requestor,
                                         size_t max_alloc_words) {
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
 // we don't use to and from now, so forget about it.
 return;
  //<-tony 2010-06-14/
  
  /*
  if (requestor == this || _promotion_failed) return;
  assert(requestor->level() > level(), "TransNewGeneration must be youngest");
	*/
  /* $$$ Assert this?  "trace" is a "MarkSweep" function so that's not appropriate.
  if (to_space->top() > to_space->bottom()) {
    trace("to_space not empty when contribute_scratch called");
  }
  */

/*
  ContiguousSpace* to_space = to();
  assert(to_space->end() >= to_space->top(), "pointers out of order");
  size_t free_words = pointer_delta(to_space->end(), to_space->top());
  if (free_words >= MinFreeScratchWords) {
    ScratchBlock* sb = (ScratchBlock*)to_space->top();
    sb->num_words = free_words;
    sb->next = list;
    list = sb;
  }
  */
}

void TransNewGeneration::reset_scratch() {
  // If contributing scratch in to_space, mangle all of
  // to_space if ZapUnusedHeapArea.  This is needed because
  // top is not maintained while using to-space as scratch.
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  return;
  //<-tony 2010-06-14/
  
 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
  /*
  if (ZapUnusedHeapArea) {
    to()->mangle_unused_area_complete();
  }
  */
   //<-tony 2010-06-28/
   
}

bool TransNewGeneration::collection_attempt_is_safe() {

 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
   /*
  if (!to()->is_empty()) {
    return false;
  }
   */
   
  if (_next_gen == NULL) {
    GenCollectedHeap* gch = GenCollectedHeap::heap();
   //tony 02/06/2010 Fixing Type Based GC, Tony

	//    _next_gen = gch->next_gen(this);
   
     //Added by Feng. Feb 2006
    if (gch->collector_policy()->is_typegen_policy() )
      _next_gen = gch->get_gen(2);
    else
      _next_gen = gch->next_gen(this);
	

   //-tony 02/06/2010
   
    assert(_next_gen != NULL,
           "This must be the youngest gen, and not the only gen");
  }

  // Decide if there's enough room for a full promotion
  // When using extremely large edens, we effectively lose a
  // large amount of old space.  Use the "MaxLiveObjectEvacuationRatio"
  // flag to reduce the minimum evacuation space requirements. If
  // there is not enough space to evacuate eden during a scavenge,
  // the VM will immediately exit with an out of memory error.
  // This flag has not been tested
  // with collectors other than simple mark & sweep.
  //
  // Note that with the addition of promotion failure handling, the
  // VM will not immediately exit but will undo the young generation
  // collection.  The parameter is left here for compatibility.
  const double evacuation_ratio = MaxLiveObjectEvacuationRatio / 100.0;

  // worst_case_evacuation is based on "used()".  For the case where this
  // method is called after a collection, this is still appropriate because
  // the case that needs to be detected is one in which a full collection
  // has been done and has overflowed into the young generation.  In that
  // case a minor collection will fail (the overflow of the full collection
  // means there is no space in the old generation for any promotion).
  size_t worst_case_evacuation = (size_t)(used() * evacuation_ratio);

  return _next_gen->promotion_attempt_is_safe(worst_case_evacuation,
                                              HandlePromotionFailure);

}

void TransNewGeneration::gc_epilogue(bool full) {
  // Check if the heap is approaching full after a collection has
  // been done.  Generally the young generation is empty at
  // a minimum at the end of a collection.  If it is not, then
  // the heap is approaching full.
  GenCollectedHeap* gch = GenCollectedHeap::heap();
  clear_should_allocate_from_space();
  if (collection_attempt_is_safe()) {
    gch->clear_incremental_collection_will_fail();
  } else {
    gch->set_incremental_collection_will_fail();
    if (full) { // we seem to be running out of space
      set_should_allocate_from_space();
    }
  }


  if (ZapUnusedHeapArea) {

   // eden()->check_mangled_unused_area_complete();//by tony 2010-09-26 removing eden space from TransNewGeneration
	 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
	/*
	 from()->check_mangled_unused_area_complete();
    to()->check_mangled_unused_area_complete();
    */
	 //<-tony 2010-06-14/

	  //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
	for(int i=0;i<MAX_TRANSEDEN;i++)
		{
		eden(i)->check_mangled_unused_area_complete();
		}
		 //<-tony 2010-07-04/

	 
  }

  // update the generation and space performance counters
  update_counters();
  gch->collector_policy()->counters()->update_counters();
}

void TransNewGeneration::record_spaces_top() {
  assert(ZapUnusedHeapArea, "Not mangling unused space");
//  eden()->set_top_for_allocations();//by tony 2010-09-26 removing eden space from TransNewGeneration
	  //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
	for(int i=0;i<MAX_TRANSEDEN;i++)
		{
		eden(i)->set_top_for_allocations();
		}
		 //<-tony 2010-07-04/

		 
 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
  /*
  to()->set_top_for_allocations();
  from()->set_top_for_allocations();
  */
   //<-tony 2010-06-14/
   
}


void TransNewGeneration::update_counters() {
  if (UsePerfData) {
//    _eden_counters->update_all();//by tony 2010-09-26 removing eden space from TransNewGeneration
	 //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
	 /*
    _from_counters->update_all();
    _to_counters->update_all();
    */
	 //<-tony 2010-06-14/

	  //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
	for(int i=0;i<MAX_TRANSEDEN;i++)
		{
		_trans_eden_counters[i]->update_all();
		}
		 //<-tony 2010-07-04/
		 
	
    _gen_counters->update_all();
  }
}

void TransNewGeneration::verify(bool allow_dirty) {
//  eden()->verify(allow_dirty);//by tony 2010-09-26 removing eden space from TransNewGeneration
  	  //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
	for(int i=0;i<MAX_TRANSEDEN;i++)
		{
		eden(i)->verify(allow_dirty);
		}
		 //<-tony 2010-07-04/

   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
    /*
  from()->verify(allow_dirty);
    to()->verify(allow_dirty);
    */
    //<-tony 2010-06-14/
     
}

void TransNewGeneration::print_on(outputStream* st) const {
  Generation::print_on(st);
//		st->print("  eden    ");//by tony 2010-09-26 removing eden space from TransNewGeneration
//		  eden()->print_on(st); //by tony 2010-09-26 removing eden space from TransNewGeneration

   //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
   	for(int i=0;i<MAX_TRANSEDEN;i++)
   		{
		//don't bother to print the available transEdens, too many of them usually.
		//by tony 2010/10/29
   		if(getTransEden(i)->state!=TransEden::Available)
			{
			st->print("  T-eden %d",i);
			eden(i)->print_on(st);
			} 
  		}
    //<-tony 2010-07-04/
    
   //tony-> 2010-06-24 Removing From & To,  TransGCDefNew, Tony
    /*
  st->print("  from");
  from()->print_on(st);
  st->print("  to  ");
  to()->print_on(st);
  */
   //<-tony 2010-06-14/
   
}


const char* TransNewGeneration::name() const {
  return "TransNewGeneration";
}

// Moved from inline file as they are not called inline
CompactibleSpace* TransNewGeneration::first_compaction_space() const {
 	assert(MAX_TRANSEDEN>0,"We require that at least one transEden space in our transNewGeneration.");
  return eden(0);//by tony 2010-09-26 removing eden space from TransNewGeneration
}

  int   TransNewGeneration::get_trans_eden_id(void* add){
	  	for(int j=0;j<MAX_TRANSEDEN;j++)
	  		{
	  		if(getTransEden(j)->is_in_reserved(add))
				return j;
	  		}
		return -1;
  	}

 //tony-> 2010-07-26 Allocating into transEden Separately
 //if the current thread is already assigned to a transEden,and the transEden is not full, return that transEden
 //if the current thread is already assigned to a transEden,and the transEden is full,return NULL,then it will automatically allocate into common eden.
 //if the current thread is not a valid transaction thread, return NULL
 //if the current thread is a valid transaction thread, and not assigned yet, assign one transEden if available
 //if no transEden available, ignore this thread as a transaction, reset the transID of the thread, so that this thread will never come to remote young again.
//by tony 2010-07-29


  TransEden* TransNewGeneration::getTransEdenToAllocate_nolock(){	

 //tony-> 2010-09-13
   //check if a transaction father thread has been identified or not.
  if(!Universe::isFoundTCPAcceptThread())return NULL;
 //<-tony 2010-09-13

 
   //as Transaction based allocation, we need to determine which eden to allocate.
  JavaThread* currentThread=NULL;//JavaThread::current();
  Thread* thread=get_thread();
  if(thread->is_Java_thread())
  	currentThread=(JavaThread*)thread;
  else
  	return NULL;
 
  TransEden* transEden=NULL;

  int tid=currentThread->transID();

  if(tid==-1)
		return NULL;//transEden=eden(); //no valid transID, allocate into the normal Eden, eventually, we want this goes to the eden of local young to avoid bloating the remote young gen.//by tony 2010-07-26

 	//check the transEdenID in with the thread
   if(currentThread->transEdenID()!=-1)
   	{
   				transEden=getTransEden(currentThread->transEdenID());
			   	if(tid==transEden->transID)
			   	{
			   			if(transEden->state==TransEden::Assigned)
							return transEden;
						else
							{//full
							//if(DebugTypeGenGC)
								{
							  gclog_or_tty->print_cr("trans %d by thread %d ditched due to state of the TEden Changed..",tid,currentThread->osthread()->thread_id());
							  tty->print_cr("trans %d by thread %d ditched due to state of the TEden Changed.",tid,currentThread->osthread()->thread_id());
				 			}
							//ditch this thread, sadly
							 currentThread->setTransEdenID(-1);
			  		 		 currentThread->setTransID(-1);
							return NULL;
							}
			   	}
				else
				{//they don't match any more, means that this thead may be ditched to local host after the transEden is full or after a full GC
				 //Run Simba, run away, and never return!	
						//if(DebugTypeGenGC)
							{
								  gclog_or_tty->print_cr("trans %d by thread %d ditched due to full GC.",tid,currentThread->osthread()->thread_id());
								  tty->print_cr("trans %d by thread %d ditched due to full GC.",tid,currentThread->osthread()->thread_id());
						 	}						

						 currentThread->setTransEdenID(-1);
		  		 		 currentThread->setTransID(-1);
						 return NULL;
				}
   }
  	//need to search in the current transEden
  	for(int j=0;j<MAX_TRANSEDEN;j++)
		if(getTransEden(j)->transID==tid){
			//got it!
			transEden=getTransEden(j);
			currentThread->setTransEdenID(j);//update the thread information
			break;
			}
	
	if(transEden==NULL)
		{
		//not found, assign one if available
		if(_assignedCount<MAX_TRANSEDEN)
			{
				  	for(int j=0;j<MAX_TRANSEDEN;j++)
						if(getTransEden(j)->state==TransEden::Available){
							getTransEden(j)->state=TransEden::Assigned;
							getTransEden(j)->transID=tid;						
//							_assignedCount++;							
							increaseSharedInt(&_assignedCount);
							decreaseSharedInt(&_availableCount);//_availableCount is decreased when assigned to some transaction

							currentThread->setTransEdenID(j);
							transEden=getTransEden(j);
							if(Universe::isTrainingDone())
								transEden->trained=true;
							//if(DebugTypeGenGC)
								if(NeedTransGCInfo)
								{
								gclog_or_tty->print_cr("trans %d by thread %d goes to eden %d. assignedCount:%d/%d ",tid,currentThread->osthread()->thread_id(),j,_assignedCount,MAX_TRANSEDEN);
								tty->print_cr("trans %d by thread %d goes to eden %d. assignedCount:%d/%d ",tid,currentThread->osthread()->thread_id(),j,_assignedCount,MAX_TRANSEDEN);
								
								if(DebugTypeGenGC&&_assignedCount==MAX_TRANSEDEN){
										gclog_or_tty->print_cr("We are full now, other transactions will go to normal eden");			
									}
								}
							
							break;
							}
			//assert(transEden!=NULL,"since _assignedCount<MAX_TRANSEDEN, should succeed in assigning the transEden.");	//not really, concurrency	//by tony 2010-10-12
			}
		//by tony 2010-10-13 concurrency bug.Whenever we return NULL for the thread, we reset the transId
		
		if(transEden==NULL)
		{
			//we are full for the transaction lots now, should we tell the allocator not to call us again until available?
			currentThread->setTransID(-1);
			//if(DebugTypeGenGC)
//				if(NeedTransGCInfo)
				{
				gclog_or_tty->print_cr("trans %d by thread %d goes to local eden, transEden N/A. assignedCount:%d/%d ",tid,currentThread->osthread()->thread_id(),_assignedCount,MAX_TRANSEDEN);
				tty->print_cr("trans %d by thread %d goes to local eden,transEden N/A. assignedCount:%d/%d ",tid,currentThread->osthread()->thread_id(),_assignedCount,MAX_TRANSEDEN);
				}
				
//			transEden=eden();
			}
		}
	  return transEden;
  	}


  //<-tony 2010-07-26/
  

HeapWord* TransNewGeneration::
	allocate(size_t word_size,bool is_tlab) {
  // This is the slow-path allocation for the DefNewGeneration.
  // Most allocations are fast-path in compiled code.
  // We try to allocate from the eden.  If that works, we are happy.
  // Note that since DefNewGeneration supports lock-free allocation, we
  // have to use it here, as well.

  //as Transaction based allocation, we need to determine which eden to allocate.
  HeapWord* result=NULL;
  GenCollectedHeap *gch = GenCollectedHeap::heap();
  TransEden* transEden=NULL;

/*
 if(!Heap_lock->owned_by_self()&&!TransGC_lock->owned_by_self())
 	{
	  MutexLocker ml(TransGC_lock);
	  transEden=getTransEdenToAllocate();
 	}
 else
 	{
 		transEden=getTransEdenToAllocate();
 	}
 */


	transEden=getTransEdenToAllocate(); 	
	
	
 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
 /*
  if(transEden!=NULL)
  	{
  	 //check if the transEden is full
 	 if( ((TransEden*)transEden)->state==TransEden::Full)
	 	transEden=eden(); //goes to common eden
  	}
  else
  		transEden=eden(); //no assigned a transEden
  		*/
   //<-tony 2010-09-21

  
  assert(transEden!=NULL,"allocate into TransNewGen, but the transEden is NULL, check!");
	 if(transEden==NULL)
  		return NULL;
   //tony-> 2010-10-08 pretenuring for overflowed transEden
				//allocate into transEden
		if(transEden->free()>=word_size*BytesPerWord)
			{
			  result = transEden->par_allocate(word_size);
   
			  if (result != NULL) {
			    return result;
			  }
			  else
			  	{
			  	if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]transEden->par_allocate returns NULL");}
			  	}
			  
					  do {
						    HeapWord* old_limit = transEden->soft_end();

							//if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]:old_limit="INTPTR_FORMAT,old_limit);}
							
						    if (old_limit < transEden->end()) {
						      // Tell the next generation we reached a limit.
						      HeapWord* new_limit =
						        gch->get_gen(2)->allocation_limit_reached(transEden, transEden->top(), word_size);
							  //if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY]:new_limit="INTPTR_FORMAT,new_limit);}
						      if (new_limit != NULL) {
						        Atomic::cmpxchg_ptr(new_limit, transEden->soft_end_addr(), old_limit);
						      } else {
						        assert(transEden->soft_end() == transEden->end(),
						               "invalid state after allocation_limit_reached returned null");
						      }
						    } else {
						      // The allocation failed and the soft limit is equal to the hard limit,
						      // there are no reasons to do an attempt to allocate
						      assert(old_limit == transEden->end(), "sanity check");
						      break;
						    }
						    // Try to allocate until succeeded or the soft limit can't be adjusted
						    result = transEden->par_allocate(word_size);
							
						  } while (result == NULL);

					  if(result!=NULL)
					  		return result;
			
			}

			  //overflowed
			if(!transEden->overflowed)
				transEden->overflowed=true;
		
		//if we are here, the transEden is overflowed,allocate into old
		/*
		  Generation* old=gch->get_gen(2);

		   if (old->should_allocate(word_size,is_tlab)) 
	    		result= old->allocate(word_size,  is_tlab);
		*/
			return result;

    //<-tony 2010-10-08
    
 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
/*
	//we are running out of space for the selected transEden, try common Eden
  if(transEden!=eden()){
  	//set full to transEden.
  	((TransEden*)transEden)->state=TransEden::Full;
  	transEden=eden();
  	result = transEden->par_allocate(word_size);
   
    return result;

  }
  */
   //<-tony 2010-09-21
   
	
   //tony-> 2010-08-03 Collecting transEdens
   /*
  do {
    HeapWord* old_limit = transEden->soft_end();
    if (old_limit < transEden->end()) {
      // Tell the next generation we reached a limit.
      HeapWord* new_limit =
        next_gen()->allocation_limit_reached(transEden, transEden->top(), word_size);
      if (new_limit != NULL) {
        Atomic::cmpxchg_ptr(new_limit, transEden->soft_end_addr(), old_limit);
      } else {
        assert(transEden->soft_end() == transEden->end(),
               "invalid state after allocation_limit_reached returned null");
      }
    } else {
      // The allocation failed and the soft limit is equal to the hard limit,
      // there are no reasons to do an attempt to allocate
      assert(old_limit == transEden->end(), "sanity check");
      break;
    }
    // Try to allocate until succeeded or the soft limit can't be adjusted
    result = transEden->par_allocate(word_size);
  } while (result == NULL);
*/

  // If the eden is full and the last collection bailed out, we are running
  // out of heap space, and we try to allocate the from-space, too.
  // allocate_from_space can't be inlined because that would introduce a
  // circular dependency at compile time.
 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
/*
if (result == NULL) {
    result = allocate_from_space(word_size);
  }
   //<-tony 2010-06-28/
*/
 //<-tony 2010-06-28/
 
}

HeapWord* TransNewGeneration::par_allocate(size_t word_size,
                                         bool is_tlab) {

TransEden* transEden=NULL;
/*
 if(!Heap_lock->owned_by_self()&&!TransGC_lock->owned_by_self())
 	{
	  MutexLocker ml(TransGC_lock);
	  transEden=getTransEdenToAllocate();
 	}
 else
 	{
 		transEden=getTransEdenToAllocate();
 	}
 */
 	
	transEden=getTransEdenToAllocate(); 	
	

assert(transEden!=NULL,"allocate into TransNewGen, but the transEden is NULL, check!");

HeapWord* result=NULL;
if(transEden==NULL)
  		return NULL;
	 
/*tony 02/06/2010 Fixing Type Based GC, Tony*/ 

//return eden()->par_allocate(word_size);
/*
  if(transEden!=NULL)
  	{
  	 //check if the transEden is full
 	 if( ((TransEden*)transEden)->state==TransEden::Full)
	 	transEden=eden(); //goes to common eden
  	}
  else
  		transEden=eden(); //no assigned a transEden
*/
   //tony-> 2010-10-08 pretenuring for overflowed transEden
		if(transEden->free()>=word_size*BytesPerWord)
			{
			//allocate into transEden
				result=transEden->par_allocate(word_size);
				return result;
			}
		else
			{
			//overflowed
			if(!transEden->overflowed)
				transEden->overflowed=true;
			}
/*//by tony 2010-10-11 pretenuring for overflowed transEden, we don't do pretenuring in par_allocate, intead, we do it in allocate()
	//if we are here, the transEden is overflowed,allocate into old
	  GenCollectedHeap *gch = GenCollectedHeap::heap();
	  Generation* old=gch->next_gen(this);
	   if (old->should_allocate(word_size,is_tlab)) 
    		return old->allocate(word_size,  is_tlab);
	   else
		   	return NULL;
	
*/

	
    //<-tony 2010-10-08
    return NULL;

/*-tony 02/06/2010 */

}

void TransNewGeneration::gc_prologue(bool full) {
  // Ensure that _end and _soft_end are the same in eden space.
//  eden()->set_soft_end(eden()->end());//by tony 2010-09-26 removing eden space from TransNewGeneration

    //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
   	for(int i=0;i<MAX_TRANSEDEN;i++)
   		{
   		eden(i)->set_soft_end(eden(i)->end());
   		}
    //<-tony 2010-07-04/
    
}

size_t TransNewGeneration::tlab_capacity() const {
 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
//  return eden()->capacity();
 
  return capacity();
   //<-tony 2010-07-04/
   
}

size_t TransNewGeneration::unsafe_max_tlab_alloc() const {
  return unsafe_max_alloc_nogc();
}
/*
reset any resettable transEdens, return true if any.
resettable condition: 1.Collectable 2. Trained.
//tony-> 2011/02/18 Trying to not stop the world when a trained transaction is over.
*/
bool TransNewGeneration::collectResettableTransEden()
{
			bool didSomething=false;
		  if(!Universe::isTrainingDone()||!Pretenure||!AVOID_COLLECT_TRAINED) //training is not done yet, so return doing nothing.
		  	return false;
		  
		  for(int j=0;j<MAX_TRANSEDEN;j++){
		  	if(getTransEden(j)->state==TransEden::Collectable&&getTransEden(j)->trained)
				{
					decreaseSharedInt(&_collectableCount);

					reset_transEden(j);
					
					getTransEden(j)->clear(SpaceDecorator::Mangle); //by tony 2010-09-16 clearing the top.					

					 //tony-> 2011/02/21 re-examine the synchronization implementation.

					  //<-tony 2011-02-21
					didSomething=true;	
				if(NeedTransGCInfo){
				  	tty->print_cr("[TransGC]Reset TransEden-%d Avail:%d Assi:%d,Coll:%d",j,_availableCount,_assignedCount,_collectableCount);		  
				  	gclog_or_tty->print_cr("[TransGC]Reset TransEden-%d Avail:%d Assi:%d,Coll:%d",j,_availableCount,_assignedCount,_collectableCount);
					}
  
		  		}
		  	}
			return didSomething;
			
}
 //tony-> 2010-08-03 Collecting transEdens
void TransNewGeneration::collectTransEden(int id){
		if(getTransEden(id)->state!=TransEden::Collectable)
			return;

  GenCollectedHeap* gch = GenCollectedHeap::heap();
  size_t gch_prev_used = gch->used();
 //tony-> 2011/02/21 re-examine the synchronization implementation.
	 decreaseSharedInt(&_collectableCount);
  //<-tony 2011-02-21


  if(getTransEden(id)->trained&&AVOID_COLLECT_TRAINED) //shortcut, we just erase the transEden, see what's gonna happen.
  	{
			reset_transEden(id);
			getTransEden(id)->clear(SpaceDecorator::Mangle); //by tony 2010-09-16 clearing the top.

			if(NeedTransGCInfo){
		  	tty->print_cr("[TransGC]Collected by resetting: Avail:%d Assi:%d,Coll:%d",id,_availableCount,_assignedCount,_collectableCount);		  
		  	gclog_or_tty->print_cr("[TransGC]Collected by resetting: Avail:%d Assi:%d,Coll:%d",id,_availableCount,_assignedCount,_collectableCount);
			}

		   if (PrintGC && !PrintGCDetails) {
		      gch->print_heap_change(gch_prev_used);
		    }
		   
		return;
  }
  



  gch->save_marks();//by tony 2010-08-12
  
  _next_gen = gch->next_gen(this);

   
  assert(_next_gen != NULL,
    "This must be the youngest gen, and not the only gen");

  // If the next generation is too full to accomodate promotion
  // from this generation, pass on collection; let the next generation
  // do it.
  if (!collection_attempt_is_safe()) {
    gch->set_incremental_collection_will_fail();
    return;
  }

// Capture heap used before collection (for printing).

  size_t old_prev_used = _next_gen->used();//by tony 2010-09-27 investigating the residual of the transactions
  size_t trans_eden_prev_used=getTransEden(id)->used();
  _next_gen->save_top_for_examine();
//	if(DebugTypeGenGC) gclog_or_tty->print("Transaction %d:top="PTR_FORMAT ",bottom="PTR_FORMAT " used="SIZE_FORMAT,getTransEden(id)->transID, getTransEden(id)->top(), getTransEden(id)->bottom(), getTransEden(id)->used());                        


   if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:set MinorGCGoingOn=true]");MinorGCGoingOn=true;}


   getTransEden(id)->setState(TransEden::Collecting); //set the state of the transEden, //by tony 2010-08-31
   //so that we can exclude it self when iterating spaces inside the transNewGeneration.

 if(ResidualDemographics)
   getTransEden(id)->checkDemographicsBeforeGC();//by tony 2011/01/21 
   
  init_assuming_no_promotion_failure();

  TraceTime t1("GC-TransNew", PrintGC && !PrintGCDetails, true, gclog_or_tty);

  SpecializationStats::clear();

  // These can be shared for all code paths
 //tony-> 2010-08-26 Fixing transEdens collector
 
  IsAliveClosure is_alive(getTransEden(id)); //give a transEden area
  //	IsAliveClosure is_alive(this);
   //<-tony 2010-08-26
  ScanWeakRefClosure scan_weak_ref(getTransEden(id),this);

  	
  age_table()->clear();
 
  gch->rem_set()->prepare_for_younger_refs_iterate(false);


//  assert(gch->no_allocs_since_save_marks(1), "save marks have not been newly set.");

  // Not very pretty.
  CollectorPolicy* cp = gch->collector_policy();

 
  FastScanClosure fsc_with_no_gc_barrier(this, false);
  FastScanClosure fsc_with_gc_barrier(this, true);

  fsc_with_no_gc_barrier.set_boundaries(getTransEden(id)->bottom(),getTransEden(id)->end());//refine it, maybe top?

  fsc_with_gc_barrier.set_boundaries(getTransEden(id)->bottom(),getTransEden(id)->end());//refine it to be within the selected eden space
  
  set_promo_failure_scan_stack_closure(&fsc_with_no_gc_barrier);

  FastEvacuateFollowersClosure evacuate_followers(gch, _level, this,
                                                  &fsc_with_no_gc_barrier,
                                                  &fsc_with_gc_barrier);


  assert(gch->no_allocs_since_save_marks(0),
         "save marks have not been newly set.");

  gch->gen_process_strong_roots(_level,
                                true, // Process younger gens, if any, as
                                      // strong roots.
                                false,// not collecting permanent generation.
                                SharedHeap::SO_AllClasses,   
                                &fsc_with_gc_barrier,
                                &fsc_with_no_gc_barrier);

/*
if(DebugTypeGenGC)
{
  gclog_or_tty->print_cr("gc transNew 3, gen_process_strong_roots done");
  gch->rem_set()->verify();
}
*/

  // "evacuate followers".
  evacuate_followers.do_void();

/*
if(DebugTypeGenGC)
{
  gclog_or_tty->print_cr("gc transNew 4, evacuate_followers.do_void() done, verifying rem_set");
  gch->rem_set()->verify();
}
*/

  FastKeepAliveClosure keep_alive(this, &scan_weak_ref);
  ReferenceProcessor* rp = ref_processor();

//  rp->setup_policy(clear_all_soft_refs);
  rp->setup_policy(true); //by tony 2010-08-12


  
  rp->process_discovered_references(&is_alive, &keep_alive, &evacuate_followers,
                                    NULL);

  if (!promotion_failed()) {

 //tony-> 2010-12-05 printing out the allocation site of the residuals.	
  JvmtiTagMap::gc_epilogue(true);
   //<-tony 2010-12-05
   
	size_t residual=_next_gen->used()-old_prev_used;
     TransEden* transEden=getTransEden(id);
	 gclog_or_tty->print_cr("[Transaction %d residuals:"SIZE_FORMAT"=" SIZE_FORMAT "K"
                        "=%3d%% of "SIZE_FORMAT "K, threadCount=%d], trained=%d, overflowed=%d",getTransEden(id)->transID, residual, residual/K,  (int)((double)residual*100.0/trans_eden_prev_used),trans_eden_prev_used/K,getTransEden(id)->threadCount,getTransEden(id)->trained,getTransEden(id)->overflowed );

	  //tony-> 2011/01/16 check the residuals' demographics.
	 if(ResidualDemographics)
	 	{
		     gclog_or_tty->print("Demographics BeforeGC:");
			 for(int i=0;i<9;i++)
			  	gclog_or_tty->print("%d,",transEden->demographicsBeforeGC[i]);
			 gclog_or_tty->print_cr("%d",transEden->demographicsBeforeGC[9]);
			 
		     gclog_or_tty->print("Demographics @GC-%d:",gch->total_collections());
			 for(int i=0;i<9;i++)
			  	gclog_or_tty->print("%d,",transEden->demographics[i]);
			 gclog_or_tty->print_cr("%d",transEden->demographics[9]);
	 	}
	  //<-tony 2011-1-16

//tony-> 2010-11-04 analysing the residuals
 //we need to scan the incremented part of the  tenured generation for the type of the objects.
//	_next_gen->examine_type_of_objects(); //FIXME: just try to see if it works.

	if(ExamineResiduals&&!Universe::isTrainingDone())
	{
		trainingResidual();
	}

	gclog_or_tty->flush();//by tony 2011/01/21
	
	if(ExamineResiduals&&getTransEden(id)->trained&&STILL_CURIOUS) // in case that we are still curious about what's the leftover for a trained transGC.
		_next_gen->examine_type_of_objects();
 //<-tony 2010-11-04
  	
   if (PrintGC && !PrintGCDetails) {

      gch->print_heap_change(gch_prev_used);
    }
    
    //getTransEden(id)->clear(SpaceDecorator::Mangle);
	reset_transEden(id);
	getTransEden(id)->clear(SpaceDecorator::Mangle); //by tony 2010-09-16 clearing the top.
	
  } else {
    // deallocate stack and it's elements
    delete _promo_failure_scan_stack;
    _promo_failure_scan_stack = NULL;

    remove_forwarding_pointers();
    if (PrintGCDetails) {
      gclog_or_tty->print(" (promotion failed)");
    }
    gch->set_incremental_collection_will_fail();

 //tony-> 2011/02/21 re-examine the synchronization implementation.
	 increaseSharedInt(&_collectableCount); //we decreased the collectableCount at the beginning, now we shoudl 
	 //increase it since we are not done with this collection.
  //<-tony 2011-02-21
  

    // Reset the PromotionFailureALot counters.
    NOT_PRODUCT(Universe::heap()->reset_promotion_should_fail();)
  }
  SpecializationStats::print();
  update_time_of_last_gc(os::javaTimeMillis());
 if(DebugTypeGenGC){gclog_or_tty->print_cr("[TONY:set MinorGCGoingOn=false]");MinorGCGoingOn=false;}
	 
 }


   //tony-> 2010-11-04 analysing the residuals
  void TransNewGeneration::trainingResidual() //this is used when a transaction is over, we scan the survived objects, and tell what class will be surviving.
   	{
		static int residual_unchanged_time=0;
		_gcCount++;
		// we are trying to skip the first few transEdens to avoid flooding the kclass trained.
		if(_gcCount<=TRAINING_AFTER_GC)
				return;
		
		if(residual_unchanged_time<UNCHANGED_TIME_FOR_TRAINING_END)
		{
			 		int pre_total=Universe::total_residual_count();
					_next_gen->examine_type_of_objects();
					if(Universe::total_residual_count()==pre_total)
						residual_unchanged_time++;
					else
						residual_unchanged_time=0;
		}
		else
		{
				//trainning finished!
			Universe::setTrainingDone();
			//if(DebugTypeGenGC)
				{
				gclog_or_tty->print_cr("[residuals]training done!");tty->print_cr("[residuals]training done!");
				}
		}

		return;
	
   	}
    //<-tony 2010-11-04



 //tony-> 2010-08-26 Fixing transEdens collector	
ScanWeakRefClosure::ScanWeakRefClosure(Space* sp,Generation* g) :
  OopClosure(g->ref_processor()), _g(g)

{
  assert(TransGC && g->level()==1 , "Optimized for youngest generation");
  _boundary = _g->reserved().end();

  _upper_boundary=sp->end();
  _lower_boundary=sp->bottom();
}


//<-tony 2010-08-26  
