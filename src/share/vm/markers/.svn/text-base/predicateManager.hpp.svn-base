//@@PENG
// predicateManager is responsible for manage predicates and delegate 
// the processings to each predicate handler, which in turn apply the 
// corresponding constraints at the runtime. 

// predicateHandler is the interface that should be implemented by 
// classes interested in handling marker predicates and handling 
// them during program runtime. 
// 
//class predicateHandler {
//public:
//	void process (int mid, const char* ps) = 0;
//	void handle () = 0;
//}

#include "../utilities/utlist.h"

class marker;
class markerDescriptor;

struct predicateWatcher VALUE_OBJ_CLASS_SPEC {
	// to make a utlist
	predicateWatcher *prev;
  predicateWatcher *next;
	
	// mostly mandatory callbacks
	bool (*attach)(markerDescriptor *md);

#ifdef TWO_WAY_MARKER
  void (*enter)(marker *m);
	void (*exit)(marker *m);
#else
  void (*enter)(markerDescriptor *md);
#endif
	
	// optional callbacks
	void (*print)();
	void (*method_loaded)(methodHandle m);
};

#ifdef TWO_WAY_MARKER    

#define DECLARE_WATCHER() \
	private: \
    static int _clientID; \
		static predicateWatcher _watcher[1]; \
	public: \
		static bool do_attach(markerDescriptor *md); \
		static void do_enter(marker *m); \
		static void do_exit(marker *m); \
		static void init(int clientID);

#define IMPLEMENT_WATCHER_BASE(name) \
	predicateWatcher name::_watcher[1]; \
  int name::_clientID = 0; \
	void name::init(int clientID) { \
    _clientID = clientID; \
		_watcher->attach = name::do_attach; \
		_watcher->enter = name::do_enter; \
		_watcher->exit = name::do_exit; \
		_watcher->print = NULL; \
		_watcher->method_loaded = NULL;
	
#else

#define DECLARE_WATCHER() \
	private: \
    static int _clientID; \
		static predicateWatcher _watcher[1]; \
	public: \
		static bool do_attach(markerDescriptor *md); \
		static void do_enter(markerDescriptor *md); \
		static void init(int clientID);

#define IMPLEMENT_WATCHER_BASE(name) \
	predicateWatcher name::_watcher[1]; \
  int name::_clientID = 0; \
	void name::init(int clientID) { \
    _clientID = clientID; \
		_watcher->attach = name::do_attach; \
		_watcher->enter = name::do_enter; \
		_watcher->print = NULL; \
		_watcher->method_loaded = NULL;
  
#endif // TWO_WAY_MARKER

#define IMPLEMENT_WATCHER(name) \
		IMPLEMENT_WATCHER_BASE(name) \
		predicateManager::subscribe(_watcher); \
	}

#define IMPLEMENT_WATCHER_INIT(name) \
		IMPLEMENT_WATCHER_BASE(name) \
		MORE_INIT() \
		predicateManager::subscribe(_watcher); \
	}

#define print_watcher(name) name::_watcher->print()
#define LINK_POISON 0xbaddbabe

class predicateManager: AllStatic {
private:
	static predicateWatcher* _wpool;
	static predicateWatcher* _klass_load_watchers;
	
	static int  _invocation_counter;
	
public:
	static void initialize();

	static bool has_watchers()            { return _wpool != NULL;      }
	static int invocation_counter()       { return _invocation_counter; }

	static void subscribe (predicateWatcher *w);	
	static void broadcast (markerDescriptor *md);
	
	static void method_loaded  (methodHandle method);
	static void methods_loaded (objArrayOop methods);
	
	static void print_watcher_results ();
	
#ifdef TWO_WAY_MARKER
	inline
	static void marker_enter (marker *m) {
		predicateWatcher *w;
		
		++ _invocation_counter;
		DL_FOREACH(m->desc()->watchers(), w) {
			w->enter(m);
		}
	}
	
	inline
	static void marker_exit (marker *m) {
		predicateWatcher *w;
		
		DL_FOREACH(m->desc()->watchers(), w) {
			w->exit(m);
		}
	}
#else
  inline
	static void marker_enter (u2 mid) {
		predicateWatcher *w;
		markerDescriptor* md = markerDescManager::get(mid);
		++ _invocation_counter;
		DL_FOREACH(md->watchers(), w) {
			w->enter(md);
		}
	}
#endif // TWO_WAY_MARKER

private:
	predicateManager();
};
