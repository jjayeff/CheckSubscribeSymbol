// autodelete.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "processor.h"

Processor processor;

int CheckSubscribeSymbol() {
	// Run Program;
	if (processor.Run())
		return 1;

	string file = processor.file_path + processor.front_name + "-" + processor.back_name;

	string in_file = file + ".in";
	string out_file = file + ".out";

	// Read file .in .out get value
	if (processor.ReadFile(out_file))
		return 1;
	if (processor.ReadFile(in_file))
		return 1;
	// Connecting Database
	if (processor.ConnectDataBase())
		return 1;

	// Get symbol from database
	char cmd_temp[1024] = { 0 };
	// Get symbol from database acc_info
	sprintf_s(cmd_temp,
		"SELECT symbol FROM acc_info.dbo.security_def WHERE trading_date >= '%s'", processor.trading_date);
	if (processor.GetSymbolBase(cmd_temp, 1))
		return 1;
	sprintf_s(cmd_temp,
		"SELECT symbol FROM acc_info_stock.dbo.security_def WHERE trading_date >= '%s'", processor.trading_date);
	if (processor.GetSymbolBase(cmd_temp))
		return 1;

	processor.CheckSymbol();

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2) {
		LOGI << "Usage: " << argv[0] << " [Check = 1],[Change = 2]";
		return 1;
	}

	switch (stoi(argv[1])) {
	case 1:
		if (CheckSubscribeSymbol())
			return 1;
	case 2: {
	}
	}

	return 0;
}