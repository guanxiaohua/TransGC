// marker descriptor

# include "incls/_precompiled.incl"
# include "incls/_markerDescriptor.cpp.incl"

GrowableArray<markerDescriptor*>* markerDescManager::_descs = NULL;

void init_predicate_watchers();
void init_predicate_manager();

void initialize_marker_support(){
	assert (markerDescManager::_descs == NULL, 
		"Marker support has already been initialized!");
	
	// Caveat #1: allocate as resource obj, otherwise memory leak will be deemed.
	markerDescManager::_descs = new (ResourceObj::C_HEAP) 
		GrowableArray<markerDescriptor*>(0, true);
		
	// Caveat #2: index starts at ONE, not ZERO
	markerDescManager::_descs->push(NULL);
	
	init_predicate_watchers(); // load predicate watchers
}

markerDescriptor* markerDescManager::add(u2 mid, void *cid, 
																				void *info, short oneway) { 
	assert (_descs != NULL, "Marker support is not ready!");
	markerDescriptor *md = new markerDescriptor(mid, cid, info, oneway);
	_descs->at_put_grow(mid, md, NULL);
	
	return md;
}

markerDescriptor* markerDescManager::get(int which) { 
	if (which < _descs->length() && which >= 0)
		return _descs->at(which); 
	else {
		tty->print_cr(
			"[AS-ERROR] Trying to obtain a non-existed descriptor: %d", which);
		return NULL;
	}
}
