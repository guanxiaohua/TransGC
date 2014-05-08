# include "incls/_precompiled.incl"
# include "incls/_predicateManager.cpp.incl"

int predicateManager::_invocation_counter = 0;
predicateWatcher* predicateManager::_wpool = NULL;
predicateWatcher* predicateManager::_klass_load_watchers = NULL;

void predicateManager::initialize() {
	// nothing for now
}

void predicateManager::subscribe (predicateWatcher *w) {
	DL_APPEND(_wpool, w);
	if (w->method_loaded) 
		DL_APPEND(_klass_load_watchers, w);
}

void predicateManager::broadcast (markerDescriptor *md) {
	predicateWatcher *w;
	DL_FOREACH(_wpool, w) {
		if (w->attach(md)){
			DL_APPEND(md->_watchers, w);
		}
	}
}

void predicateManager::print_watcher_results () {
	predicateWatcher *w;
	DL_FOREACH(_wpool, w) {
		if (w->print) w->print();
	}
  
	tty->print_cr("[AS] Total marker invocations: %d", 
			          predicateManager::invocation_counter());
}

void predicateManager::method_loaded (methodHandle method) {
	predicateWatcher *w;
	HandleMark hm(Thread::current());
	
	DL_FOREACH(_klass_load_watchers, w) {
		w->method_loaded(method);
	}
}

void predicateManager::methods_loaded (objArrayOop methods) {
	if (_klass_load_watchers != NULL) {
    predicateWatcher *w;
    HandleMark hm(Thread::current());
    methodHandle method; // recyclable
      
    for(int i = 0; i < methods->length(); i++) {
      method = (methodOop)methods->obj_at(i);
      
      DL_FOREACH(_klass_load_watchers, w) {
        w->method_loaded(method);
      }
    }
  }
}

#ifdef PROFILE_APP_METHOD

#include <stdio.h>

FILE* dumpfile = NULL;

static void app_class_do(klassOop k) {
  if (k->klass_part()->is_app_klass()) {    
    ResourceMark rm(Thread::current());
    HandleMark hm(Thread::current());
    instanceKlassHandle this_klass (Thread::current(), k);
    objArrayOop methods = this_klass->methods();
    methodOop method = NULL;
    
    for(int i = 0; i < methods->length(); i++) {
      method = (methodOop)methods->obj_at(i);
      int count = method->interpreter_invocation_count();
      if (count > 0) {          
        fprintf(dumpfile, "%d ", count);          
      #ifdef PRODUCT
        fprintf(dumpfile, "%s ", this_klass->external_name());
      #else
        fprintf(dumpfile, "%s ", this_klass->internal_name());
      #endif
        fprintf(dumpfile, "%s %s\n", 
                method->name()->as_C_string(), 
                method->signature()->as_C_string());
      }
    }    
  }
}

void dump_method_profiles () {
	char home_dir[1024];
	char filename[1024];
  char benchmark[256];
  if (!os::getenv("DUMPDIR", home_dir, 1024)){
    os::getenv("HOME", home_dir, 1024);
  }  
	if (!os::getenv("BMK", benchmark, 255)){
    strcpy(benchmark, "noname");
  }
	snprintf(filename, 1024, "%s/method_dump_%s.log", 
           home_dir, benchmark);
	
  dumpfile = fopen(filename, "w");
	SystemDictionary::classes_do(app_class_do);
  fclose(dumpfile);
}
#endif
