#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

int main(int argc, const char* argv[])
{
	// Ensure that there's only one argument; the path to language txt
	if (argc == 1)
	{
		std::cout
			<< "g3GXT - a simple GXT builder for GTAIII - by guard3" << std::endl
			<< "Usage: g3GXT [path_to_language_text_file]";
		return 0;
	}
	if (argc > 2)
	{
		std::cout << "Too many arguments.";
		return 1;
	}

	// Ensure that the file has a txt extension, so that it can be used as output filename
	std::string name(argv[1]);
	for (char& c : name) c = tolower(c);
	if (name.rfind(".txt") != name.length() - 4)
	{
		std::cout << "Specify a .txt file.";
		return 1;
	}
	else name.erase(name.length() - 4);

	// Open language txt
	std::wifstream languageTextFile(argv[1]);
	if (!languageTextFile)
	{
		std::cout << "Couldn't open file " << argv[1];
		return 1;
	}

	std::cout << "Parsing..." << std::endl;

	// Copy txt contents to a stringstream, excluding comments
	std::wstringstream languageTextStream;
	wchar_t character;
	bool comment = false;
	unsigned long long lineCounter = 1;
	while (languageTextFile.get(character))
	{
		if (character == L'\n')
		{
			languageTextStream << character;
			lineCounter++;
		}
		else
		{
			if (comment)
			{
				if (character == L'}') comment = false;
			}
			else
			{
				if (character == L'}')
				{
					std::cout << "Error in line " << lineCounter << ". Incomplete comment.\nCompilation failed.";
					return 1;
				}
				if (character == L'{') comment = true;
				else languageTextStream << character;
			}
		}
	}
	languageTextFile.close();


	// Store labels and text from stringstream to a vector and sort it alphabetically based on labels
	std::wstring line;

	struct Entry
	{
		std::string label = "";
		std::wstring text = L"";
	} entry;

	std::vector<Entry> entries;

	for (int i = 1; std::getline(languageTextStream, line); i++)
	{
		if (line.empty()) continue;
		while (iswspace(line[0])) line.erase(0, 1);
		if (line.empty()) continue;
		while (iswspace(line.back())) line.pop_back();
		if (line.empty()) continue;

		if (line.front() == L'[' && line.back() == L']')
		{
			line.erase(0, 1);
			line.pop_back();
			if (line.length() > 7)
			{
				std::cout << "Error in line " << i << ". Label must be up to 7 characters.\nCompilation failed.";
				return 1;
			}
			if (line.empty())
			{
				std::cout << "Error in line " << i << ". Label can't be empty.\nCompilation failed.";
				return 1;
			}

			std::string label = "";
			for (wchar_t& wc : line)
			{
				if (iswascii(wc)) label += wc;
				else
				{
					std::cout << "Error in line " << i << ". Label must contain only ASCII characters.\nCompilation failed.";
					return 1;
				}
			}
			
			entries.push_back(entry);
			entry.text = L"";
			entry.label = label;
		}
		else
		{
			if (entry.label.empty())
			{
				std::cout << "Error in line " << i << ". Missing label name.\nCompilation failed.";
				return 1;
			}
			entry.text += line;
		}
	}

	if (entries.size() < 3)
	{
		std::cout << "Text file must have at least 2 labels defined.\nCompilation failed.";
		return 1;
	}

	entries.erase(entries.begin());
	entries.pop_back();


	struct { bool operator() (Entry i, Entry j) { return i.label < j.label; } } labelComp;
	std::sort(entries.begin(), entries.end(), labelComp);

	//Removing blank text entries
	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].text.empty())
		{
			entries.erase(entries.begin() + i);
			i--;
		}
	}

	for (int i = 0; i < entries.size() - 1; i++)
	{
		if (entries[i].label == entries[i + 1].label)
		{
			std::cout << "Label \"" << entries[i].label << "\" defined more than once\nCompilation failed.";
			return 1;
		}
	}

	// Create output file
	std::cout << "Building..." << std::endl;

	std::ofstream outputFile(name + ".gxt", std::ios::binary);
	if (!outputFile)
	{
		std::cout << "Couldn't create output file.\nCompilation failed.";
		return 1;
	}
	const char *tkey = "TKEY";
	const char *tdat = "TDAT";
	unsigned int tkey_size = entries.size() * 12;
	unsigned int tdat_size = 0;

	outputFile.write(tkey, 4);
	outputFile.write(reinterpret_cast<const char*>(&tkey_size), 4);

	unsigned long long zero = 0;

	for (Entry& e : entries)
	{
		outputFile.write(reinterpret_cast<const char*>(&tdat_size), 4);
		outputFile.write(reinterpret_cast<const char*>(e.label.c_str()), e.label.length());
		outputFile.write(reinterpret_cast<const char*>(&zero), 8 - e.label.length());
		tdat_size += 2 * (e.text.length() + 1);
	}

	outputFile.write(reinterpret_cast<const char*>(tdat), 4);
	outputFile.write(reinterpret_cast<const char*>(&tdat_size), 4);

	for (Entry& e : entries)
		outputFile.write(reinterpret_cast<const char*>(e.text.c_str()), 2 * (e.text.length() + 1));

	outputFile.close();
	std::cout << "Done!";
	return 0;
}