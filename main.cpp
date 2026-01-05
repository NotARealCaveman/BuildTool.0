#pragma once
#include "Support.h"

using namespace Manifest_Utility;
using namespace ReignEngine;

struct TestObject
{
	MFstring _string{ "0" };
	MFu32 _u32{ 0 };
	MFchar _char{ '0' };
	EntryID _ID;
};

struct TestObjectExporter : public DatabaseExporter<TestObject>
{
	void ExportBinaryData(const TestObject& entry, std::ofstream& database) const
	{
		ExportString(entry._string, database);
		ExportObject(entry._u32, database);
		ExportObject(entry._char, database);
	}
};

struct TestObjectBuilder : public DatabaseBuilder<TestObject>
{
	TestObjectBuilder()
		: DatabaseBuilder<TestObject>{ std::make_unique<TestObjectExporter>() } 
	{}

	std::unique_ptr<Form> CreateEntryForm(const std::string& entryName, const TestObject& entryData, std::stack<std::unique_ptr<Form>>& formStack, std::unordered_map<EntryID, MFstring>& symbols)
	{
		std::unique_ptr<Form> result{ std::make_unique<Form>() };
		result->headerLine.lineName = "Test Object Builder";
		result->headerLine.textBuffer.buffer = BuildHeaderString(entryName);

		const MFstring stringString{ entryData._string };
		const MFstring u32String{ std::to_string(entryData._u32) };
		const MFstring charString{ entryData._char };

		CreateEntryLine(stringLine.index, stringLine.name, stringString, result);
		CreateEntryLine(u32Line.index, u32Line.name, u32String, result);
		CreateEntryLine(charLine.index, charLine.name, charString, result);
		const EntryID entryID{ BuildSymbolHash(entryName) };
		Form* formAddress{ result.get() };
		result->exitFunction = [this, &symbols, formAddress, entryName, entryID](const MFint32)->void
		{
			CreateEntry(*formAddress, entryName, entryID, symbols);
		};

		return result;
	}

	void CreateEntry(const Form& entryForm, const std::string& entryName, const EntryID entryID, std::unordered_map<EntryID, MFstring>& symbols)
	{
		TestObject testObject;

		const FlatMap<FormLine>& bodyLines{ entryForm.bodyLines };
		testObject._string = BuildString(bodyLines.at(stringLine.index).value());
		testObject._u32 = BuildU32(bodyLines.at(u32Line.index).value());
		const MFstring t{ bodyLines.at(charLine.index).value() };
		testObject._char = t.at(0);

		StoreObject(entryName, entryID, std::move(testObject), symbols);
	}

	MFsize EntrySize(const TestObject& entry)
	{
		return ComputeStringSize(entry._string) + ComputeObjectSize(entry._u32) + ComputeObjectSize(entry._char);
	}

private:
	static const BuildLineDetails stringLine;
	static const BuildLineDetails u32Line;
	static const BuildLineDetails charLine;
};
const BuildLineDetails TestObjectBuilder::stringLine{ .index{ 0 }, .name { "STRING LINE"} };
const BuildLineDetails TestObjectBuilder::u32Line{ .index{ 1 }, .name { "U32 LINE"} };
const BuildLineDetails TestObjectBuilder::charLine{ .index{ 2 }, .name { "CHAR LINE"} };

struct TestObjectImporter : public DatabaseImporter<TestObject>
{
	TestObject Construct(const SymbolHash importID, std::ifstream& database) const
	{
		TestObject result;

		ImportString(result._string, database);
		ImportObject(result._u32, database);
		ImportObject(result._char, database);
		result._ID = importID;

		return result;
	}
};

using DatabaseExportFunction = std::function<void(std::ofstream& out, std::streamoff& currentWritePosition)>;

template<typename T, DBHeader Header = DatabaseHeader>
DatabaseExportFunction MakeExportFunction(DatabaseBuilder<T>& builder/*, const DatabaseExporter<T>& exporter*/, Header& header)
{
	return [&builder, &header](std::ofstream& out, std::streamoff& currentWritePosition)
	{
		builder.ExportDatabase(header, out, currentWritePosition);
	};
}

struct MBDFile
{
	void ImportFile();
	void ExportFile();

	std::ifstream mbdIn;
	//export data
	std::ofstream mbdOut;
	MFsize payloadSize{ 0 };
	//order our databases via key 
	std::map<EntryKey, DatabaseExportFunction> exportMap;
	std::unordered_map<EntryID, MFstring> symbols;
};

void MBDFile::ExportFile()
{
	//TODO: OPEN MBD FOR EXPORTING
	const std::string path{ "Databases\\TestDatabase.mbd" };
	mbdOut.open(path, std::ios::out | std::ios::binary);
	assert(mbdOut.is_open());

	const MFu32 databaseCount{ static_cast<MFu32>(exportMap.size()) };
	DatabaseHeader dbHeader;
	dbHeader.entryCount = databaseCount;
	//compute file size
	const MFsize headerSize{ sizeof(dbHeader) };
	const MFsize tocSize{ dbHeader.entryCount * sizeof(EntryKey) };
	const MFsize totalSize{ headerSize + tocSize + payloadSize };
	assert(mbdOut.is_open());
	//allocate blank binary for random access writes 
	mbdOut.seekp(totalSize - 1);
	mbdOut.put('\0');//commit write	
	mbdOut.seekp(std::ios::beg);

	//export header	
	ExportObject(dbHeader, mbdOut);
	const std::streampos tableOfContents{ mbdOut.tellp() };
	MFu32 index{ 0 };
	std::streamoff currentWritePosition{ static_cast<std::streamoff>(sizeof(dbHeader) + tocSize) };
	//export quest and table of contents			
	for (auto [entryKey, exportFunction] : exportMap)
	{
		//export database TOC
		EntryKey writeKey{ entryKey };
		writeKey.entryPosition = currentWritePosition;
		const std::streampos keyPosition{ tableOfContents + static_cast<std::streamoff>(index * sizeof(EntryKey)) };
		mbdOut.seekp(keyPosition);
		ExportObject(writeKey, mbdOut);
		//export database data
		assert(exportMap.contains(writeKey));
		DatabaseExportFunction exportFunction{ exportMap.at(writeKey) };
		mbdOut.seekp(currentWritePosition);
		ForwardFunction(exportFunction, mbdOut, currentWritePosition);
		++index;
	}

	mbdOut.close();
}

static constexpr MFu64 DBID_1{ 1 }; 

int main()
{
	Console console{ 120,80 };
	HideConsoleCursor();
	const MFstring testIdentifier{ "test" };
	const EntryID testID{ testIdentifier.c_str() };
	TestObjectImporter testObjectImporter;
	TestObjectBuilder testObjectBuilder;
	DatabaseHeader header;
	MBDFile file;	
	file.exportMap.insert(std::make_pair(EntryKey{ .entryID{DBID_1},.entryName{"Database1"} }, MakeExportFunction(testObjectBuilder, header)));
	/*
	TestObject testObject;
	testObject._string = "string";
	testObject._ID = testID;
	testObject._u32 = 69;
	testObject._char = 'x';
	testObjectBuilder.StoreObject(testIdentifier, testID, std::move(testObject), symbols);
	*/
	TestObject testObject{ testObjectImporter.LoadEntry(testID,DBID_1) };
	testObjectBuilder.StoreObject(testIdentifier, testID, std::move(testObject), symbols);
	testObject = testObjectBuilder.entries.at(testID);

	formStack.push(std::move(testObjectBuilder.CreateEntryForm(testIdentifier, testObject, formStack, symbols)));
	formStack.top()->DisplayForm(console);

	while (!formStack.empty())
	{
		std::this_thread::sleep_for(std::chrono::microseconds(50));
		static auto lastTime{ std::chrono::steady_clock::now() };
		auto currentTime{ std::chrono::steady_clock::now() };
		if ((currentTime - lastTime) >= std::chrono::milliseconds{ 500 })
		{
			console.isCursorVisible = !console.isCursorVisible;
			lastTime = currentTime;
			formStack.top()->DisplayForm(console);
		}

		if (_kbhit())
		{
			InputCode inputCode;
			BUILD_KEY_CODES keyCode{ GetKey(inputCode) };

			const Form& currentForm{ *formStack.top() };
			assert(keyMap.contains(keyCode));
			ForwardFunction(keyMap.at(keyCode), inputCode);

			if (formStack.size())
				formStack.top()->DisplayForm(console);
		}
	}		
	file.payloadSize += testObjectBuilder.DatabaseSize<DatabaseHeader>();
	file.ExportFile();

	return 0;
}

int WINAPI WinMain(
	HINSTANCE hInstance,      // Handle to the current instance
	HINSTANCE hPrevInstance,  // Always NULL in modern Windows
	LPSTR     lpCmdLine,      // Command-line arguments as a single string
	int       nCmdShow        // How the window is to be shown
)
{
	main();
}
