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
//#define MAX_TRANSEDEN 10
class EdenSpace;
class ContiguousSpace;
class ScanClosure;
 extern Mutex* trans_lock; //by tony 2010-10-27 concurrency bug.
 
  
// TransNewGeneration is a young generation containing a serials of Transa

/*tony 2010-06-14 TransGCDefNew, Tony*/ 
//TransNewGeneration is a similar one, but we will not have from and to space, and the collection
//operation will be different in that we will wipe the sub spaces per transaction.
/*-tony 2010-06-14 */
	
class TransNewGeneration: public Generation {
  friend class VMStructs;
  friend class GenCollectedHeap;
  friend class TypeGenCollectorPolicy;
protected:
  int         _tenuring_threshold;   // Tenuring threshold for next collection.
  ageTable    _age_table;
  Generation* _next_gen;
  // Size of object to pretenure in words; command line provides bytes
  size_t        _pretenure_size_threshold_words;

  ageTable*   age_table() { return &_age_table; }
  // Initialize state to optimistically assume no promotion failure will
  // happen.
  void   init_assuming_no_promotion_failure();
  // True iff a promotion has failed in the current collection.
  bool   _promotion_failed;
  bool   promotion_failed() { return _promotion_failed; }

  // Handling promotion failure.  A young generation collection
  // can fail if a live object cannot be copied out of its
  // location in eden or from-space during the collection.  If
  // a collection fails, the young generation is left in a
  // consistent state such that it can be collected by a
  // full collection.
  //   Before the collection
  //     Objects are in eden or from-space
  //     All roots into the young generation point into eden or from-space.
  //
  //   After a failed collection
  //     Objects may be in eden, from-space, or to-space
  //     An object A in eden or from-space may have a copy B
  //       in to-space.  If B exists, all roots that once pointed
  //       to A must now point to B.
  //     All objects in the young generation are unmarked.
  //     Eden, from-space, and to-space will all be collected by
  //       the full collection.
  void handle_promotion_failure(oop);

  // In the absence of promotion failure, we wouldn't look at "from-space"
  // objects after a young-gen collection.  When promotion fails, however,
  // the subsequent full collection will look at from-space objects:
  // therefore we must remove their forwarding pointers.
  void remove_forwarding_pointers();

  // Preserve the mark of "obj", if necessary, in preparation for its mark
  // word being overwritten with a self-forwarding-pointer.
  void   preserve_mark_if_necessary(oop obj, markOop m);

  // When one is non-null, so is the other.  Together, they each pair is
  // an object with a preserved mark, and its mark value.
  GrowableArray<oop>*     _objs_with_preserved_marks;
  GrowableArray<markOop>* _preserved_marks_of_objs;

  // Returns true if the collection can be safely attempted.
  // If this method returns false, a collection is not
  // guaranteed to fail but the system may not be able
  // to recover from the failure.
  bool collection_attempt_is_safe();

  // Promotion failure handling
  OopClosure *_promo_failure_scan_stack_closure;
  void set_promo_failure_scan_stack_closure(OopClosure *scan_stack_closure) {
    _promo_failure_scan_stack_closure = scan_stack_closure;
  }

  GrowableArray<oop>* _promo_failure_scan_stack;
  GrowableArray<oop>* promo_failure_scan_stack() const {
    return _promo_failure_scan_stack;
  }
  void push_on_promo_failure_scan_stack(oop);
  void drain_promo_failure_scan_stack(void);
  bool _promo_failure_drain_in_progress;

  // Performance Counters
  GenerationCounters*  _gen_counters;
//  CSpaceCounters*      _eden_counters;
  CSpaceCounters** _trans_eden_counters;

 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
  //CSpaceCounters*      _from_counters;
  //CSpaceCounters*      _to_counters;
  //<-tony 2010-06-28/
   

  // sizing information
  size_t               _max_eden_size;
  size_t               _max_survivor_size;

  // Allocation support
  bool _should_allocate_from_space;
  bool should_allocate_from_space() const {
    return _should_allocate_from_space;
  }
  void clear_should_allocate_from_space() {
    _should_allocate_from_space = false;
  }
  void set_should_allocate_from_space() {
    _should_allocate_from_space = true;
  }

 protected:
  // Spaces
//  EdenSpace*       _eden_space; 
 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
//  TransEden* _transEdens[MAX_TRANSEDEN];	
  TransEden** _transEdens;//transEdens is an array of TransEden*
  int _assignedCount;
  int _availableCount;
  int _collectableCount;
  int _gcCount;
  //<-tony 2010-07-04/
 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
//  ContiguousSpace* _from_space;
//  ContiguousSpace* _to_space;
   //<-tony 2010-06-28/
  TransEden* getTransEdenToAllocate()
  {
    TransEden* transEden=NULL;
   	trans_lock->lock_without_safepoint_check();
		transEden=getTransEdenToAllocate_nolock(); 	
	trans_lock->unlock();
	return transEden;
	
  }; //according to current thread, selece which transEden to allocate from
  TransEden* getTransEdenToAllocate_nolock();
  int getCollectableTransEdenCount()
  	{
	  	return _collectableCount;
  	}

   //tony-> 2010-11-04 analysing the residuals

  void trainingResidual(); //this is used when a transaction is over, we scan the survived objects, and tell what class will be surviving.
    //<-tony 2010-11-04

 //tony-> 2010-09-21 only when available transEden goes down to 0, and collectable is at least 1, do we collect transNewGen
  int getAvailableTransEdenCount()
  	{
  			return _availableCount;
  	}

	void decreaseSharedInt(int *p_theCount)
		{
			changeSharedIntByOne(p_theCount,false);
		}
	void increaseSharedInt(int *p_theCount)
		{
			changeSharedIntByOne(p_theCount,true);
		}
	/*change the shared int pointed by p_theCount by one, direction is indicated by b_increase
	Atomic::cmpxchg is used.
	*/
	void changeSharedIntByOne(int * p_theCount, bool b_increase){
		int count,new_count,result;
		do{
			count=*p_theCount;
			if(b_increase)
				new_count=count+1;
			else				
				new_count=count-1;
			result=(int)Atomic::cmpxchg(new_count,p_theCount,count);
			if(result==count) //succeeded!
				return;	
		}while(true);

	}

//<-tony 2010-09-21
//reset the designated transEden
  void reset_transEden(int id){
  	if(id<MAX_TRANSEDEN &&id>-1){
		getTransEden(id)->reset();
		//_assignedCount--;	
		decreaseSharedInt(&_assignedCount);
		 //tony-> 2011/02/21 re-examine the synchronization implementation.
		increaseSharedInt(&_availableCount);//added _availableCount for easy counting.
		 //<-tony 2011-02-21
		}
  	}
  void reset_transEdens()
  	{
  		for(int i=0;i<MAX_TRANSEDEN;i++)
				getTransEden(i)->reset();		
		_assignedCount=0;
		_collectableCount=0;
		_availableCount=MAX_TRANSEDEN;
  	}
	/*
	reset any resettable transEdens, return true if any.
	resettable condition: 1.Collectable 2. Trained.
	//tony-> 2011/02/18 Trying to not stop the world when a trained transaction is over.
	*/
	bool collectResettableTransEden();

  	void collectTransEden(int id);
	
	void collectTransEdens(){
		for(int i=0;i<MAX_TRANSEDEN;i++)
				collectTransEden(i);		

	}




  //just set the transEden as collectable
/*
We need to verify that the ending transaction is still holding a transEden, because after a full GC, all transaction data will be remove,
but the threads already in transaction are not updated.
So when these threads park, the transaction is really ending, 
but we cannot collect those transEdens which the threads used to allocate into.
To solve this problem, we need to verify the transID of the thread and the transEden
before label the transEden to be collectable.
*/


  void set_transEden_collectable(int id, int transID,int threadcount)
  	{
	if(id<MAX_TRANSEDEN &&id>-1){
		TransEden* teden=getTransEden(id);
		
		if(teden->transID==transID){
			teden->setState(TransEden::Collectable);
			increaseSharedInt(&_collectableCount);
			teden->threadCount=threadcount;
			assert(_collectableCount<=MAX_TRANSEDEN,"collectable Count should be no bigger than MAX_TRANSEDEN.");
			}
		}
  	}
  enum SomeProtectedConstants {
    // Generations are GenGrain-aligned and have size that are multiples of
    // GenGrain.
    MinFreeScratchWords = 100
  };

  // Return the size of a survivor space if this generation were of size
  // gen_size.
  size_t compute_survivor_size(size_t gen_size, size_t alignment) const {
    size_t n = gen_size / (SurvivorRatio + 2);
    return n > alignment ? align_size_down(n, alignment) : alignment;
  }

 public:  // was "protected" but caused compile error on win32
  class IsAliveClosure: public BoolObjectClosure {
    Generation* _g;
	EdenSpace* _ed; //by tony 2010-08-12
  public:
    IsAliveClosure(Generation* g);
	IsAliveClosure(EdenSpace* ed);//by tony 2010-08-12
    void do_object(oop p);
    bool do_object_b(oop p);
  };

  class KeepAliveClosure: public OopClosure {
  protected:
    ScanWeakRefClosure* _cl;
    CardTableRS* _rs;
    template <class T> void do_oop_work(T* p);
  public:
    KeepAliveClosure(ScanWeakRefClosure* cl);
    virtual void do_oop(oop* p);
    virtual void do_oop(narrowOop* p);
  };

  class FastKeepAliveClosure: public KeepAliveClosure {
  protected:
    HeapWord* _boundary;
    template <class T> void do_oop_work(T* p);
		 
  public:
    FastKeepAliveClosure(TransNewGeneration* g, ScanWeakRefClosure* cl);
    virtual void do_oop(oop* p);
    virtual void do_oop(narrowOop* p);
  };

  class EvacuateFollowersClosure: public VoidClosure {
    GenCollectedHeap* _gch;
    int _level;
    ScanClosure* _scan_cur_or_nonheap;
    ScanClosure* _scan_older;
  public:
    EvacuateFollowersClosure(GenCollectedHeap* gch, int level,
                             ScanClosure* cur, ScanClosure* older);
    void do_void();
  };

  class FastEvacuateFollowersClosure;
  friend class FastEvacuateFollowersClosure;
  class FastEvacuateFollowersClosure: public VoidClosure {
    GenCollectedHeap* _gch;
    int _level;
    TransNewGeneration* _gen;
    FastScanClosure* _scan_cur_or_nonheap;
    FastScanClosure* _scan_older;
  public:
    FastEvacuateFollowersClosure(GenCollectedHeap* gch, int level,
                                 TransNewGeneration* gen,
                                 FastScanClosure* cur,
                                 FastScanClosure* older);
    void do_void();
  };

 public:
    TransNewGeneration(ReservedSpace rs, size_t initial_byte_size, int level,
                   const char* policy="Copy");

  virtual Generation::Name kind() { return Generation::TransNew; }


  // Accessing spaces

 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
//  EdenSpace*       eden() const           { return _eden_space; }//by tony 2010-09-26
 //<-tony 2010-09-21

 //tony-> 2010-07-04 Adding TransEden,  TransGCDefNew, Tony
  EdenSpace*       eden(int id) const           { 
  //  assert(id<MAX_TRANSEDEN &&id>-1,"invalid transEden id!"); 
  //FIXME: there is a bug out of concurrency, so this assertion removed temporarily. 2010-7-22 //by tony

  	if(id<MAX_TRANSEDEN &&id>-1)
				return _transEdens[id]; 
	
   //tony-> 2010-09-26 Removing eden space from TransNewGeneration
	else
		{
		  if(DebugTypeGenGC){
			  gclog_or_tty->print_cr("invalid transEden id(%i), returning NULL",id);	  
	  		}
		  		return NULL; 
		  
		}

	 //<-tony 2010-09-21

 	}
   TransEden*       getTransEden(int id) const           { 
  //  assert(id<MAX_TRANSEDEN &&id>-1,"invalid transEden id!"); 
  //FIXME: there is a bug out of concurrency, so this assertion removed temporarily. 2010-7-22 //by tony

  	if(id<MAX_TRANSEDEN &&id>-1)
				return _transEdens[id]; 
	 //tony-> 2010-09-26 Removing eden space from TransNewGeneration
	else
		{
		  if(DebugTypeGenGC){
			  gclog_or_tty->print_cr("invalid transEden id(%i), returning null.",id);	  
	  		}
		  		return NULL; 
		  
		}

 	}
 //<-tony 2010-07-04/
   

 //tony-> 2010-06-28 Removing From & To,  TransGCDefNew, Tony
  //ContiguousSpace* from() const           { return _from_space;  }
  //ContiguousSpace* to()   const           { return _to_space;    }
  //<-tony 2010-06-28/
  
  virtual CompactibleSpace* first_compaction_space() const;

  // Space enquiries
  size_t capacity() const;
  size_t used() const;
  size_t free() const;
  size_t max_capacity() const;
  size_t capacity_before_gc() const;
  size_t unsafe_max_alloc_nogc() const;
  size_t contiguous_available() const;

  size_t max_eden_size() const              { return _max_eden_size; }
  size_t max_survivor_size() const          { return _max_survivor_size; }

  bool supports_inline_contig_alloc() const { return true; }
  HeapWord** top_addr() const;
  HeapWord** end_addr() const;

  // Thread-local allocation buffers
  bool supports_tlab_allocation() const { return true; }
  size_t tlab_capacity() const;
  size_t unsafe_max_tlab_alloc() const;

  // Grow the generation by the specified number of bytes.
  // The size of bytes is assumed to be properly aligned.
  // Return true if the expansion was successful.
  bool expand(size_t bytes);

  // TransNewGeneration cannot currently expand except at
  // a GC.
  virtual bool is_maximal_no_gc() const { return true; }

  // Iteration
  void object_iterate(ObjectClosure* blk);
  void object_iterate_since_last_GC(ObjectClosure* cl);


 //tony-> 2010-08-30 Fixing transEdens collector
 //we should not iterate in the same TransEden while collecting it.
  void object_iterate_with_self_excluded(ObjectClosure* blk, int transEdenID);
  void object_iterate_since_last_GC_with_self_excluded(ObjectClosure* cl,int transEdenID);
  //<-tony 2010-08-30

  void younger_refs_iterate(OopsInGenClosure* cl);

  void space_iterate(SpaceClosure* blk, bool usedOnly = false);

  virtual  void space_iterate_with_some_excluded(SpaceClosure* blk, bool usedOnly = false);


  // Allocation support

  //tony-> 2010-08-03 Collecting transEdens
  /*
   virtual bool should_allocate(size_t word_size, bool is_tlab) {
    assert(UseTLAB || !is_tlab, "Should not allocate tlab");

    size_t overflow_limit    = (size_t)1 << (BitsPerSize_t - LogHeapWordSize);

    const bool non_zero      = word_size > 0;
    const bool overflows     = word_size >= overflow_limit;
    const bool check_too_big = _pretenure_size_threshold_words > 0;
    const bool not_too_big   = word_size < _pretenure_size_threshold_words;
    const bool size_ok       = is_tlab || !check_too_big || not_too_big;

    bool result = !overflows &&
                  non_zero   &&
                  size_ok;

    return result;
  }
  */
  //here, we judge if there is enough space for new allocation in the trnasNewGeneration
  //
  virtual bool should_allocate(size_t word_size, bool is_tlab) {
    assert(UseTLAB || !is_tlab, "Should not allocate tlab");

 //tony-> 2010-09-13
   //check if a transaction father thread has been identified or not.
  if(!Universe::isFoundTCPAcceptThread())return false;
 //<-tony 2010-09-13	

 
    //which transEden?
    TransEden* teden=NULL;
/*
	  if(!Heap_lock->owned_by_self()&&!TransGC_lock->owned_by_self())
	 	{
		  MutexLocker ml(TransGC_lock);
		  teden=getTransEdenToAllocate();
	 	}
	 else
	 	{
	 		teden=getTransEdenToAllocate();
	 	}
 
 */	
	teden=getTransEdenToAllocate(); 		
	return(teden!=NULL);

	/*we postpone this check to the allocation part, because we want the transNewGen to promote it directly to old gen .
	//by tony 2010-10-08 pretenuring for overflowed transEden
	//see if the eden has enough space? 
	if(teden->free()>=word_size*BytesPerWord)
		return true;
	else
		return false;
		*/

  }

 //<-tony 2010-08-03/

   //tony-> 2010-08-19 Collecting transEdens
  int   get_trans_eden_id(void* add);
    //<-tony 2010-08-19/
    
  HeapWord* allocate(size_t word_size, bool is_tlab);
  HeapWord* allocate_from_space(size_t word_size);

  HeapWord* par_allocate(size_t word_size, bool is_tlab);

  // Prologue & Epilogue
  virtual void gc_prologue(bool full);
  virtual void gc_epilogue(bool full);

  // Save the tops for eden, from, and to
  virtual void record_spaces_top();

  // Doesn't require additional work during GC prologue and epilogue
  virtual bool performs_in_place_marking() const { return false; }

  // Accessing marks
  void save_marks();
  void reset_saved_marks();
  bool no_allocs_since_save_marks();

  // Need to declare the full complement of closures, whether we'll
  // override them or not, or get message from the compiler:
  //   oop_since_save_marks_iterate_nv hides virtual function...
#define TransNew_SINCE_SAVE_MARKS_DECL(OopClosureType, nv_suffix) \
  void oop_since_save_marks_iterate##nv_suffix(OopClosureType* cl);

  ALL_SINCE_SAVE_MARKS_CLOSURES(TransNew_SINCE_SAVE_MARKS_DECL)

#undef TransNew_SINCE_SAVE_MARKS_DECL

  // For non-youngest collection, the TransNewGeneration can contribute
  // "to-space".
  virtual void contribute_scratch(ScratchBlock*& list, Generation* requestor,
                          size_t max_alloc_words);

  // Reset for contribution of "to-space".
  virtual void reset_scratch();

  // GC support
  virtual void compute_new_size();
  virtual void collect(bool   full,
                       bool   clear_all_soft_refs,
                       size_t size,
                       bool   is_tlab);
  HeapWord* expand_and_allocate(size_t size,
                                bool is_tlab,
                                bool parallel = false);

  oop copy_to_survivor_space(oop old);
  int tenuring_threshold() { return _tenuring_threshold; }

  // Performance Counter support
  void update_counters();

  // Printing
  virtual const char* name() const;
  virtual const char* short_name() const { return "TransNew"; }

  bool must_be_youngest() const { return false; } ////by tony 2010-08-12
  bool must_be_oldest() const { return false; }

  // PrintHeapAtGC support.
  void print_on(outputStream* st) const;

  void verify(bool allow_dirty);

 protected:
  // If clear_space is true, clear the survivor spaces.  Eden is
  // cleared if the minimum size of eden is 0.  If mangle_space
  // is true, also mangle the space in debug mode.
  void compute_space_boundaries(uintx minimum_eden_size,
                                bool clear_space,
                                bool mangle_space);
  // Scavenge support
  void swap_spaces();
};
