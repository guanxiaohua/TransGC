//@@PENG
// A markerDescriptor describes a marker and will be populated when a 
// class file is loaded.

class predicateWatcher;
class predicateManager;
class markerDescriptor: public CHeapObj {
	friend class predicateManager; // allow access to _watchers
	
private:
	u2                _mid; 	// marker id
	short              _one_way; // only enter, no exit
	void							*_cid; 	// class id => assigned by our analysis	
	void 							*_info; 	// extra info, e.g. predicates	
	predicateWatcher	*_watchers; 	// extra info, e.g. predicates

public:
	u2 mid() const          { return _mid; }
	void* cid() const        { return _cid; }
	void* info() const       { return _info; }
	short one_way() const    { return _one_way; }
	
	predicateWatcher* watchers() { return _watchers; }

public:
	markerDescriptor(u2 mid, void *cid, void *info, short one_way)
		:_mid(mid), _cid(cid), _info(info),
		 _watchers(NULL), _one_way(one_way)
	{
	}
};

class markerDescManager: AllStatic {
private:
	static GrowableArray<markerDescriptor*> 	*_descs;

public:
	static markerDescriptor* add(u2 mid, void *cid, void *info, short oneway=0);
	static markerDescriptor* get(int which);

public:
	friend void initialize_marker_support();
};

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
