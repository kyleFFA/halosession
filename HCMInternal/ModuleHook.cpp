#include "pch.h"
#include "ModuleHook.h"
#include "ModuleCache.h"
#include "ModuleHookManager.h"
#include "ScopedThreadSuspender.h"
#include "HookQuiescence.h"

std::unique_ptr<ModuleInlineHook> ModuleInlineHook::make(const std::wstring associatedModule, std::shared_ptr<MultilevelPointer> original_func, void* new_func, bool startEnabled)
{
	auto ptr = std::unique_ptr< ModuleInlineHook>(new ModuleInlineHook(associatedModule, original_func, new_func, startEnabled));
	ModuleHookManager::addHook(associatedModule, ptr.get());
	return ptr;

}

std::unique_ptr<ModuleMidHook> ModuleMidHook::make(const std::wstring associatedModule, std::shared_ptr<MultilevelPointer> original_func, safetyhook::MidHookFn new_func, bool startEnabled, bool gameplayScoped)
{
	auto ptr = std::unique_ptr< ModuleMidHook>(new ModuleMidHook(associatedModule, original_func, new_func, startEnabled, gameplayScoped));
	ModuleHookManager::addHook(associatedModule, ptr.get());
	return ptr;
}

std::unique_ptr<ModulePatch> ModulePatch::make(const std::wstring associatedModule, std::shared_ptr<MultilevelPointer> original_func, std::vector<byte> patchedBytes, bool startEnabled)
{
	auto ptr = std::unique_ptr< ModulePatch>(new ModulePatch(associatedModule, original_func, patchedBytes, startEnabled));
	ModuleHookManager::addHook(associatedModule, ptr.get());
	return ptr;
}

std::unique_ptr<ModulePatch> ModulePatch::makeNOPCall(const std::wstring associatedModule, std::shared_ptr<MultilevelPointer> original_func, bool startEnabled)
{
	return ModulePatch::make(associatedModule, original_func, std::vector<byte>{0x90, 0x90, 0x90, 0x90, 0x90}, startEnabled);
}

const std::wstring& ModuleHookBase::getAssociatedModule() const
{
	return this->mAssociatedModule;
}

ModuleInlineHook::~ModuleInlineHook()
{
	detach(); // H2 lifecycle owner retires hooks before any service destructor.
	ModuleHookManager::removeHook(getAssociatedModule(), this);

}

ModuleMidHook::~ModuleMidHook()
{
	detach(); // H2 lifecycle owner retires hooks before any service destructor.
	ModuleHookManager::removeHook(getAssociatedModule(), this);
}

ModulePatch::~ModulePatch()
{
	detach(); // H2 lifecycle owner retires hooks before any service destructor.
	ModuleHookManager::removeHook(getAssociatedModule(), this);
}



void ModuleInlineHook::attach()
{
    std::unique_lock stateLock(ModuleHookManager::h2StateMutex(),std::defer_lock);
    if(getAssociatedModule()==L"halo2.dll") { stateLock.lock(); if(!ModuleHookManager::h2EffectsAllowed()) return; }
    std::lock_guard freezeLock(hcmThreadSuspensionMutex()); // serialize with native H2 scalar patches
	PLOG_DEBUG << "inline_hook attempting attach: " << getAssociatedModule();
	PLOG_VERBOSE << "hookFunc " << this->mHookFunction;
	if (this->isHookInstalled()) {
		PLOG_DEBUG << "attach failed: hook already installed";
		return;
	}

	if (this->mOriginalFunction == nullptr)
	{
		PLOG_DEBUG << "attach failed: no pointer to original function";
		return;
	}

	uintptr_t pOriginalFunction;
	if (!this->mOriginalFunction->resolve(&pOriginalFunction))
	{
		PLOG_ERROR << "attach failed: pOriginalFunction pointer failed to resolve" << MultilevelPointer::GetLastError();
		return;
	}

	PLOG_VERBOSE << "pOriginalFunction " << std::hex << pOriginalFunction;

	this->mInlineHook = safetyhook::create_inline((void*)pOriginalFunction, this->mHookFunction);

	PLOG_DEBUG << "inline_hook successfully attached: " << this->getAssociatedModule();

}

void ModuleMidHook::attach()
{
    std::unique_lock stateLock(ModuleHookManager::h2StateMutex(),std::defer_lock);
    if(usesGameplayLifecycle()) { stateLock.lock(); if(!ModuleHookManager::gameplayEffectsAllowed(getAssociatedModule())) return; }
    std::lock_guard freezeLock(hcmThreadSuspensionMutex()); // serialize with native H2 scalar patches
    if(mMidHook && !mMidHook.enabled() && !detach()) return; // Retired stub still owns a callback return.
	PLOG_DEBUG << "mid_hook attempting attach: " << this->getAssociatedModule();
	if (this->isHookInstalled()) {
		PLOG_DEBUG << "attach failed: hook already installed";
		return;
	}

	if (this->mOriginalFunction == nullptr)
	{
		PLOG_DEBUG << "attach failed: no pointer to original function";
		return;
	}

	uintptr_t pOriginalFunction;
	if (!this->mOriginalFunction->resolve(&pOriginalFunction))
	{
		PLOG_ERROR << "attach failed: pOriginalFunction pointer failed to resolve: " << MultilevelPointer::GetLastError();
		return;
	}

	PLOG_VERBOSE << "pOriginalFunction: " << std::hex << pOriginalFunction;
	PLOG_VERBOSE << "mHookFunction: " << std::hex << this->mHookFunction;
	PLOG_VERBOSE << "this->mMidhook: " << this->mMidHook.operator bool();


	this->mMidHook = safetyhook::create_mid((void*)pOriginalFunction, this->mHookFunction);


	PLOG_DEBUG << "mid_hook successfully attached: " << this->getAssociatedModule();
}

void ModuleInlineHook::setCallbackDrain(std::function<bool()> drained) {
    callbacksDrained=std::move(drained);
    DWORD64 image=0;
    if(auto fn=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(mHookFunction),&image,nullptr)) {
        callbackBegin=image+fn->BeginAddress;callbackSize=fn->EndAddress-fn->BeginAddress;
    }
}
bool ModuleInlineHook::prepareDetach() {
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(!mInlineHook || !mInlineHook.enabled()) return true;
    const auto result=mInlineHook.try_disable();
    if(!result) { setCleanupFailure("grid entry-hook disable failed"); PLOG_ERROR<<"[HookCleanup] disable failed; retaining trampoline and module: "<<getAssociatedModule();return false; }
    if(!*result) { setCleanupFailure("original call still in flight");return false; }
    return true;
}
bool ModuleInlineHook::detach() {
    if(!prepareDetach()) return false;
    // No freeze, registry, state or viewer lock while callbacks finish. The
    // module lease and disabled hook keep the original callable until drained.
    if(callbacksDrained && !callbacksDrained()) {setCleanupFailure("grid callback still in flight");return false;}
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(mInlineHook && callbacksDrained) {
        const auto& trampoline=mInlineHook.trampoline();
        if(!callbackSize || hook_lifetime::quiescent({callbackBegin,callbackSize},{trampoline.address(),trampoline.size()})!=hook_lifetime::Result::Clear) {
            setCleanupFailure("grid callback return not yet quiescent");return false;
        }
    }
    mInlineHook={};setCleanupFailure("");
    return true;
}
bool ModuleMidHook::prepareDetach() {
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(mMidHook && !mMidHook.disable()) { setCleanupFailure("mid-hook disable failed");return false; }
    return true;
}
bool ModuleMidHook::detach() {
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(!mMidHook) return true;
    if(!mMidHook.disable()) { setCleanupFailure("mid-hook disable failed"); PLOG_ERROR<<"[HookCleanup] mid-hook disable failed: "<<getAssociatedModule();return false; }
    if(usesGameplayLifecycle()) {
        const auto& stub=mMidHook.stub_allocation();const auto& trampoline=mMidHook.trampoline_allocation();
        const auto state=hook_lifetime::quiescent({stub.address(),stub.size()},{trampoline.address(),trampoline.size()});
        if(state!=hook_lifetime::Result::Clear) {
            setCleanupFailure(state==hook_lifetime::Result::Busy?"mid-hook callback/stub return still in flight":"thread state unreadable during mid-hook retirement");return false;
        }
    }
    mMidHook={};setCleanupFailure("");
    return true;
}

namespace {
    bool copyPatchBytes(void* to,const void* from,size_t size) {
        __try { memcpy(to,from,size);return true; }
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    bool writePatchBytes(uintptr_t address,const byte* bytes,size_t size) {
        ScopedThreadSuspender pause;
        if(!pause.snapshotOk() || pause.missedCount()) return false;
        DWORD previous=0,unused=0;
        if(!VirtualProtect(reinterpret_cast<void*>(address),size,PAGE_EXECUTE_READWRITE,&previous)) return false;
        const bool copied=copyPatchBytes(reinterpret_cast<void*>(address),bytes,size);
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),size)!=FALSE;
        const bool protectedAgain=VirtualProtect(reinterpret_cast<void*>(address),size,previous,&unused)!=FALSE;
        return copied && flushed && protectedAgain;
    }
    uintptr_t moduleBase(const std::wstring& name) {
        return reinterpret_cast<uintptr_t>(GetModuleHandleW(name==L"main"?nullptr:name.c_str()));
    }
}
void ModulePatch::report(Outcome result,uintptr_t address) {
    setCleanupFailure(result==Outcome::Conflict?"patch byte conflict":
        (result==Outcome::ReadFailed || result==Outcome::WriteFailed || result==Outcome::VerifyFailed)?"patch restoration/readback failed":"");
    const auto before=outcome.exchange(result);
    if(before==result) return;
    const auto description=magic_enum::enum_name(result);
    const auto detail=std::format("[PatchCleanup] feature={} module={} base={:X} generation={} address={:X} rva={:X} wanted={} outcome={}",
        feature.empty()?"ModulePatch":feature,wstr_to_str(getAssociatedModule()),ownerBase,ownerGeneration,address,
        address>=ownerBase?address-ownerBase:0,getWantsToBeAttached(),description);
    if(result==Outcome::UnownedPatch || result==Outcome::Conflict || result==Outcome::ReadFailed || result==Outcome::WriteFailed || result==Outcome::VerifyFailed)
        PLOG_ERROR<<detail;
    else PLOG_INFO<<detail;
}
void ModulePatch::attach() {
    std::unique_lock stateLock(ModuleHookManager::h2StateMutex(),std::defer_lock);
    if(getAssociatedModule()==L"halo2.dll") stateLock.lock();
    const auto base=moduleBase(getAssociatedModule());
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(getAssociatedModule()==L"halo2.dll" && !ModuleHookManager::h2EffectsAllowed()) {report(Outcome::Deferred);return;}
    if(!base) {report(Outcome::ModuleAbsent);return;}
    const auto generation=ModuleCache::generation(getAssociatedModule());
    uintptr_t address=0;
    if(!mOriginalFunction->resolve(&address)) {report(Outcome::ReadFailed);return;}
    std::vector<byte> current(mPatchedBytes.size());
    if(!copyPatchBytes(current.data(),reinterpret_cast<void*>(address),current.size())) {report(Outcome::ReadFailed,address);return;}
    if(base!=ownerBase || generation!=ownerGeneration) {
        mOriginalBytes.clear();unownedPatch=false;ownerBase=base;ownerGeneration=generation;
    }
    if(mOriginalBytes.empty()) {
        if(current==mPatchedBytes) {unownedPatch=true;report(Outcome::UnownedPatch,address);return;}
        unownedPatch=false;mOriginalBytes=current;
    }
    if(current==mPatchedBytes) {report(Outcome::AlreadyApplied,address);return;}
    if(current!=mOriginalBytes) {report(Outcome::Conflict,address);return;}
    if(!writePatchBytes(address,mPatchedBytes.data(),mPatchedBytes.size())) {report(Outcome::WriteFailed,address);return;}
    if(!copyPatchBytes(current.data(),reinterpret_cast<void*>(address),current.size())) {report(Outcome::ReadFailed,address);return;}
    report(current==mPatchedBytes?Outcome::Applied:Outcome::VerifyFailed,address);
}
bool ModulePatch::detach() {
    const auto base=moduleBase(getAssociatedModule());
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(!base) {report(Outcome::ModuleAbsent);return true;}
    if(mOriginalBytes.empty()) {report(unownedPatch?Outcome::UnownedPatch:Outcome::NeverApplied);return true;}
    if(base!=ownerBase || ModuleCache::generation(getAssociatedModule())!=ownerGeneration) {report(Outcome::OwnerChanged);return true;}
    uintptr_t address=0;
    if(!mOriginalFunction->resolve(&address)) {report(Outcome::ReadFailed);return false;}
    std::vector<byte> current(mPatchedBytes.size());
    if(!copyPatchBytes(current.data(),reinterpret_cast<void*>(address),current.size())) {report(Outcome::ReadFailed,address);return false;}
    if(current==mOriginalBytes) {report(Outcome::AlreadyRestored,address);return true;}
    if(current!=mPatchedBytes) {report(Outcome::Conflict,address);return false;}
    if(!writePatchBytes(address,mOriginalBytes.data(),mOriginalBytes.size())) {report(Outcome::WriteFailed,address);return false;}
    if(!copyPatchBytes(current.data(),reinterpret_cast<void*>(address),current.size())) {report(Outcome::ReadFailed,address);return false;}
    if(current!=mOriginalBytes) {report(Outcome::VerifyFailed,address);return false;}
    report(Outcome::Restored,address);return true;
}

void ModuleHookBase::updateHookState()
{
	if (this->mWantsToBeAttached)
	{
		this->attach();
	}
	else
	{
		this->detach();
	}
}


// Getter/setter for mWantsToBeAttached
void ModuleHookBase::setWantsToBeAttached(bool value) {
    const bool changed=mWantsToBeAttached.exchange(value)!=value;
    // H2 physical changes have one owner, outside loader/render/state callbacks.
    if(changed && !usesGameplayLifecycle()) updateHookState();
}

bool ModuleHookBase::getWantsToBeAttached() const
{
	return this->mWantsToBeAttached;
}

// Gets a ref to the safetyhook object, mainly used for calling the original function from within the new function
safetyhook::InlineHook& ModuleInlineHook::getInlineHook()
{
	return this->mInlineHook;
}



bool ModuleInlineHook::isHookInstalled() const
{
	std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    return this->mInlineHook.operator bool() && this->mInlineHook.enabled();
}

bool ModuleMidHook::isHookInstalled() const
{
	std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    return this->mMidHook.operator bool() && this->mMidHook.enabled();
}

bool ModulePatch::isHookInstalled() const {
    const auto base=moduleBase(getAssociatedModule());
    std::lock_guard freezeLock(hcmThreadSuspensionMutex());
    if(!base || mOriginalBytes.empty() || base!=ownerBase || ModuleCache::generation(getAssociatedModule())!=ownerGeneration) return false;
    uintptr_t address=0;
    if(!mOriginalFunction->resolve(&address)) return false;
    std::vector<byte> current(mPatchedBytes.size());
    return copyPatchBytes(current.data(),reinterpret_cast<void*>(address),current.size()) && current==mPatchedBytes;
}
