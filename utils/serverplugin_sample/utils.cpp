#include "utils.h"
#include <windows.h>

void SwapVTableEntry(void* pObject, int vtableIndex, void* newFunction)
{
	void** pVTable = *(void***)pObject;
	DWORD oldProtect;
	VirtualProtect(&pVTable[vtableIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	pVTable[vtableIndex] = newFunction;
	VirtualProtect(&pVTable[vtableIndex], sizeof(void*), oldProtect, &oldProtect);
}