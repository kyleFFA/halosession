#include "pch.h"
#include "ModuleCache.h"


// Adapted from scales' MCCTAS https://github.com/Scaless/HaloTAS/blob/master/HaloTAS/libhalotas/dll_cache.cpp
std::unordered_map<std::wstring, MODULEINFO> ModuleCache::mCache = {};

void ModuleCache::initialize() {
    std::unordered_map<std::wstring,MODULEINFO> snapshot;
    HMODULE modules[1024];DWORD needed=0;
    if(EnumProcessModules(GetCurrentProcess(),modules,sizeof(modules),&needed)) {
        const size_t count=std::min<size_t>(needed/sizeof(HMODULE),std::size(modules));
        for(size_t i=0;i<count;++i) {
            wchar_t name[MAX_PATH]{};MODULEINFO info{};
            if(GetModuleBaseNameW(GetCurrentProcess(),modules[i],name,MAX_PATH) &&
               GetModuleInformation(GetCurrentProcess(),modules[i],&info,sizeof(info))) snapshot.emplace(name,info);
        }
    }
    std::lock_guard lock(cacheMutex);
    mCache=std::move(snapshot);generations.clear();
    for(const auto& [name,info]:mCache) generations[name]=++nextGeneration;
}

bool ModuleCache::addModuleToCache(const std::wstring& moduleName,HMODULE moduleHandle) {
    if(!moduleHandle) return false;
    MODULEINFO info{};
    // Never hold our cache lock while entering Windows loader APIs.
    if(!GetModuleInformation(GetCurrentProcess(),moduleHandle,&info,sizeof(info))) return false;
    std::lock_guard lock(cacheMutex);
    const auto it=mCache.find(moduleName);
    if(it!=mCache.end() && it->second.lpBaseOfDll==moduleHandle) return true;
    mCache[moduleName]=info;generations[moduleName]=++nextGeneration;
    return true;
}

bool ModuleCache::removeModuleFromCache(const std::wstring& moduleName)
{
    std::lock_guard lock(cacheMutex);
	if (auto it = ModuleCache::mCache.find(moduleName); it != ModuleCache::mCache.end()) {
		ModuleCache::mCache.erase(it);
		return true;
	}

	return false;
}

std::optional<HMODULE> ModuleCache::getModuleHandle(const std::wstring& moduleName)
{
    std::lock_guard lock(cacheMutex);
	if (auto it = ModuleCache::mCache.find(moduleName); it != ModuleCache::mCache.end()) {
		return (HMODULE)it->second.lpBaseOfDll;
	}

	return std::nullopt;
}

std::optional<MODULEINFO> ModuleCache::getModuleInfo(const std::wstring& moduleName)
{
    std::lock_guard lock(cacheMutex);
	if (auto it = ModuleCache::mCache.find(moduleName); it != ModuleCache::mCache.end()) {
		return it->second;
	}

	return std::nullopt;
}

bool ModuleCache::isModuleInCache(const std::wstring& moduleName)
{
    std::lock_guard lock(cacheMutex);
	auto it = ModuleCache::mCache.find(moduleName);
	return it != ModuleCache::mCache.end();

}
uint64_t ModuleCache::generation(const std::wstring& name) { std::lock_guard lock(cacheMutex); auto it=generations.find(name);return it==generations.end()?0:it->second; }
