/*
* MIT License
*
* Copyright (c) 2023 nastys
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <intrin.h>
#include <detours.h>

void InjectCode(void* address, const std::vector<uint8_t> data)
{
    const size_t byteCount = data.size() * sizeof(uint8_t);

    DWORD oldProtect;
    VirtualProtect(address, byteCount, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, data.data(), byteCount);
    VirtualProtect(address, byteCount, oldProtect, nullptr);
}

static auto Original_j_ChangeGameSubState = reinterpret_cast<void(__fastcall*)(int, int)>(0x00000001402c48b0);

bool console = false;

void __fastcall Hooked_j_ChangeGameSubState(int state, int substate)
{
    if (console) printf("[IntroPatch] state %i, substate %i; return address %p\n", state, substate, _ReturnAddress());

    if (state == 12 && substate == 3)
    {
        Hooked_j_ChangeGameSubState(9, 47);
        return;
    }

    Original_j_ChangeGameSubState(state, substate);
}

bool patchAdvLogo()
{
    if (console) printf("[IntroPatch] Patching ADV_LOGO...\n");
    
    if (*((BYTE*)0x14EBFFE7B) != 0x75)
    {
        if (console) printf("[IntroPatch] E: Unsupported game version.\n");
        return false;
    }

    InjectCode((void*)0x14EBFFE7B, { 0xEB });
    if (console) printf("[IntroPatch] ADV_LOGO patched.\n");

    return true;
}

bool hooker()
{
    if (console) printf("[IntroPatch] Hooking j_ChangeGameSubState...\n");
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)Original_j_ChangeGameSubState, (PVOID)Hooked_j_ChangeGameSubState);
    DetourTransactionCommit();
    if (console) printf("[IntroPatch] j_ChangeGameSubState hooked.\n");

    return true;
}

extern "C" 
{
    __declspec(dllexport) void Init() {
        console = freopen("CONOUT$", "w", stdout) != NULL;

        if (console) printf("[IntroPatch] Initializing...\n");

        if (patchAdvLogo() && hooker())
        {
            if (console) printf("[IntroPatch] Initialized.\n");
        }

    }
}
