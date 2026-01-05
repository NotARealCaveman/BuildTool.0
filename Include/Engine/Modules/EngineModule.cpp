#include "EngineModule.h"

using namespace ReignEngine;


MFbool ReignEngine::FindDatabaseEntry(const SymbolHash databaseID, const std::streamoff headerSize, std::ifstream& database)
{
	//database.seekg(std::ios::beg);	
	//read just base Sheader data
	DatabaseHeader databaseHeader;
	const std::streampos currentPos{ database.tellg() };
	database.read(reinterpret_cast<char*>(&databaseHeader), sizeof(databaseHeader));
	//move to actual header offset
	database.seekg(currentPos + headerSize);
	//read in ToC to find entry record	
	std::vector<EntryKey> databaseKeys(databaseHeader.entryCount);
	database.read(reinterpret_cast<char*>(databaseKeys.data()), databaseHeader.entryCount * sizeof(EntryKey));
	//binary search over entry keys(L<G)
	const auto entryKey{ std::lower_bound(databaseKeys.begin(),	databaseKeys.end(), databaseID, [](const EntryKey& databaseEntry, const SymbolHash& databaseID)
	{
		return databaseEntry.entryID < databaseID; })
	};
	if (entryKey == databaseKeys.end() || entryKey->entryID != databaseID)
		return false;
	//move database to combat attributes
	database.seekg(entryKey->entryPosition);

	return true;
}