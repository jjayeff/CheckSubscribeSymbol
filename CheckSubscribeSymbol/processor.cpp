#include "stdafx.h"
#include "Processor.h"
Configuration	config;
LogClass		vnLog;
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
Processor::Processor() {
	// Setting configuration
	config.setConfig("CheckSubscribeSymbol.ini");

	string appPath = config.getAbsolutePath();

	config.setValue("Application", "FrontName", "D0118__FIX__MD1");
	config.setValue("Application", "BackName", "SET_1901140211232301");
	config.setValue("Application", "TradingDate", "2019-01-14 00:00:00.000");
	config.setValue("Application", "LogPath", appPath);

	config.setValue("Database", "Driver", "SQL Server Native Client 11.0");
	config.setValue("Database", "Server", "172.17.1.43");
	config.setValue("Database", "Database", "acc_info");
	config.setValue("Database", "Username", "sa");
	config.setValue("Database", "Password", "P@ssw0rd");
	config.setValue("Database", "LogName", "CheckSubscribeSymbol");

	writeNameFileConfig();
	front_name = config.getValueString("Application", "FrontName");
	back_name = config.getValueString("Application", "BackName");
	string date = "20" + back_name.substr(4, 2) + "-" + back_name.substr(6, 2) + "-" + back_name.substr(8, 2) + " 00:00:00.000";
	writeConfig("TradingDate", date);
	trading_date = date;

	db_driver = config.getValueString("Database", "Driver");
	db_server = GetIpByName(config.getValueString("Database", "Server"));
	db_database = config.getValueString("Database", "Database");
	db_user = config.getValueString("Database", "Username");
	db_password = config.getValueString("Database", "Password");
	db_logname = config.getValueString("Database", "LogName");

	vnLog.InitialLog(config.getValueString("Application", "LogPath"), "CheckSubscribeSymbol", 10, true);

	// Connect Datebase
	if (!dbs.connect(db_driver, db_server, db_user, db_password))
	{
		cout << "!Database connect fail" << endl;
		dbs.commit();
	}
	else
	{
		cout << "Database connected : " << db_server << ", Driver : " << db_driver << endl;
	}
}
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
Processor::~Processor() {
}
//+------------------------------------------------------------------+
//| Read File                                                        |
//+------------------------------------------------------------------+
bool Processor::ReadFile(string input) {
	ifstream myfile(input.c_str());
	string line;
	size_t   p = 0;
	myfile.seekg(p);
	if (myfile.is_open()) {
		int y_code_count = 0;
		while (myfile.eof() == false) {
			getline(myfile, line);
			if (!input.compare(input.size() - 3, 3, ".in")) {
				// Push MsgType to array
				if (FindField(line, "35=X") > -1) {
					SIn tmp;
					tmp.msg_type = line.substr(FindField(line, "35=") + 3, 1);
					tmp.security_res_id = line.substr(FindField(line, "262=") + 4, 18);
					m_in_file.push_back(tmp);
				}
				else if (FindField(line, "35=Y") > -1)
					y_code_count++;
			}
			else {
				// Push MsgType to array
				if (FindField(line, "35=V") > -1 && FindField(line, "55=[N/A]") < 0 && FindField(line, "269=2") > -1) {
					int x = FindField(line, "55=");
					int y = FindField(line, "48=") - 4;
					SOut tmp;
					tmp.msg_type = line.substr(FindField(line, "35=V") + 3, 1);
					tmp.md_req_id = line.substr(FindField(line, "262=") + 4, 18);
					tmp.symbol = line.substr(x + 3, y - x);
					m_out_file.push_back(tmp);
				}
			}
			p = myfile.tellg();  //*2
		}
		if (y_code_count > 0) {
			LOGW << "Y= " << y_code_count;
			string log = "File .in have 35=Y (" + to_string(y_code_count) + ")";
			InsertLogs(db_logname, 0, log, "acc_info");
			InsertLogs(db_logname, 0, log, "acc_info_stock");
		}
		return 0;
	}
	else {
		return 1;
	}
}
//+------------------------------------------------------------------+
//| Find Field B2bits                                                |
//+------------------------------------------------------------------+
int Processor::FindField(string line, char* input) {
	char *tmp = new char[line.size() + 1];
	copy(line.begin(), line.end(), tmp);
	char *result = strstr(tmp, input);
	int position = result - tmp;

	return position;
}
//+------------------------------------------------------------------+
//| Database Function                                                |
//+------------------------------------------------------------------+
void Processor::GetSymbolBase(char* cmd_temp, bool check) {
	if (!dbs.isConnected())
	{
		LOGE << "Database disconnect! Try reconnect!";
		dbs.connect();
	}

	if (!dbs.execute(cmd_temp))
	{
		LOGE << "!Excute database fail";
	}

	data = dbs.getSQLData();

	while (data.FetchNext())
	{
		string tmp = data.GetField("symbol");
		if (check)
			db_symbol_acc.push_back(tmp);
		else
			db_symbol_acc_stock.push_back(tmp);
	}
}
bool Processor::InsertLogs(string app, int res, string comment, string db) {
	if (!dbs.isConnected())
	{
		cout << "Database disconnect! Try reconnect!" << endl;
		dbs.connect();
	}

	char cmd_temp[512];
	sprintf_s(cmd_temp, "INSERT INTO %s.dbo.app_schedule_log VALUES('%s', GETDATE(), '%d', '%s');", db.c_str(), app.c_str(), res, comment.c_str());

	if (!dbs.execute(cmd_temp))
	{
		LOGE << "!Excute database fail";
		return false;
	}

	LOGI << "InsertLog Success!!";
	return true;
}
//+------------------------------------------------------------------+
//| Check Symbol Function                                            |
//+------------------------------------------------------------------+
void Processor::CheckSymbolByDB(vector<string> input, bool check) {
	int count = 0;
	for (int i = 0; i < input.size(); i++) {
		//cout << endl << input[i] << ":";
		for (int j = 0; j < m_out_file.size(); j++) {
			if (input[i] == m_out_file[j].symbol && m_out_file[j].check) {
				//cout << m_out_file[j].symbol;
				count++;
				break;
			}
		}
	}
	// Insert log to database
	string log, db;
	if (check) {
		LOGI << "acc_info: " << count << "/" << input.size();
		db = "acc_info";
	}
	else {
		LOGI << "acc_info_stock: " << count << "/" << input.size();
		db = "acc_info_stock";
	}
	if (input.size() == count && input.size() && count) {
		log += "Request symbol complete (" + to_string(count) + "/" + to_string(input.size()) + ")";
		InsertLogs(db_logname, 1, log, db);
	}
	else {
		log += "Request symbol fail (" + to_string(count) + "/" + to_string(input.size()) + ")";
		InsertLogs(db_logname, 0, log, db);
	}

}
int Processor::CheckSymbol() {
	LOGI << ".in: (" << m_in_file.size() << ") | .out: (" << m_out_file.size() << ")";
	for (int i = 0; i < m_out_file.size(); i++) {
		for (int j = 0; j < m_in_file.size(); j++) {
			if (m_out_file[i].md_req_id == m_in_file[j].security_res_id) {
				m_out_file[i].check = true;
			}
		}
	}

	CheckSymbolByDB(db_symbol_acc, 1);
	CheckSymbolByDB(db_symbol_acc_stock);

	return 0;
}
//+------------------------------------------------------------------+
//| Other Function                                                   |
//+------------------------------------------------------------------+
string Processor::GetIpByName(string hostname)
{
	string ans = hostname;
	WSADATA wsaData;
	int iResult;
	DWORD dwError;

	struct hostent* remoteHost;
	struct in_addr addr;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		return hostname;
	}

	remoteHost = gethostbyname(ans.c_str());

	if (remoteHost == NULL)
	{
		dwError = WSAGetLastError();
		if (dwError != 0)
		{
			if (dwError == WSAHOST_NOT_FOUND)
			{
				return ans;
			}
			else if (dwError == WSANO_DATA)
			{
				return ans;
			}
			else
			{
				return ans;
			}
		}
	}
	else
	{
		if (remoteHost->h_addrtype == AF_INET)
		{
			addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
			ans = inet_ntoa(addr);
		}
	}
	return ans;
}
void Processor::writeConfig(LPCTSTR key, string value) {
	LPCTSTR result = value.c_str();
	LPCTSTR path = ".\\CheckSubscribeSymbol.ini";
	WritePrivateProfileString(_T("Application"), key, result, path);
}
void Processor::writeNameFileConfig() {
	string path = "./";
	string real_path = path;
	string max = "";
	for (const auto & entry : fs::directory_iterator(path)) {
		std::ostringstream oss;
		oss << entry;
		std::string path = oss.str();
		if ((path.substr(path.size() - 3, 3) == ".in" || path.substr(path.size() - 4, 4) == ".out") && FindField(path, "SET_") > -1) {
			if (max < path.substr(FindField(path, "SET_") + 4, 16)) {
				max = path.substr(FindField(path, "SET_") + 4, 16);
				real_path = path;
			}
		}
	}
	writeConfig("FrontName", real_path.substr(2, FindField(real_path, "SET_") - 3));
	writeConfig("BackName", real_path.substr(FindField(real_path, "SET_"), 20));
}