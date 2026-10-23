#include <windows.h>
extern "C" {
__declspec(dllexport) volatile LONG originalEntered=0,originalProceed=0;
__declspec(dllexport) unsigned char patchSite[8]={1,2,3,4,5,6,7,8};
__declspec(dllexport) unsigned char secondSite[8]={8,7,6,5,4,3,2,1};
__declspec(dllexport) __declspec(noinline) int fixtureOriginal(int value) {
    volatile int work=value;
    for(int i=0;i<17;++i) work=work+1;
    return work;
}
__declspec(dllexport) __declspec(noinline) int fixtureBlockingOriginal(int value) {
    InterlockedExchange(&originalEntered,1);
    while(!InterlockedCompareExchange(&originalProceed,0,0)) Sleep(1);
    return fixtureOriginal(value);
}
}
