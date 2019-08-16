#include <Windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>

typedef wchar_t wchar;

int ErrorBox(const char* format, ...)
{
	int result;
	va_list args;
	va_start(args, format);
	result = _vscprintf(format, args);
	char* textBuff = new char[result + 1];
	vsprintf(textBuff, format, args);
	va_end(args);
	MessageBox(NULL, textBuff, "g3GXT - VC", MB_ICONERROR);
	delete[] textBuff;
	return result;
}

int wmain(int argc, const wchar** argv)
{
	// Check that there's only one argument specified.
	if (argc != 2)
	{
		ErrorBox("Specify one folder containing the txt sources.");
		return 0;
	}

	// Get input folder name
	std::wstring inputFolder(argv[1]);
	switch (inputFolder.back())
	{
	case L'/': inputFolder.back() = L'\\'; break;
	default: inputFolder += L'\\';
	}
	inputFolder += L'*';

	// Get a list of all mission names from files in that folder
	std::vector<std::string> missionNames;
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(inputFolder.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		ErrorBox("Couldn't get file info in the specified folder.");
		return 0;
	}
	bool mainFound = false;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;
		size_t filenamelen = wcslen(ffd.cFileName);
		if (filenamelen < 5)
			continue;
		if (_wcsicmp(ffd.cFileName + filenamelen - 4, L".txt") != 0)
			continue;
		if (filenamelen > 11)
		{
			ErrorBox(".txt files must have a filename of max 7 latin characters, numbers or underscores");
			FindClose(hFind);
			return 0;
		}
		
		std::string filename;
		wchar* c;
		for (c = ffd.cFileName; *c != L'.'; ++c)
		{
			if (*c == L'_')
				filename += L'_';
			else if ((*c > 47 && *c < 58) || (*c > 64 && *c < 91))
				filename += *c;
			else if (*c > 96 && *c < 123)
				filename += *c - 32;
			else
			{
				ErrorBox(".txt files must have a filename of max 7 latin characters, numbers or underscores");
				FindClose(hFind);
				return 0;
			}
		}
		
		if (filename == "MAIN")
			mainFound = true;
		else
			missionNames.push_back(filename);
	} while (FindNextFileW(hFind, &ffd));
	if (!mainFound)
	{
		ErrorBox("MAIN.TXT wasn't found in the specified directory.");
		return 0;
	}
	struct { bool operator()(std::string s1, std::string s2) { return s1 < s2; } } comparator;
	std::sort(missionNames.begin(), missionNames.end(), comparator);
	missionNames.insert(missionNames.begin(), "MAIN");
	inputFolder.pop_back();

	// open output file
	std::ofstream gxtFile(inputFolder.substr(0, inputFolder.size() - 1) + L".gxt", std::ios::binary);
	if (!gxtFile)
	{
		ErrorBox("Could not create output .gxt file.");
		return 0;
	}

	gxtFile.write("TABL", 4);
	unsigned long TABLsize = missionNames.size() * 12;
	gxtFile.write(reinterpret_cast<const char*>(&TABLsize), 4);
	struct sTmp { char missionName[8]; unsigned long offset; }*tmp = new sTmp[missionNames.size()]();
	for (unsigned long i = 0; i < missionNames.size(); ++i)
		strcpy(reinterpret_cast<char*>(tmp[i].missionName), missionNames[i].c_str());
	gxtFile.write(reinterpret_cast<const char*>(tmp), TABLsize);
	delete[] tmp;

	unsigned long offset = 8 + TABLsize;
	std::vector<unsigned long> offsets;

	for (unsigned long i = 0; i < missionNames.size(); ++i)
	{
		// Open mission text file
		std::wifstream missionTextFile(inputFolder + std::wstring(missionNames[i].begin(), missionNames[i].end()) + L".txt");
		if (!missionTextFile)
		{
			ErrorBox("Could not open %s.txt", missionNames[i].c_str());
			return 0;
		}

		// Parse the file to remove comments
		std::wstringstream missionTextStream;
		unsigned long long lineCounter = 1;
		wchar wc;
		bool isComment = false;
		while (missionTextFile.get(wc))
		{
			if (wc == L'\n')
			{
				++lineCounter;
				missionTextStream << wc;
			}
			else
			{
				if (isComment)
				{
					if (wc == L'}') isComment = false;
				}
				else
				{
					if (wc == L'}')
					{
						ErrorBox("%s: Error in line %i\nUnexpected '}' character", missionNames[i].c_str(), lineCounter);
						return 0;
					}
					if (wc == L'{') isComment = true;
					else missionTextStream << wc;
				}
			}
		}
		if (isComment)
		{
			ErrorBox("%s: Error in line %i\nIncomplete comment.", missionNames[i].c_str(), lineCounter);
			return 0;
		}

		// Store labels and text to an alphabetically sorted vector
		struct sEntry
		{
			std::string label;
			std::wstring text;
		} entry;
		std::vector<sEntry> entries;
		std::wstring line;
		for (unsigned long long j = 1; std::getline(missionTextStream, line); ++j)
		{
			// Remove whitespaces
			/*
			if (line.empty())
				continue;
			while (iswspace(line.front()))
			{
				line.erase(0, 1);
				if (line.empty())
					continue;
			}
			while (iswspace(line.back()))
				line.pop_back();*/
			if (line.empty()) continue;
			while (iswspace(line[0])) line.erase(0, 1);
			if (line.empty()) continue;
			while (iswspace(line.back())) line.pop_back();

			// Add information to current entry
			if (line.front() == L'[')
			{
				if (line.back() != L']')
				{
					ErrorBox("%s: Error in line %i.\nLabel name must be inside [brakets].", missionNames[i].c_str(), j);
					return 0;
				}
				line = line.substr(1, line.length() - 2);
				if (line.empty())
				{
					ErrorBox("%s: Error in line %i.\nLabel cannot be empty.", missionNames[i].c_str(), j);
					return 0;
				}
				while (iswspace(line.front()))
				{
					line.erase(0, 1);
					if (line.empty())
					{
						ErrorBox("%s: Error in line %i.\nLabel cannot be empty.", missionNames[i].c_str(), j);
						return 0;
					}
				}
				while (iswspace(line.back()))
					line.pop_back();
				if (line.length() > 7)
				{
					ErrorBox("%s: Error in line %i.\nLabel name must have max 7 latin characters, numbers or underscores.", missionNames[i].c_str(), j);
					return 0;
				}
				if (!entry.label.empty())
				{
					ErrorBox("%s: Error in line %i.\nLabel has no text.", missionNames[i].c_str(), j);
					return 0;
				}
				for (wchar& c : line)
				{
					if (c == L'_')
						entry.label += '_';
					else if ((c > 47 && c < 58) || (c > 64 && c < 91))
						entry.label += c;
					else if (c > 96 && c < 123)
						entry.label += c - 32;
					else
					{
						ErrorBox("%s: Error in line %i.\nLabel name must have max 7 latin characters, numbers or underscores.", missionNames[i].c_str(), j);
						FindClose(hFind);
						return 0;
					}
				}
				entry.text = std::wstring();
			}
			else
			{
				if (entry.label.empty())
				{
					ErrorBox("%s: Error in line %i.\nMissing label name.", missionNames[i].c_str(), j);
					return 0;
				}
				if (entry.text.empty())
				{
					entry.text = line;
					entries.push_back(entry);
					entry = { std::string(), std::wstring() };
				}
				else
				{
					ErrorBox("%s: Error in line %i.\nText for label [%s] is already specified.", missionNames[i].c_str(), j, entry.label.c_str());
					return 0;
				}
			}
		}

		// Sort labels
		if (entries.size() == 0)
		{
			ErrorBox("%s: Error.\nFile is empty.", missionNames[i].c_str());
			return 0;
		}
		if (entries.size() > 1)
		{
			struct { bool operator()(sEntry e1, sEntry e2) { return e1.label < e2.label; } } comparator2;
			std::sort(entries.begin(), entries.end(), comparator2);
			for (unsigned long k = 1; k < entries.size(); ++k)
			{
				if (entries[k].label == entries[k - 1].label)
				{
					ErrorBox("Label [%s] was defined more than once.", entries[k].label.c_str());
					return 0;
				}
			}
		}

		// Align in file
		unsigned long zero = 0;
		if (offset & 3)
		{
			gxtFile.write(reinterpret_cast<const char*>(&zero), 4 - (offset & 3));
			offset = (offset & 0xFFFFFFFC) + 4;
		}
		if (i > 0)
		{
			char tmp[8]{ 0 };
			strcpy(tmp, missionNames[i].c_str());
			gxtFile.write(tmp, 8);
		}

		gxtFile.write("TKEY", 4);
		unsigned long TKEYsize = entries.size() * 12;
		unsigned long TDATsize = 0;
		gxtFile.write(reinterpret_cast<const char*>(&TKEYsize), 4);
		for (sEntry& e : entries)
		{
			gxtFile.write(reinterpret_cast<const char*>(&TDATsize), 4);
			char labelName[8]{ 0 };
			strcpy(labelName, e.label.c_str());
			gxtFile.write(labelName, 8);
			TDATsize += 2 * (e.text.length() + 1);
		}

		gxtFile.write("TDAT", 4);
		gxtFile.write(reinterpret_cast<const char*>(&TDATsize), 4);
		for (sEntry& e : entries)
			gxtFile.write(reinterpret_cast<const char*>(e.text.c_str()), 2 * (e.text.length() + 1));

		offsets.push_back(offset);

		offset += 8 + TKEYsize + 8 + TDATsize + (i > 0 ? 8 : 0);
	}

	// Add offsets to mission texts
	for (unsigned long i = 0; i < offsets.size(); ++i)
	{
		gxtFile.seekp(16 + 12 * i, std::ios::beg);
		gxtFile.write(reinterpret_cast<const char*>(&offsets[i]), 4);
	}

	gxtFile.close();

	return 0;
}