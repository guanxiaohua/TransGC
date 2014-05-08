# include "incls/_precompiled.incl"
# include "incls/_syncDistanceWatcher.cpp.incl"

#define PREDICATE "sync-distance"

uint32_t syncDistanceWatcher::_conflicts = 0;
markerDescriptor* syncDistanceWatcher::barrier = NULL;
std::map<pid_t, CallTreeNode*> syncDistanceWatcher::_call_trees;

#define MORE_INIT() { \
  _watcher->print = syncDistanceWatcher::do_print; \
}

IMPLEMENT_WATCHER_INIT(syncDistanceWatcher)


bool syncDistanceWatcher::do_attach(markerDescriptor *md) {
	assert (md->info() != NULL, "[AS-SD] Invalid predicate string."); 
	
	bool ret = false;
	char *preds = (char*)md->info();
	if (strstr(preds, PREDICATE)) {		
		ret = true; // we want to subscribe
		tty->print_cr("[AS] sync-distance-watcher activated (%d)", md->mid());	
	}
	return ret;
}

void syncDistanceWatcher::do_enter(marker *m){
  JavaThread* thread = (JavaThread*)Thread::current();
  int mid = m->desc()->mid();
  if (mid != SPECIAL_MID){
    CallTreeNode* parent = (CallTreeNode*)thread->get_client_tag(_clientID);
    if (!parent){
      tty->print_cr("[AS] New root %d:%d", thread->tid(), mid);
      CallTreeNode* node = new CallTreeNode(m->desc()->mid());      
      _call_trees.insert(std::make_pair(node->mid(), node));
      
      // cache the last node
      thread->set_client_tag(_clientID, node);
    }
    else {
      tty->print_cr("[AS] %d:%d ---> %d", thread->tid(), parent->mid(), mid);
      CallTreeNode* node = parent->addSuccesor(mid); // wind
      thread->set_client_tag(_clientID, node);
    }
  }
  else { // walk back call tree and update hit counters
    CallTreeNode* last = (CallTreeNode*)thread->get_client_tag(_clientID);
    tty->print_cr("[AS] %d: hit @ %d", thread->tid(), last->mid());
    for (; last; last->do_hit(), last=last->parent());
  }
}

void syncDistanceWatcher::do_exit(marker *m) {		
  int mid = m->desc()->mid();
  if (mid == SPECIAL_MID) // nothing to do for SPECIAL
    return;
    
  JavaThread* thread = (JavaThread*)Thread::current();    
  CallTreeNode* current = (CallTreeNode*)thread->get_client_tag(_clientID);
  assert (current, "where is this node from?");
  
  thread->set_client_tag(_clientID, current->parent());  // unwind
}

#include <list>

void DFS_CallTree(CallTreeNode* node, std::list<CallTreeNode*>& actives){
  // push node on stack
  actives.push_back(node);
  
  std::map<int, CallTreeNode*> succs = node->getSuccessors();    
  if (succs.size() == 0){ // down at leaf
    std::list<CallTreeNode*>::iterator it;
    tty->cr();
    for (it=actives.begin(); it!=actives.end(); ++it){
      tty->print_cr("m%d = {%d, %d, %f}", (*it)->mid(), (*it)->trip(), 
        (*it)->hit(), (*it)->hitRatio());
    }
  }
  else { // recurse       
    std::map<int, CallTreeNode*>::iterator it;
    for (it = succs.begin(); it != succs.end(); ++it)
      DFS_CallTree(it->second, actives);
  }
  
  // pop node off stack
  actives.pop_back();
}

void syncDistanceWatcher::do_print() {	
  std::map<pid_t, CallTreeNode*>::iterator it; 
  for (it = _call_trees.begin(); it != _call_trees.end(); ++it){ 
    CallTreeNode* root = it->second;
    std::list<CallTreeNode*> actives;
    DFS_CallTree(root, actives);
  }
}

