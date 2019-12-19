// Modloader.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "Modloader.h"
#include "logger.h"
#include "assert_util.h"
#include <exception>

// This function is used to load in all of the mods into the main assembly.
// It will construct them, create a hook into il2cpp_init to use
// and load the mods in il2cpp_init.
MODLOADER_API int load(void)
{
	SetCurrentDirectory(L"BONEWORKS");
	if (!CreateDirectory(L"Logs/", NULL) && ERROR_ALREADY_EXISTS != GetLastError())
	{
		MessageBoxW(NULL, L"Could not create Logs/ directory!", L"Could not create Logs directory!'",
			MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);
		ExitProcess(GetLastError());
	}

	init_logger(L"modloader");

	if (!CreateDirectory(L"Mods/", NULL) && ERROR_ALREADY_EXISTS != GetLastError())
	{
		LOG("FAILED TO CREATE MODS DIRECTORY!\n");
		MessageBoxW(NULL, L"Could not create Mods/ directory!", L"Could not create Mods directory!'",
			MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);
		free_logger();
		ExitProcess(GetLastError());
	}

	WIN32_FIND_DATAW findDataAssemb;
	LOG("Attempting to find GameAssembly.dll\n");
	HANDLE findHandleAssemb = FindFirstFileW(L"GameAssembly.dll", &findDataAssemb);
	if (findHandleAssemb == INVALID_HANDLE_VALUE)
	{
		LOG("FAILED TO FIND GameAssembly.dll!\n");
		free_logger();
		MessageBoxW(NULL, L"Could not locate game being injected!", L"Could not find GameAssembly.dll'",
			MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);
		ExitProcess(GetLastError());
		return 0;
	}
	auto gameassemb = LoadLibraryA("GameAssembly.dll");
	ASSERT(gameassemb, L"GameAssembly.dll failed to load!");
	//auto attemptProc = GetProcAddress(gameassemb, "il2cpp_string_new");
	//auto base = (uint64_t)gameassemb;
	//LOG("GameAssembly.dll base: %x\n", base);
	//LOG("il2cpp_string_new proc address: %p\n", attemptProc);
	//LOG("il2cpp_string_new RVA: %x\n", (uint64_t)attemptProc - base);
	//LOG("Attempting to patch il2cpp_string_new...\n");

	//PLH::CapstoneDisassembler dis(PLH::Mode::x64);
	//PLH::x64Detour detour((uint64_t)attemptProc, (uint64_t)test_il2cpp_string_new, &il2cpp_string_new_orig_offset, dis);
	//detour.hook();

	////il2cpp_string_new_orig = patch_raw(gameassemb, (DWORD)attemptProc - base, test_il2cpp_string_new, 0x40);
	//LOG("Patched il2cpp_string_new!\n");

	WIN32_FIND_DATAW findData;
	LOG("Attempting to find all files that match: 'Mods/*.dll'\n");
	HANDLE findHandle = FindFirstFileW(L"Mods/*.dll", &findData);

	if (findHandle == INVALID_HANDLE_VALUE)
	{
		LOG("FAILED TO FIND ANY MODS TO LOAD!\n");
		//free_logger();
		return 0;
	}


	do
	{
		// Maybe we can just call LoadLibrary on this file, and GetProcAddress of load and call that?
			// 240 = max path length
		wchar_t path[240] = { 0 };
		wcscpy_s(path, L"Mods/");
		wcscat_s(path, findData.cFileName);
		LOG("%ls: Loading\n", path);
		auto lib = LoadLibrary(path);
		if (!lib) {
			LOG("%ls: Failed to find library!\n", path);
			continue;
		}
		auto loadCall = GetProcAddress(lib, "load");
		if (!loadCall) {
			LOG("%ls: Failed to find load call!\n", path);
			continue;
		}
		LOG("%ls: Calling 'load' function!\n", path);
		try {
			auto func = reinterpret_cast<function_ptr_t<int, HANDLE, HMODULE>>(loadCall);
			LOG("%ls: Calling load with function pointer: %p\n", path, func);
			if (func) {
				// Initialize a unique log for this mod
				auto log = make_logger(findData.cFileName);
				LOG("%ls: Created local log with pointer: %p\n", path, log);
				func(log, gameassemb);
			}
		}
		catch (int s) {
			LOG("%ls: FAILED TO CALL 'load' FUNCTION!\n", path);
		}
		LOG("%ls: Loaded!\n", path);
	} while (FindNextFileW(findHandle, &findData) != 0);

	LOG("Loaded all mods!\n");

	//free_logger();
	return 0;
}
