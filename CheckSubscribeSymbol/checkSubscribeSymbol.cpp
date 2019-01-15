// autodelete.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "processor.h"

Processor processor;

int main()
{
	string file = processor.front_name + "-" + processor.back_name;
	string in_file = file + ".in";
	string out_file = file + ".out";

	LOGI << "Read file " << file;
	// Read file .in .out get value
	if (processor.ReadFile(out_file))
		LOGE << "Cannot read " << out_file;
	if (processor.ReadFile(in_file))
		LOGE << "Cannot read " << in_file;

	// Get symbol from database
	char cmd_temp[1024] = { 0 };
	// Get symbol from database acc_info
	sprintf_s(cmd_temp,
		"SELECT symbol FROM acc_info.dbo.security_def WHERE trading_date >= '%s'", processor.trading_date);
	processor.GetSymbolBase(cmd_temp, 1);
	sprintf_s(cmd_temp,
		"SELECT symbol FROM acc_info_stock.dbo.security_def WHERE trading_date >= '%s'", processor.trading_date);
	processor.GetSymbolBase(cmd_temp);

	processor.CheckSymbol();

	string filename = "./";
	struct stat result;
	if (stat(filename.c_str(), &result) == 0)
	{
		auto mod_time = result.st_mtime;
		struct tm lt;
		localtime_s(&lt, &mod_time);
		char timbuf[80];
		strftime(timbuf, sizeof(timbuf), "%c", &lt);
	}

	return 0;
}