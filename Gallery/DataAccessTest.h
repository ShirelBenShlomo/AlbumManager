#pragma once

#include "DatabaseAccess.h"

class DataAccessTest
{
public:
	static void AddingRecords(DatabaseAccess database);
	static void updatingRecords(DatabaseAccess database);
	static void DeletingRecords(DatabaseAccess database);
	
};