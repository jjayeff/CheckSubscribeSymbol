#pragma once
#include "stdafx.h"
#include <DBAPI.h>
#include <plog/vnlog.h>

class Processor
{
public:
	Processor();
	~Processor();

public:
	struct SOut
	{
		string				msg_type = "";					// MsgType 
		string				md_req_id = "";					// MDReqID
		string				symbol = "";					// Symbol
		bool				check = false;					// Check
	};
	struct SIn
	{
		string				msg_type = "";					// MsgType 
		string				security_res_id = "";			// SecurityResponseID
	};

	vector<SOut>		m_out_file;
	vector<SIn>			m_in_file;
	vector<string>		db_symbol_acc;
	vector<string>		db_symbol_acc_stock;
	int					y_code_count = 0;
	string				front_name = "";
	string				back_name = "";
	string				key_back_name = "";
	string				trading_date = "";
	string				db_driver = "";
	string				db_server = "";
	string				db_database = "";
	string				db_user = "";
	string				db_password = "";
	string				db_logname = "";

	DBAPI		dbs;
	SQLDATA		data;

private:

public:
	bool				ReadFile(string input);
	int					FindField(string line, char* input);
	int				GetSymbolBase(char* cmd_temp, bool check = false);
	bool				InsertLogs(string app, int res, string comment, string db);
	string				GetIpByName(string hostname);
	void				writeConfig(LPCTSTR key, string value);
	void				CheckSymbolByDB(vector<string> input, bool check = false);
	int					CheckSymbol();
	void				writeNameFileConfig();

private:
};

extern Processor processor;