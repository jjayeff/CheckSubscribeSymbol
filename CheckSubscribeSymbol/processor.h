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
	};
	struct SIn
	{
		string				msg_type = "";					// MsgType 
		string				security_res_id = "";			// SecurityResponseID
	};
	struct SAll
	{
		string				msg_type = "";					// MsgType 
		string				md_req_id = "";					// MDReqID
		string				security_res_id = "";			// SecurityResponseID
		string				symbol = "";					// Symbol
	};

	vector<SOut>		m_out_file;
	vector<SIn>			m_in_file;
	vector<SAll>		m_all_file;
	vector<string>		db_symbol_acc;
	vector<string>		db_symbol_acc_stock;
	int					msg_type_x = 0;
	int					msg_type_y = 0;
	int					msg_type_v = 0;
	string				front_name = "";
	string				back_name = "";
	string				key_front_name = "";
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
	int					Run();
	int					ReadFile(string input);
	int					FindField(string line, char* input);
	int					ConnectDataBase();
	int					GetSymbolBase(char* cmd_temp, bool check = false);
	int					InsertLogs(string app, int res, string comment, string db);
	string				GetIpByName(string hostname);
	void				writeConfig(LPCTSTR key, string value);
	void				CheckSymbolByDB(vector<string> input, bool check = false);
	int					CheckSymbol();
	int					SetFrontBackName();

private:
};

extern Processor processor;