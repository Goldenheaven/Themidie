#include "MinHook.h"
#include "hooks.hpp"

#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS

#include <winternl.h>
#include <ntstatus.h>

#define error(message) MessageBoxA(0, message, "Themidie", 16L);

BOOL APIENTRY DllMain(HINSTANCE__ *module, unsigned long call_reason, void *reserved)
{
	if (call_reason == 1)
	{
		LdrDisableThreadCalloutsForDll(module);

		if (MH_Initialize() != MH_OK)
		{
			error("Could not initialize hooks");
			return FALSE;
		}

		if (MH_CreateHook(&GetModuleHandleA, &gmha_hook, (void**)&og_gmha) != MH_OK)
		{
			error("Could not hook GetModuleHandleA");
			return FALSE;
		}

		if (MH_CreateHook(&FindWindowA, &fwa_hook, (void**)&og_fwa) != MH_OK)
		{
			error("Could not hook FindWindowA");
			return FALSE;
		}

		if (MH_CreateHook(&RegOpenKeyA, &roka_hook, (void**)&og_roka) != MH_OK)
		{
			error("Could not hook RegOpenKeyA");
			return FALSE;
		}

		if (MH_CreateHook(&RegQueryValueExA, &rqvea_hook, (void**)&og_rqvea) != MH_OK)
		{
			error("Could not hook RegQueryValueExA");
			return FALSE;
		}

		if (MH_CreateHook(&NtSetInformationThread, &nsit_hook, (void**)&og_nsit) != MH_OK)
		{
			error("Could not hook NtSetInformationThread");
			return FALSE;
		}

		if (MH_CreateHook(&NtQueryVirtualMemory, &nqvm_hook, (void**)&og_nqvm) != MH_OK)
		{
			error("Could not hook NtQueryVirtualMemory");
			return FALSE;
		}

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		{
			error("Could not enable hooks");
			return FALSE;
		}

	}

	return 1;
}
