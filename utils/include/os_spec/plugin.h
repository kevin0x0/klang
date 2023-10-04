#ifndef KEVCC_UTILS_INCLUDE_OS_SPEC_PLUGIN_H
#define KEVCC_UTILS_INCLUDE_OS_SPEC_PLUGIN_H

typedef void* KevPlugin;

KevPlugin kev_plugin_load(const char* filename);
void kev_plugin_unload(KevPlugin plugin);
void* kev_plugin_get_symbol(const char* symbolname);

#endif
