#include <Windows.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <Psapi.h>
#include <iostream>
#include <filesystem>

namespace filesystem = std::filesystem;

#define NAME_LEN 256

#define PAGE_HAS_EXECUTE (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
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



bool CheckAddress(LPVOID address, LPVOID base, SIZE_T size)
{
	// no comment :/
	if ((address >= base) && (address < (LPVOID)((size_t)base + size)))
		return true;
	return false;
	
}



template <class T>
inline std::string ToHex(T v, size_t hex_len = sizeof(T) << 1)
{

	static const char* digits = "0123456789ABCDEF";
	std::string res(hex_len + 2, '0');
	const char* str_pref = "0x";
	memcpy((void*)res.c_str(), str_pref, 2);

	for (size_t i = 2, j = (hex_len - 1) * 4; i < hex_len + 2; i++, j -= 4)
		res[i] = digits[(v >> j) & 0x0f];

	return res;

}


//courtesy 2 dude on stackoverflow
inline std::wstring ToWstring(std::string s)
{
	size_t len = s.size();
	std::wstring ws(len, ' ');
	MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, s.c_str(), len, (LPWSTR)ws.c_str(), len);
	return ws;
}

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


bool DumpBuffer(HANDLE hProc, LPVOID address, size_t size)
{


	std::wstring fname = path;
	fname.append(L"\\");
	fname.append(ToWstring(ToHex((SIZE_T)address)) + L".bin");
	char* buf = new char[size];

	SIZE_T st;
	ReadProcessMemory(hProc, address, buf, size, &st);


	


	HANDLE hFile = CreateFileW(fname.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD bytesWritten;

	//write dumped memory to file
	if (!WriteFile(hFile, buf, size, &bytesWritten, nullptr))
		return false;

	CloseHandle(hFile);
	//boo hoo leaky memory
	delete[] buf;
	return true;

}


int main()
{
	path = std::wstring(L"./") + std::to_wstring(static_cast<long>(time(0)));
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, 49500);
	filesystem::create_directories(path);
	int a;
	std::vector<ModuleInfo> mods = GetModules(hProc);
	for (auto& v : mods)
	{
		v.PrintInfo();
	}

	//regions with execute flag
	vector_MemInfo memRegionsExecute;
	MEMORY_BASIC_INFORMATION curInfo;
	SIZE_T curBase = 0;
	
	while (VirtualQueryEx(hProc, (LPCVOID)curBase, &curInfo, sizeof(MEMORY_BASIC_INFORMATION)) != 0)
	{
		if (curBase > (SIZE_T)curInfo.BaseAddress)
			break;
		curBase = (SIZE_T)curInfo.BaseAddress;
		//if execute page

		if ((curInfo.Protect & PAGE_HAS_EXECUTE) != 0)
		{
			//add entry
			//please dont be pass by ref...
			memRegionsExecute.push_back(curInfo);
		}
		curBase += curInfo.RegionSize;

	}

	//This only searches for regions that does not belong to any page for now
	//However there are 2 cases that are not accounted (for now):
	//b. manual mapped module is BETWEEN two known modules, with same protection
	//second case needs more work tho(also unlikely to happen unless deliberately made to be like that)
	//oh yea u can also hide ur pages from the thingy iirc but im not gonna do anything remotely related to that...

	//Deleted the first case cuz i think u actually need 2 scan for page ranges anyway
	//now i have to do ranges stuff now...
	//THE USACO NIGHTMARES ARE BACK AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA


	//okok tree data stucture kind of
	//except the nodes are ranges now so i ned 2 write custom class 
	//ugh

	//actually no im just gonna bruteforce the range
	//its like 4am now im not dealing wiith this shit





	
	for (auto& v : memRegionsExecute)
	{
		//cross reference w/ the other thing
		bool found = false;
		for (auto& modinfo : mods)
		{
			if (CheckAddress(v.BaseAddress, modinfo.modInfo.lpBaseOfDll, modinfo.modInfo.SizeOfImage))
			{
				found = true;
				//std::cout << "Found matching alloc base\n";
				break;
			}

			
		};

		if (found)
		{
			continue;
		}
		else
		{
			//maybe consider dumping the region? :pleading_face:
			std::cout << "Found region not associated with a module!\n";
			std::cout << "Address: 0x" << std::hex << v.BaseAddress << "\n\n";
			std::cout << "Size: " << std::dec << v.RegionSize << " / 0x" << std::hex << v.RegionSize << "\n\n\n";
			DumpBuffer(hProc, v.BaseAddress, v.RegionSize);

		}
	}
	std::cout << "Press enter to terminate...";
	std::cin >> a;

}