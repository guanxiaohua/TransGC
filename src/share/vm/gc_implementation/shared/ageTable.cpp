/*
 * Copyright 1997-2009 Sun Microsystems, Inc.  All Rights Reserved.
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

/* Copyright 1992-2009 Sun Microsystems, Inc. and Stanford University.
   See the LICENSE file for license information. */

# include "incls/_precompiled.incl"
# include "incls/_ageTable.cpp.incl"

ageTable::ageTable(bool global,bool generation0) {

  clear();
 
  if (UsePerfData && global) {

    ResourceMark rm;
    EXCEPTION_MARK;


    const char* agetable_ns = "generation.0.agetable";
    const char* agetable_ns_1= "generation.1.agetable";
	

	  /*tony 01/18/2010 Porting Type Based GC, Tony*/ 

    const char* bytes_ns = PerfDataManager::name_space((!generation0)&&TransGC?agetable_ns_1:agetable_ns, "bytes");

	  /*-tony 01/18/2010 */
	  
	

    for(int age = 0; age < table_size; age ++) {
      char age_name[10];
      jio_snprintf(age_name, sizeof(age_name), "%2.2d", age);
      const char* cname = PerfDataManager::counter_name(bytes_ns, age_name);
      _perf_sizes[age] = PerfDataManager::create_variable(SUN_GC, cname,
                                                          PerfData::U_Bytes,
                                                          CHECK);
    }

    const char* cname = PerfDataManager::counter_name(generation0?agetable_ns:agetable_ns_1, "size");
    PerfDataManager::create_constant(SUN_GC, cname, PerfData::U_None,
                                     table_size, CHECK);
	 /*tony 01/18/2010 Porting Type Based GC, Tony*/ 
	if(TransGC)
	     generation0=false;
	 /*-tony 01/18/2010 */

  }
}

void ageTable::clear() {
  for (size_t* p = sizes; p < sizes + table_size; ++p) {
    *p = 0;
  }
}

void ageTable::merge(ageTable* subTable) {
  for (int i = 0; i < table_size; i++) {
    sizes[i]+= subTable->sizes[i];
  }
}

void ageTable::merge_par(ageTable* subTable) {
  for (int i = 0; i < table_size; i++) {
    Atomic::add_ptr(subTable->sizes[i], &sizes[i]);
  }
}

int ageTable::compute_tenuring_threshold(size_t survivor_capacity) {
  size_t desired_survivor_size = (size_t)((((double) survivor_capacity)*TargetSurvivorRatio)/100);
  size_t total = 0;
  int age = 1;
  assert(sizes[0] == 0, "no objects with age zero should be recorded");
  while (age < table_size) {
    total += sizes[age];
    // check if including objects of age 'age' made us pass the desired
    // size, if so 'age' is the new threshold
    if (total > desired_survivor_size) break;
    age++;
  }
  int result = age < MaxTenuringThreshold ? age : MaxTenuringThreshold;

  if (PrintTenuringDistribution || UsePerfData) {

    if (PrintTenuringDistribution) {
      gclog_or_tty->cr();
      gclog_or_tty->print_cr("Desired survivor size %ld bytes, new threshold %d (max %d)",
        desired_survivor_size*oopSize, result, MaxTenuringThreshold);
    }

    total = 0;
    age = 1;
    while (age < table_size) {
      total += sizes[age];
      if (sizes[age] > 0) {
        if (PrintTenuringDistribution) {
          gclog_or_tty->print_cr("- age %3d: %10ld bytes, %10ld total",
            age, sizes[age]*oopSize, total*oopSize);
        }
      }
      if (UsePerfData) {
        _perf_sizes[age]->set_value(sizes[age]*oopSize);
      }
      age++;
    }
    if (UsePerfData) {
      SharedHeap* sh = SharedHeap::heap();
      CollectorPolicy* policy = sh->collector_policy();
      GCPolicyCounters* gc_counters = policy->counters();
      gc_counters->tenuring_threshold()->set_value(result);
      gc_counters->desired_survivor_size()->set_value(
        desired_survivor_size*oopSize);
    }
  }

  return result;
}
