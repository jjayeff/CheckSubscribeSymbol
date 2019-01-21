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
	struct SChange
	{
		string				trade_seq_so_series = "";		// TradeSeqNoSeries
		string				symbol = "";					// Symbol
	};

	vector<SOut>		m_out_file;
	vector<SIn>			m_in_file;
	vector<SAll>		m_all_file;
	vector<SChange>		m_change_file;
	vector<string>		ignore_sumbol;
	vector<string>		db_symbol_acc;
	vector<string>		db_symbol_acc_stock;
	int					msg_type_x = 0;
	int					msg_type_y = 0;
	int					msg_type_v = 0;
	string				front_name = "";
	string				back_name = "";
	string				key_front_name = "";
	string				key_back_name = "";
	string				file_path = "";
	string				trading_date = "";
	string				db_driver = "";
	string				db_server = "";
	string				db_database = "";
	string				db_user = "";
	string				db_password = "";
	string				db_logname = "";
	int					run_time = 0;

	DBAPI				dbs;
	SQLDATA				data;

private:

public:
	int					Run();
	int					ReadFile(string input);
	int					FindField(string line, char* input);
	int					ConnectDataBase();
	int					GetSymbolBase(char* cmd_temp, bool check = false);
	int					InsertLogs(string app, int res, string comment, string db);
	string				GetIpByName(string hostname);
	void				writeConfig(LPCTSTR path, LPCTSTR key, string value);
	void				CheckSymbolByDB(vector<string> input, bool check = false);
	int					CheckSymbol();
	int					SetFrontBackName();
	int					ChangeTradeSeqNoSeri(string input);
	int					CutString(string input);

private:
};

extern Processor processor;