#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

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
	MessageBoxA(NULL, textBuff, "g3GXT - III", MB_ICONERROR);
	delete[] textBuff;
	return result;
}

int wmain(int argc, const wchar** argv)
{
	// Ensure that there's only one argument; the path to language txt
	if (argc != 2)
	{
		ErrorBox("Specify a .txt file.");
		return 0;
	}

	// Ensure that the file has a txt extension, so that it can be used as output filename
	std::wstring inputName(argv[1]);
	if (inputName.length() < 5)
	{
		ErrorBox("Specify a .txt file.");
		return 0;
	}
	if (_wcsicmp(inputName.c_str() + inputName.length() - 4, L".txt") != 0)
	{
		ErrorBox("Specify a .txt file.");
		return 0;
	}
	
	// Open language txt
	std::wifstream languageTextFile(argv[1]);
	if (!languageTextFile)
	{
		ErrorBox("Could not open input .txt file.");
		return 0;
	}

	// Copy txt contents to a stringstream, excluding comments
	std::wstringstream languageTextStream;
	wchar wc;
	bool isComment = false;
	unsigned long long lineCounter = 1;
	unsigned long long lastCommentLine;
	while (languageTextFile.get(wc))
	{
		if (wc == L'\n')
		{
			languageTextStream << wc;
			lineCounter++;
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
					ErrorBox("Error in line %i.\nUnexpected '}' character.", lineCounter);
					return 0;
				}
				if (wc == L'{')
				{
					isComment = true;
					lastCommentLine = lineCounter;
				}
				else
					languageTextStream << wc;
			}
		}
	}
	if (isComment)
	{
		ErrorBox("Error in line %i.\nIncomplete comment.", lastCommentLine);
		return 0;
	}
	languageTextFile.close();

	// Store labels and text from stringstream to a vector and sort it alphabetically based on labels
	struct sEntry
	{
		std::string label;
		std::wstring text;
	} entry;
	std::vector<sEntry> entries;
	std::wstring line;
	for (unsigned long long i = 1; std::getline(languageTextStream, line); ++i)
	{
		if (line.empty()) continue;
		while (iswspace(line[0])) line.erase(0, 1);
		if (line.empty()) continue;
		while (iswspace(line.back())) line.pop_back();

		// Add info to current entry
		if (line.front() == L'[')
		{
			// store previous entry if complete
			if (!entry.label.empty() && !entry.text.empty())
			{
				entries.push_back(entry);
				entry = { std::string(), std::wstring() };
			}

			if (line.back() != L']')
			{
				ErrorBox("Error in line %i.\nLabel name must be inside [brakets].", i);
				return 0;
			}
			line = line.substr(1, line.length() - 2);
			if (line.empty())
			{
				ErrorBox("Error in line %i.\nLabel cannot be empty.", i);
				return 0;
			}
			while (iswspace(line.front()))
			{
				line.erase(0, 1);
				if (line.empty())
				{
					ErrorBox("Error in line %i.\nLabel cannot be empty.", i);
					return 0;
				}
			}
			while (iswspace(line.back()))
				line.pop_back();

			if (line.length() > 7)
			{
				ErrorBox("Error in line %i.\nLabel name must have max 7 latin characters, numbers or underscores.", i);
				return 0;
			}
			if (!entry.label.empty())
			{
				ErrorBox("Error in line %i.\nLabel has no text.", i);
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
					ErrorBox("Error in line %i.\nLabel name must have max 7 latin characters, numbers or underscores.", i);
					return 0;
				}
			}
			entry.text = std::wstring();
		}
		else
		{
			if (entry.label.empty())
			{
				ErrorBox("Error in line %i.\nMissing label name.", i);
				return 0;
			}
			if (!entry.text.empty())
				entry.text += L"\n";
			entry.text += line;
		}
	}

	if (entries.size() == 0)
	{
		ErrorBox("Error.\nFile is empty.");
		return 0;
	}
	if (entries.size() > 1)
	{
		struct { bool operator()(sEntry e1, sEntry e2) { return e1.label < e2.label; } } comparator;
		std::sort(entries.begin(), entries.end(), comparator);
		for (unsigned long k = 1; k < entries.size(); ++k)
		{
			if (entries[k].label == entries[k - 1].label)
			{
				ErrorBox("Label [%s] was defined more than once.", entries[k].label.c_str());
				return 0;
			}
		}
	}

	// Write to file
	std::ofstream gxtFile(inputName.substr(0, inputName.length() - 3) + L"gxt", std::ios::binary);
	if (!gxtFile)
	{
		ErrorBox("Could not create output .gxt file.");
		return 0;
	}

	unsigned long TKEYsize = entries.size() * 12;
	unsigned long TDATsize = 0;

	gxtFile.write("TKEY", 4);
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

	gxtFile.close();
	return 0;
}