#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

typedef wchar_t wchar;
typedef unsigned long uint32;

inline int ErrorBox(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int size = _vscprintf(format, args);
	char* text = new char[size + 1];
	size = vsprintf(text, format, args);
	va_end(args);
	MessageBox(NULL, text, "gxttotxt", MB_ICONERROR);
	delete[] text;
	return size;
}

int wmain(int argc, const wchar** argv)
{
	if (argc != 2)
	{
		ErrorBox("Specify one gxt file as argument.");
		return 0;
	}

	wchar* extension = PathFindExtensionW(argv[1]);
	if (_wcsicmp(extension, L".gxt") != 0)
	{
		ErrorBox("Specify one gxt file as argument.");
		return 0;
	}

	// Load gxt file in memory
	std::ifstream gxtFile(argv[1], std::ios::binary | std::ios::ate);
	if (!gxtFile)
	{
		ErrorBox("Couldn't open file.");
		return 0;
	}
	size_t gxtFileSize = gxtFile.tellg();
	gxtFile.seekg(0);
	char* gxtFileBuff = new char[gxtFileSize];
	gxtFile.read(gxtFileBuff, gxtFileSize);
	gxtFile.close();
	
	struct sMissionText
	{
		char name[8];
		uint32 offset;
	};

	char* TABLbuff = gxtFileBuff + 8;
	uint32& TABLsize = *reinterpret_cast<uint32*>(gxtFileBuff + 4);
	sMissionText* missionTexts = reinterpret_cast<sMissionText*>(TABLbuff);
	uint32 numMissionTexts = TABLsize / sizeof(sMissionText);

	struct sKey
	{
		uint32 offset;
		char name[8];
	};

	for (uint32 i = 0; i < numMissionTexts; ++i)
	{
		// open output txt file
		std::wofstream txtFile(std::string(missionTexts[i].name) + ".txt");

		// prepare the "sorted" vector of keys
		char* TKEYbuff = gxtFileBuff + missionTexts[i].offset + (i > 0 ? 12 : 4);
		uint32 TKEYsize = *reinterpret_cast<uint32*>(TKEYbuff);
		TKEYbuff += 4;
		sKey* keys = reinterpret_cast<sKey*>(TKEYbuff);
		std::vector<sKey> keyVector;
		for (uint32 j = 0; j < TKEYsize / sizeof(sKey); ++j)
			keyVector.push_back(keys[j]);
		struct { bool operator()(sKey key1, sKey key2) { return key1.offset < key2.offset; } } keysort;
		std::sort(keyVector.begin(), keyVector.end(), keysort);

		char* TDAT = TKEYbuff + TKEYsize + 8;

		// print to file
		for (sKey& key : keyVector)
		{
			txtFile
				<< '[' << key.name << ']' << std::endl
				<< reinterpret_cast<wchar*>(TDAT + key.offset) << std::endl
				<< std::endl;
		}
	}

	delete[] gxtFileBuff;

	return 0;
}