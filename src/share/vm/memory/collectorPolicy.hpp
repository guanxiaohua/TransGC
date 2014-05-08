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

// This class (or more correctly, subtypes of this class)
// are used to define global garbage collector attributes.
// This includes initialization of generations and any other
// shared resources they may need.
//
// In general, all flag adjustment and validation should be
// done in initialize_flags(), which is called prior to
// initialize_size_info().
//
// This class is not fully developed yet. As more collector(s)
// are added, it is expected that we will come across further
// behavior that requires global attention. The correct place
// to deal with those issues is this class.

// Forward declarations.
class GenCollectorPolicy;
class TwoGenerationCollectorPolicy;
class AdaptiveSizePolicy;
#ifndef SERIALGC
class ConcurrentMarkSweepPolicy;
class G1CollectorPolicy;
#endif // SERIALGC

/*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
class TypeGenCollectorPolicy;
/*-tony 02/06/2010 */

class GCPolicyCounters;
class PermanentGenerationSpec;
class MarkSweepPolicy;

class CollectorPolicy : public CHeapObj {
 protected:
  PermanentGenerationSpec *_permanent_generation;
  GCPolicyCounters* _gc_policy_counters;

  // Requires that the concrete subclass sets the alignment constraints
  // before calling.
  virtual void initialize_flags();
  virtual void initialize_size_info();
  // Initialize "_permanent_generation" to a spec for the given kind of
  // Perm Gen.
  void initialize_perm_generation(PermGen::Name pgnm);

  size_t _initial_heap_byte_size;
  size_t _max_heap_byte_size;
  size_t _min_heap_byte_size;

  size_t _min_alignment;
  size_t _max_alignment;

  CollectorPolicy() :
    _min_alignment(1),
    _max_alignment(1),
    _initial_heap_byte_size(0),
    _max_heap_byte_size(0),
    _min_heap_byte_size(0)
  {}

 public:
  void set_min_alignment(size_t align)         { _min_alignment = align; }
  size_t min_alignment()                       { return _min_alignment; }
  void set_max_alignment(size_t align)         { _max_alignment = align; }
  size_t max_alignment()                       { return _max_alignment; }

  size_t initial_heap_byte_size() { return _initial_heap_byte_size; }
  void set_initial_heap_byte_size(size_t v) { _initial_heap_byte_size = v; }
  size_t max_heap_byte_size()     { return _max_heap_byte_size; }
  void set_max_heap_byte_size(size_t v) { _max_heap_byte_size = v; }
  size_t min_heap_byte_size()     { return _min_heap_byte_size; }
  void set_min_heap_byte_size(size_t v) { _min_heap_byte_size = v; }

  enum Name {
    CollectorPolicyKind,
    TwoGenerationCollectorPolicyKind,
    ConcurrentMarkSweepPolicyKind,
    ASConcurrentMarkSweepPolicyKind,
    G1CollectorPolicyKind,
    TypeGenCollectorPolicyKind
  };

  // Identification methods.
  virtual GenCollectorPolicy*           as_generation_policy()            { return NULL; }
  virtual TwoGenerationCollectorPolicy* as_two_generation_policy()        { return NULL; }
  virtual MarkSweepPolicy*              as_mark_sweep_policy()            { return NULL; }
 
 /*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
 virtual  TypeGenCollectorPolicy* 		as_type_gen_collector_policy() { return NULL; }

 /*-tony 02/06/2010 */
 
#ifndef SERIALGC
  virtual ConcurrentMarkSweepPolicy*    as_concurrent_mark_sweep_policy() { return NULL; }
  virtual G1CollectorPolicy*            as_g1_policy()                    { return NULL; }
#endif // SERIALGC
  // Note that these are not virtual.
  bool is_generation_policy()            { return as_generation_policy() != NULL; }
  bool is_two_generation_policy()        { return as_two_generation_policy() != NULL; }
  bool is_mark_sweep_policy()            { return as_mark_sweep_policy() != NULL; }
  /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
 
  bool is_typegen_policy()   { return as_type_gen_collector_policy()!=NULL;  }  //Added by Feng. Feb, 2006, changed by tony

 /*-tony 01/18/2010 */
 
#ifndef SERIALGC
  bool is_concurrent_mark_sweep_policy() { return as_concurrent_mark_sweep_policy() != NULL; }
  bool is_g1_policy()                    { return as_g1_policy() != NULL; }
#else  // SERIALGC
  bool is_concurrent_mark_sweep_policy() { return false; }
  bool is_g1_policy()                    { return false; }
#endif // SERIALGC


  virtual PermanentGenerationSpec *permanent_generation() {
    assert(_permanent_generation != NULL, "Sanity check");
    return _permanent_generation;
  }

  virtual BarrierSet::Name barrier_set_name() = 0;
  virtual GenRemSet::Name  rem_set_name() = 0;

  // Create the remembered set (to cover the given reserved region,
  // allowing breaking up into at most "max_covered_regions").
  virtual GenRemSet* create_rem_set(MemRegion reserved,
                                    int max_covered_regions);

  // This method controls how a collector satisfies a request
  // for a block of memory.  "gc_time_limit_was_exceeded" will
  // be set to true if the adaptive size policy determine that
  // an excessive amount of time is being spent doing collections
  // and caused a NULL to be returned.  If a NULL is not returned,
  // "gc_time_limit_was_exceeded" has an undefined meaning.
  virtual HeapWord* mem_allocate_work(size_t size,
                                      bool is_tlab,
                                      bool* gc_overhead_limit_was_exceeded) = 0;

 /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
virtual  HeapWord* mem_allocate_with_pretenuring(size_t size,
                                          bool is_tlab,
                                          bool pretenutring)=0;

 /*-tony 01/18/2010 */
                                          

  // This method controls how a collector handles one or more
  // of its generations being fully allocated.
  virtual HeapWord *satisfy_failed_allocation(size_t size, bool is_tlab) = 0;
  // Performace Counter support
  GCPolicyCounters* counters()     { return _gc_policy_counters; }

  // Create the jstat counters for the GC policy.  By default, policy's
  // don't have associated counters, and we complain if this is invoked.
  virtual void initialize_gc_policy_counters() {
    ShouldNotReachHere();
  }

  virtual CollectorPolicy::Name kind() {
    return CollectorPolicy::CollectorPolicyKind;
  }

  // Returns true if a collector has eden space with soft end.
  virtual bool has_soft_ended_eden() {
    return false;
  }

};

class GenCollectorPolicy : public CollectorPolicy {
 protected:
  size_t _min_gen0_size;
  size_t _initial_gen0_size;
  size_t _max_gen0_size;

  GenerationSpec **_generations;

  // The sizing of the different generations in the heap are controlled
  // by a sizing policy.
  AdaptiveSizePolicy* _size_policy;

  // Return true if an allocation should be attempted in the older
  // generation if it fails in the younger generation.  Return
  // false, otherwise.
  virtual bool should_try_older_generation_allocation(size_t word_size) const;

  void initialize_flags();
  void initialize_size_info();

  // Try to allocate space by expanding the heap.
  virtual HeapWord* expand_heap_and_allocate(size_t size, bool is_tlab);

  // compute max heap alignment
  size_t compute_max_alignment();

 // Scale the base_size by NewRation according to
 //     result = base_size / (NewRatio + 1)
 // and align by min_alignment()
 size_t scale_by_NewRatio_aligned(size_t base_size);

 // Bound the value by the given maximum minus the
 // min_alignment.
 size_t bound_minus_alignment(size_t desired_size, size_t maximum_size);

 public:
  // Accessors
  size_t min_gen0_size() { return _min_gen0_size; }
  void set_min_gen0_size(size_t v) { _min_gen0_size = v; }
  size_t initial_gen0_size() { return _initial_gen0_size; }
  void set_initial_gen0_size(size_t v) { _initial_gen0_size = v; }
  size_t max_gen0_size() { return _max_gen0_size; }
  void set_max_gen0_size(size_t v) { _max_gen0_size = v; }


 /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
 
 virtual bool is_typegen_policy()                    { return false;  }  //Added by Feng. Feb, 2006

 /*-tony 01/18/2010 */
 
 
  virtual int number_of_generations() = 0;

  virtual GenerationSpec **generations()       {
    assert(_generations != NULL, "Sanity check");
    return _generations;
  }

  virtual GenCollectorPolicy* as_generation_policy() { return this; }

  virtual void initialize_generations() = 0;

  virtual void initialize_all() {
    initialize_flags();
    initialize_size_info();
    initialize_generations();
  }

  HeapWord* mem_allocate_work(size_t size,
                              bool is_tlab,
                              bool* gc_overhead_limit_was_exceeded);

  HeapWord *satisfy_failed_allocation(size_t size, bool is_tlab);

  // The size that defines a "large array".
  virtual size_t large_typearray_limit();

  // Adaptive size policy
  AdaptiveSizePolicy* size_policy() { return _size_policy; }
  virtual void initialize_size_policy(size_t init_eden_size,
                                      size_t init_promo_size,
                                      size_t init_survivor_size);

};


// All of hotspot's current collectors are subtypes of this
// class. Currently, these collectors all use the same gen[0],
// but have different gen[1] types. If we add another subtype
// of CollectorPolicy, this class should be broken out into
// its own file.

class TwoGenerationCollectorPolicy : public GenCollectorPolicy {
 protected:
  size_t _min_gen1_size;
  size_t _initial_gen1_size;
  size_t _max_gen1_size;

  void initialize_flags();
  void initialize_size_info();
  void initialize_generations()                { ShouldNotReachHere(); }

 public:
  // Accessors
  size_t min_gen1_size() { return _min_gen1_size; }
  void set_min_gen1_size(size_t v) { _min_gen1_size = v; }
  size_t initial_gen1_size() { return _initial_gen1_size; }
  void set_initial_gen1_size(size_t v) { _initial_gen1_size = v; }
  size_t max_gen1_size() { return _max_gen1_size; }
  void set_max_gen1_size(size_t v) { _max_gen1_size = v; }

  // Inherited methods
  TwoGenerationCollectorPolicy* as_two_generation_policy() { return this; }


 /*tony 11/16/2009 Porting Type Based GC, Tony*/ 
  virtual bool is_typegen_policy()                    { return false;  }  //Added by Feng. Feb, 2006
  virtual  HeapWord* mem_allocate_work(size_t size,
                             // bool is_large_noref, //by tony
                              bool is_tlab, 
                              bool* gc_overhead_limit_was_exceeded //by tony 2010-2-10
                              );
  
  virtual  HeapWord* mem_allocate_with_pretenuring(size_t size,
                                         // bool is_large_noref,
                                          bool is_tlab,
                                          bool remote_non_use);    //Added by Feng, Jab 2006

 /*-tony 11/16/2009*/
 
  int number_of_generations()                  { return 2; }
  BarrierSet::Name barrier_set_name()          { return BarrierSet::CardTableModRef; }
  GenRemSet::Name rem_set_name()               { return GenRemSet::CardTable; }

  virtual CollectorPolicy::Name kind() {
    return CollectorPolicy::TwoGenerationCollectorPolicyKind;
	 
  }

	 /*tony 11/16/2009 Porting Type Based GC, Tony*/ 
	 /*
 HeapWord* mem_allocate_with_pretenuring(size_t size,
                                         // bool is_large_noref,
                                          bool is_tlab,
                                          bool pretenuring){return 0;}    //Added by Feng, Jab 2006
                                          //de-abstracted by tony 2010-1-31
*/
	 /*-tony 11/16/2009*/

  // Returns true is gen0 sizes were adjusted
  bool adjust_gen0_sizes(size_t* gen0_size_ptr, size_t* gen1_size_ptr,
                               size_t heap_size, size_t min_gen1_size);
};

class MarkSweepPolicy : public TwoGenerationCollectorPolicy {
 protected:
  void initialize_generations();

 public:
  MarkSweepPolicy();

  MarkSweepPolicy* as_mark_sweep_policy() { return this; }

  void initialize_gc_policy_counters();
};



/*tony 11/16/2009 Porting Type Based GC, Tony*/ 
// Type-based GC is a brand new design. In this technique, we virtually divide
// heap into three generations. All local objects are allocated in gen[0]. All remote
// objects are allocated in gen[1]. Gen[2] is mature space.  Added by Feng, Feb 1,2006
// And the TransGC is based on Type-based GC, difference is that the remote gen gen[1] is
// used as a storage for transaction sensitive objects, once a transaction is over, the region
// allocated for this transation is erased.  Added by Tony, 2009-11-16
class TypeGenCollectorPolicy : public TwoGenerationCollectorPolicy {
 protected:
  size_t _initial_local_gen_size;
  size_t _max_local_gen_size;
  size_t _initial_remote_gen_size;
  size_t _max_remote_gen_size;

 // void initialize_flags();
  void initialize_size_info();
  void initialize_generations();

 private:
  // Return true if an allocation should be attempted in the older
  // generation if it fails in the younger generation.  Return
  // false, otherwise.
  virtual bool should_try_older_generation_allocation(size_t word_size)  const ;

 public:
  // Inherited methods
  TypeGenCollectorPolicy();


   /*tony 02/06/2010 Fixing Type Based GC, Tony*/ 
  TypeGenCollectorPolicy* as_type_gen_collector_policy() { return this; }

   virtual CollectorPolicy::Name kind() {
    return CollectorPolicy::TypeGenCollectorPolicyKind;
	 
  }

   /*-tony 02/06/2010 */
     // Inherited methods

   /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
  void initialize_gc_policy_counters();
   /*-tony 01/18/2010 */
   
  bool is_two_generation_policy()              { return true; }
  bool is_typegen_policy()                    { return true;  }

  int number_of_generations()                  { return 3; }
  BarrierSet::Name barrier_set_name()          { return BarrierSet::CardTableModRef; }
  GenRemSet::Name rem_set_name()               { return GenRemSet::CardTable; }

  
  virtual HeapWord* mem_allocate_work(size_t size,
                             // bool is_large_noref, //by tony
                              bool is_tlab, 
                              bool* gc_overhead_limit_was_exceeded //by tony 2010-2-10
                              );
  

  HeapWord* mem_allocate_with_pretenuring(size_t size,
                                                          bool is_tlab,
                                                          bool pretenutring);  //Added by Feng, Jab 2006
  HeapWord *satisfy_failed_type_based_allocation(size_t size,
                                //      bool is_large_noref,
                                      bool is_tlab,
                                //      bool& notify_ref_lock,
                                      bool remote);

};



/*-tony 11/16/2009*/

