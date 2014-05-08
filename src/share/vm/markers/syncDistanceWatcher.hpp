//@@PENG

// syncRegionViolationWatcher is a watcher for synchronization region 
// violations. Basically, it will count the times that more than one 
// threads are in the same "synchronized" region. This could happen 
// because our back-end analysis will remove the synchronized keyword
// and wrap the region with our marker bytecodes. 
//
// This watcher is activated when there are such predicates in the 
// metadata in the classfile. 

#include <map>
#include <utility>

class marker;
class markerDescriptor;
class CallTreeNode;

class syncDistanceWatcher: AllStatic {
  enum { SPECIAL_MID = 0 };
private:
	static uint32_t	_conflicts;
  static markerDescriptor* barrier;
  static int special_mid;
  static std::map<pid_t, CallTreeNode*> _call_trees;
	
	DECLARE_WATCHER ()
	static void do_print();
};

class CallTreeNode {
  const int _mid;
  CallTreeNode* _parent;
  std::map<int, CallTreeNode*> _succs;
  
  unsigned long _trip;
  unsigned long _hit;
  
public:
  CallTreeNode(int mid, CallTreeNode* parent=NULL)
    :_mid(mid), _parent(parent), _trip(1), _hit(0) {}
  
  // node accessing routines
  int mid() const { return _mid; }
  CallTreeNode* parent() { return _parent; }
  
  std::map<int, CallTreeNode*>& getSuccessors(){
    return _succs;
  }
  
  // counter accessors
  void do_trip()    { ++_trip; }
  void do_hit()     { ++_hit; }
  
  int trip() const { return _trip; }
  int hit() const  { return _hit; }
  
  double hitRatio() const { return double(_hit)/_trip; }
    
  CallTreeNode* addSuccesor(int mid){
    std::map<int, CallTreeNode*>::iterator it;
    if ((it = _succs.find(mid)) == _succs.end()){
      CallTreeNode* node = new CallTreeNode(mid, this);
      _succs.insert(std::make_pair(mid, node));
      return node;
    }
    else {
      it->second->do_trip();
      return it->second;
    }
  }
};

