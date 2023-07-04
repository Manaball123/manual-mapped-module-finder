#include <Windows.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <Psapi.h>
#include <iostream>
#include <filesystem>

namespace filesystem = std::filesystem;

#define NAME_LEN 256

#define PAGE_HAS_EXECUTE PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY
struct ModuleInfo
{
	MODULEINFO modInfo;
	char name[NAME_LEN];
	void PrintInfo()
	{
		std::cout << "Module Name: " << name << "\n";
		std::cout << "Base: 0x" << std::hex << modInfo.lpBaseOfDll << "\n";
		std::cout << "Size: " << std::dec << modInfo.SizeOfImage << " / 0x" << std::hex << modInfo.SizeOfImage << "\n";
		std::cout << "Entry: 0x" << std::hex << modInfo.EntryPoint << "\n\n\n";
	}
};





//using NameModuleMap = std::unordered_map<std::string, HMODULE>;
using vector_MemInfo = std::vector<MEMORY_BASIC_INFORMATION>;

//ik ambiguous naming n all that but cant think of better names sry :(

bool GetModuleInfo(HANDLE hProc, HMODULE hMod, ModuleInfo& modInfo)
{
	if (!GetModuleInformation(hProc, hMod, &(modInfo.modInfo), sizeof(MODULEINFO)))
		return false;
	if (GetModuleBaseNameA(hProc, hMod, (LPSTR) & (modInfo.name), NAME_LEN) == 0)
		return false;
	return true;

}
std::vector<ModuleInfo> GetModules(HANDLE hProc)
{
	uint32_t len;
	DWORD dw;
	EnumProcessModulesEx(hProc, NULL, 0, (LPDWORD)&len, LIST_MODULES_ALL);
	
	len /= sizeof(HMODULE);
	HMODULE* hMods = new HMODULE[len];
	BOOL a = EnumProcessModulesEx(hProc, hMods, len * sizeof(HMODULE), &dw, LIST_MODULES_ALL);
	std::vector<ModuleInfo> modInfo;
	modInfo.resize(len);
	for (uint32_t i = 0; i < len; i++)
	{
		if (!GetModuleInfo(hProc, hMods[i], modInfo[i]))
		{
			std::cout << "GRRRRRR" << std::endl;
		}
	}
	return modInfo;
}

std::wstring path;


bool WriteBuffer(LPCVOID buffer, size_t size, std::wstring fname_app)
{
	std::wstring fname = path;
	fname.append(L"\\");
	fname.append(fname_app);



	HANDLE hFile = CreateFileW(fname.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD bytesWritten;

	//write dumped memory to file
	if (!WriteFile(hFile, buffer, size, &bytesWritten, nullptr))
		return false;

	CloseHandle(hFile);
	return true;

}


int main()
{
	path = std::wstring(L"./") + std::to_wstring(static_cast<long>(time(0)));
	filesystem::create_directories(path);
	int a;
	std::vector<ModuleInfo> mods = GetModules(OpenProcess(PROCESS_ALL_ACCESS, FALSE, 54640));
	for (auto& v : mods)
	{
		v.PrintInfo();
	}
	std::cin >> a;
	//regions with execute flag
	vector_MemInfo memRegionsExecute;
	MEMORY_BASIC_INFORMATION curInfo;
	SIZE_T curBase = 0;
	
	while (VirtualQuery((LPCVOID)curBase, &curInfo, sizeof(MEMORY_BASIC_INFORMATION) != 0))
	{
		curBase = (SIZE_T)curInfo.AllocationBase;
		//if execute page
		if ((curInfo.Protect & PAGE_HAS_EXECUTE) != 0)
		{
			//add entry
			//please dont be pass by ref...
			memRegionsExecute.push_back(curInfo);
		}


	}

	//This only searches for regions that does not belong to any page for now
	//However there are 2 cases that are not accounted (for now):
	//a. manual mapped module is located right after a known module, with same protection
	//b. manual mapped module is BETWEEN two known modules, with same protection
	//first case can be easily detected by comparing the sizes of region/module.
	//second case needs more work tho(also unlikely to happen unless deliberately made to be like that)
	//oh yea u can also hide ur pages from the thingy iirc but im not gonna do anything remotely related to that...
	for (auto& v : memRegionsExecute)
	{
		//cross reference w/ the other thing
		bool found = false;
		for (auto& modinfo : mods)
		{
			if (modinfo.modInfo.lpBaseOfDll == v.AllocationBase)
			{

			}
		}
	}


}