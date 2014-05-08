# include "incls/_precompiled.incl"
# include "incls/_predicateRepository.cpp.incl"
static int _clientCount = 0;
#define INIT_CLIENT(name) name::init(_clientCount++)

void init_predicate_watchers() {
	//init_watcher(syncRegionViolationWatcher);
	//init_watcher(mutexWatcher);
	//init_watcher(sequenceWatcher);
	//init_watcher(fsmWatcher);
  //INIT_CLIENT(MTFPerfWatcher);
  //INIT_CLIENT(syncDistanceWatcher);
}

void print_watcher_results() {
	predicateManager::print_watcher_results();
}
int clientCount() { return _clientCount; }

