// https://wiki.alliedmods.net/Signature_scanning
// slightly modified by me

#include <stdio.h>
 
#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <psapi.h>
#else
    #include <dlfcn.h>
    #include <sys/types.h>
    #include <sys/stat.h> 
#endif
 
#include "sigscan.h"
 
/* There is no ANSI ustrncpy */
unsigned char* ustrncpy(unsigned char *dest, const unsigned char *src, int len) {
    while(len--)
        dest[len] = src[len];
 
    return dest;
}
 
/* //////////////////////////////////////
    CSigScan Class
    ////////////////////////////////////// */
unsigned char* CSigScan::base_addr;
size_t CSigScan::base_len;
int CSigScan::module_handle;
void *(*CSigScan::sigscan_dllfunc)(const char *pName, int *pReturnCode);
 
/* Initialize the Signature Object */
void CSigScan::Init(unsigned char *sig, char *mask, size_t len) {
    is_set = 0;
 
    sig_len = len;
    sig_str = new unsigned char[sig_len];
    ustrncpy(sig_str, sig, sig_len);
 
    sig_mask = new char[sig_len+1];
    strncpy(sig_mask, mask, sig_len);
    sig_mask[sig_len+1] = 0;
 
    if(!base_addr)
        return ; // GetDllMemInfo() Failed
 
    if((sig_addr = FindSignature()) == NULL)
        return ; // FindSignature() Failed
 
    is_set = 1;
    // SigScan Successful!
}
 
/* Destructor frees sig-string allocated memory */
CSigScan::~CSigScan(void) {
    delete[] sig_str;
    delete[] sig_mask;
}
 
/* Get base address of the server module (base_addr) and get its ending offset (base_len) */
bool CSigScan::SetDllMemInfo(const char* dllName) {
    void *pAddr = (void*)sigscan_dllfunc;
    base_addr = 0;
    base_len = 0;
 
    #ifdef WIN32
    HMODULE _module_handle = GetModuleHandle(dllName);
	if (!_module_handle)
		return false;

	MODULEINFO modinfo;
	if (!GetModuleInformation(GetCurrentProcess(), _module_handle, &modinfo, sizeof(modinfo)))
		return false;
	module_handle = (int)_module_handle;
	base_addr = (unsigned char*)modinfo.lpBaseOfDll;
	base_len = modinfo.SizeOfImage;

 
    #else
    #error i don't know linux dll info stuff
    
    Dl_info info;
    struct stat buf;
 
    if(!dladdr(pAddr, &info))
        return false;
 
    if(!info.dli_fbase || !info.dli_fname)
        return false;
 
    if(stat(info.dli_fname, &buf) != 0)
        return false;
 
    base_addr = (unsigned char*)info.dli_fbase;
    base_len = buf.st_size;
    #endif
 
    return true;
}
 
/* Scan for the signature in memory then return the starting position's address */
void* CSigScan::FindSignature(void) {
    unsigned char *pBasePtr = base_addr;
    unsigned char *pEndPtr = base_addr+base_len;
    size_t i;
 
    while(pBasePtr < pEndPtr) {
        for(i = 0;i < sig_len;i++) {
            if((sig_mask[i] != '?') && (sig_str[i] != pBasePtr[i]))
                break;
        }
 
        // If 'i' reached the end, we know we have a match!
        if(i == sig_len)
            return (void*)pBasePtr;
 
        pBasePtr++;
    }
 
    return NULL;
}

bool CSigScan::MemSet(void* addr, int val, size_t len) {
#ifdef WIN32
	DWORD oldProtect;
    VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset(addr, val, len);
	VirtualProtect(addr, len, oldProtect, &oldProtect);
	return true;
#else
    #error check this code, could be gibberish
	return (mprotect((void*)((size_t)addr & ~0xFFF), len + ((size_t)addr - (size_t)addr & 0xFFF), PROT_READ | PROT_WRITE | PROT_EXEC) == 0) && (memset(addr, val, len) != NULL);
#endif
}

int CSigScan::GetProcAddress(const char* func) {
#ifdef WIN32
	return (int)::GetProcAddress((HMODULE)module_handle, func);
#else
	return (int)dlsym((void*)module_handle, func);
#endif
}