//@@PENG
// A markerDescriptor describes a marker and will be populated when a 
// class file is loaded. 


 //tony-> 2011/02/28 adding new bytecodes
//#define TWO_WAY_MARKER 
 //<-tony 2011-02-28
#ifdef TWO_WAY_MARKER

#include <unistd.h>
#include <sys/types.h>

//#include "../utilities/utlist.h" // runtime marker list ////by tony 2011/02/28

//#include <vector>
//#include <stack>

class markerStack;
class markerDescriptor;

class marker: public CHeapObj {
	friend class markerStack;
	
private:
  pid_t 						_tid;			/* unix thread id */
	intptr_t					*_sp;			/* stack pointer */
	markerDescriptor	*_desc;		/* descriptor */
	bool 							_pending; /* pending if previously blocked */ 

public:
	pid_t tid() const             { return _tid; 		 }
	intptr_t* sp() const          { return _sp; 		 }
	markerDescriptor* desc()      { return _desc; 	 }
	bool pending() const          { return _pending; }
	void set_pending(bool p)      { _pending = p;    }

	void init (pid_t tid, intptr_t *sp, markerDescriptor *desc) {
		_tid = tid;
		_sp = sp;
		_desc = desc;
		_pending = false;
	}

public:
	marker()
		:_tid(0), _sp(NULL), _desc(NULL), _pending(false) {
	}
	
	marker(pid_t tid, intptr_t *sp, markerDescriptor *desc)
		:_tid(tid), _sp(sp), _desc(desc), _pending(false) {
	}

//public:
//	// to make a utlist
//	marker 		*prev;
//  marker 		*next;
};

//-------------------------------------------------------------

// This class provides some utility methods for manipulating 
// a stack of markers. Calling it stack is because markers 
// work in a LIFO manner. 
// 
// Note: marker stack is per-thread
class markerStack: public CHeapObj {
private:
	pid_t 	_tid;				// unix thread id
  size_t  _sp;
  std::stack<marker*> _stack;
//	marker	*_stack;
//	marker	*_free_slot;
//	marker  _reusable;  // for oneway markers

public:
	markerStack(pid_t tid):_sp(0), _tid(tid) {
	}
	
//	~markerStack() {
//		marker *tmp = NULL;
//		DL_FOREACH(_stack, tmp) {
//			DL_DELETE(_stack, tmp);
//			delete tmp;
//		}
//	}

public:
	inline pid_t tid() const { return _tid; }
	
	marker* push(u2 mid, pid_t pid, intptr_t* sp);
	marker* pop();
	marker* peek();	
};

//-------------------------------------------------------------
inline
marker* markerStack::push(u2 mid, pid_t tid, intptr_t* sp) {	
	marker *nm;
	markerDescriptor *desc = markerDescManager::get(mid);
	/*
  if (_sp < _stack.size()){
    nm = _stack[_sp++];
    nm->init(tid, sp, desc);
  }
  else {*/
    nm = new marker(tid, sp, desc); 
    _stack.push(nm);
  //}
  
  return nm;
}

inline 
marker* markerStack::pop() {
  //assert (_sp >= 0, "invalid stack pointer.");
  if (_stack.size() > 0){
    marker* mk = _stack.top();  
    _stack.pop();
    return mk;
  }
  else {
    return NULL;
  }
}

/*
inline
marker* markerStack::push(short mid, pid_t tid, intptr_t* sp) {	
	marker *nm;
	markerDescriptor *desc = markerDescManager::get(mid);
	if (desc->one_way()) { 
		// oneway marker can share the 
		// reusable marker object
		_reusable.init(tid, sp, desc);
		nm = &_reusable;
	}
	else { // two-way markers		
		if (_free_slot) {
			// pick a free slot 
			nm = _free_slot;		
			
			// set its content
			nm->_tid = tid;
			nm->_sp = sp;
			nm->_desc = desc;
			
			// == forward the free-slot ==
			// note that the list is constructed by prepending, so the
			// last element is the very first in the list and the list
			// expands towards the list head.
			if (_free_slot != _stack)
				// still more room, grab next
				_free_slot = _free_slot->prev; 
			else
				// stack is full now, no free slot
				_free_slot = NULL; 
		}
		else {
			// no free slot is available, go ahead and 
			// allocate a marker instead
			nm = new marker(tid, sp, desc);
			DL_PREPEND(_stack, nm);
		}
	}
	
	return nm;
}

inline 
marker* markerStack::pop() {
	// invariant: _stack will be NULL only at the very beginning, 
	// as soon as a marker is created, the _stack will remain 
	// non-null since we don't deallocate markers explicitly, only
	// advance _free_slot pointers back-and-forth. So, _stack should
	// never be NULL when calling pop().
	// 
	assert (_stack != NULL, "[AS-ERROR] Popping from an empty marker stack.");
	
	// _free_slot is the end of the list
	if (_free_slot) 
	  // expand free-list
		_free_slot = _free_slot->next;	
	else 
		// grab the top-of-stack
		_free_slot = _stack;
	
	// No deallocation and pop anymore
	// DL_DELETE(_stack, _stack); 	
	// -- _length;
	
	// top-of-stack is the current free slot
	return _free_slot;
}
*/
inline 
marker* markerStack::peek() {
	return _stack.size() > 0 ? _stack.top() : NULL;
}
#endif //TWO_WAY_MARKER
