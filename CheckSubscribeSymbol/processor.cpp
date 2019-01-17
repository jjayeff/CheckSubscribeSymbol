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
	config.setValue("Application", "KeyFrontName", "D0118__FIX__MD1");
	config.setValue("Application", "KeyBackName", "SET");

	config.setValue("Database", "Driver", "SQL Server Native Client 11.0");
	config.setValue("Database", "Server", "172.17.1.43");
	config.setValue("Database", "Database", "acc_info");
	config.setValue("Database", "Username", "sa");
	config.setValue("Database", "Password", "P@ssw0rd");
	config.setValue("Database", "LogName", "CheckSubscribeSymbol");

	key_front_name = config.getValueString("Application", "KeyFrontName");
	key_back_name = config.getValueString("Application", "KeyBackName");

	db_driver = config.getValueString("Database", "Driver");
	db_server = GetIpByName(config.getValueString("Database", "Server"));
	db_database = config.getValueString("Database", "Database");
	db_user = config.getValueString("Database", "Username");
	db_password = config.getValueString("Database", "Password");
	db_logname = config.getValueString("Database", "LogName");

	vnLog.InitialLog(config.getValueString("Application", "LogPath"), "CheckSubscribeSymbol", 10, true);
}
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
Processor::~Processor() {
}
//+------------------------------------------------------------------+
//| Run Program                                                      |
//+------------------------------------------------------------------+
int Processor::Run() {
	if (SetFrontBackName())
		return 1;
	else {
		front_name = config.getValueString("Application", "FrontName");
		back_name = config.getValueString("Application", "BackName");
		time_t t = time(0);   // get time now
		tm* now = localtime(&t);
		string date = to_string(now->tm_year + 1900) + "-" + to_string(now->tm_mon + 1) + "-" + to_string(now->tm_mday) + " 00:00:00.000";
		writeConfig("TradingDate", date);
		trading_date = date;
		return 0;
	}
}
//+------------------------------------------------------------------+
//| Read File                                                        |
//+------------------------------------------------------------------+
int Processor::ReadFile(string input) {
	ifstream myfile(input.c_str());
	string line;
	size_t   p = 0;
	myfile.seekg(p);
	if (myfile.is_open()) {
		while (myfile.eof() == false) {
			getline(myfile, line);
			if (!input.compare(input.size() - 3, 3, ".in")) {
				// Push MsgType to array
				if (FindField(line, "35=X") > -1 || FindField(line, "35=Y") > -1) {
					SIn tmp;
					tmp.msg_type = line.substr(FindField(line, "35=") + 3, 1);
					tmp.security_res_id = line.substr(FindField(line, "262=") + 4, 18);
					m_in_file.push_back(tmp);
				}
			}
			else {
				// Push MsgType to array
				if (FindField(line, "35=V") > -1 && FindField(line, "55=[N/A]") < 0 && FindField(line, "269=2") > -1) {
					int symbol_index = FindField(line, "55=");
					int symbol_index_last = FindField(line, "48=") - 4;
					SOut tmp;
					tmp.msg_type = line.substr(FindField(line, "35=V") + 3, 1);
					tmp.md_req_id = line.substr(FindField(line, "262=") + 4, 18);
					tmp.symbol = line.substr(symbol_index + 3, symbol_index_last - symbol_index);
					m_out_file.push_back(tmp);
				}
			}
			p = myfile.tellg();  //*2
		}
		return 0;
	}
	else {
		LOGE << "Cannot read " << input;
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
int Processor::ConnectDataBase() {
	// Connect Datebase
	if (!dbs.connect(db_driver, db_server, db_user, db_password))
	{
		LOGE << "!Database connect fail";
		dbs.commit();
		return 1;
	}
	else
	{
		LOGI << "Database connected : " << db_server << ", Driver : " << db_driver;
		return 0;
	}
}
int Processor::GetSymbolBase(char* cmd_temp, bool check) {
	if (!dbs.isConnected()) {
		LOGE << "Database disconnect! Try reconnect!";
		dbs.connect();
		return 1;
	}

	if (!dbs.execute(cmd_temp)) {
		LOGE << "!Execute database fail";
		return 1;
	}

	data = dbs.getSQLData();

	if (data.Size() < 1) {
		LOGE << "!Not have data from database";
		return 1;
	}
	while (data.FetchNext()) {
		string tmp = data.GetField("symbol");
		if (check)
			db_symbol_acc.push_back(tmp);
		else
			db_symbol_acc_stock.push_back(tmp);
	}

	if (data.Size() > 0)
		return 0;
	else {
		LOGE << "Database not have data from (" << trading_date << ")";
		return 1;
	}
}
int Processor::InsertLogs(string app, int res, string comment, string db) {
	if (!dbs.isConnected())
	{
		LOGE << "Database disconnect! Try reconnect!";
		dbs.connect();
	}

	char cmd_temp[512];
	sprintf_s(cmd_temp, "INSERT INTO %s.dbo.app_schedule_log VALUES('%s', GETDATE(), '%d', '%s');", db.c_str(), app.c_str(), res, comment.c_str());

	if (!dbs.execute(cmd_temp))
	{
		LOGE << "!Excute database fail";
		return 0;
	}

	LOGI << "InsertLog Success!!";
	return 1;
}
//+------------------------------------------------------------------+
//| Check Symbol Function                                            |
//+------------------------------------------------------------------+
void Processor::CheckSymbolByDB(vector<string> input, bool check) {
	int count = 0;
	int non_res = 0;
	int non_req = 0;
	for (int i = 0; i < input.size(); i++) {
		//cout << endl << input[i] << ":";
		for (int j = 0; j < m_all_file.size(); j++) {
			if (input[i] == m_all_file[j].symbol && m_all_file[j].msg_type == "X") {
				//cout << m_out_file[j].symbol;
				count++;
				break;
			}
			else if (input[i] == m_all_file[j].symbol && m_all_file[j].msg_type == "V") {
				non_res++;
				break;
			}
			else if (j + 1 == m_all_file.size()) {
				non_req++;
			}
		}
	}

	// Insert log to database
	string log = "", db;
	LOGI << "Done: " << count << ", Y: " << msg_type_y << ", nonRes: " << non_res << ", nonReq: " << non_req - msg_type_y;

	if (check) {
		LOGI << "acc_info: " << count << "/" << input.size();
		db = "acc_info";
	}
	else {
		LOGI << "acc_info_stock: " << count << "/" << input.size();
		db = "acc_info_stock";
	}
	if (msg_type_y > 0) {
		log += "File .in have 35=Y (" + to_string(msg_type_y) + ")";
	}

	if (input.size() == count && input.size() && msg_type_y == 0) {
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
	// Make m_all_file 
	for (int i = 0; i < m_out_file.size(); i++) {
		for (int j = 0; j < m_in_file.size(); j++) {
			if (m_out_file[i].md_req_id == m_in_file[j].security_res_id) {
				m_out_file[i].msg_type = m_in_file[j].msg_type;
				SAll tmp;
				tmp.msg_type = m_in_file[j].msg_type;
				tmp.md_req_id = m_out_file[i].md_req_id;
				tmp.security_res_id = m_in_file[j].security_res_id;
				tmp.symbol = m_out_file[i].symbol;
				m_all_file.push_back(tmp);
				break;
			}
			else if (j + 1 == m_in_file.size()) {
				SAll tmp;
				tmp.msg_type = "V";
				tmp.md_req_id = m_out_file[i].md_req_id;
				tmp.security_res_id = m_out_file[i].md_req_id;
				tmp.symbol = m_out_file[i].symbol;
				m_all_file.push_back(tmp);
			}
		}
	}

	// Check not Response
	for (int i = 0; i < m_out_file.size(); i++)
		for (int j = 0; j < m_all_file.size(); j++)
			if (m_out_file[i].md_req_id == m_all_file[j].md_req_id) {
				break;
			}
			else if (j + 1 == m_all_file.size()) {
				msg_type_v++;
			}

	// Make file test !!
	/*ofstream myfile("example.in");
	if (myfile.is_open())
	{
		ifstream xx(front_name + "-" + back_name + ".in");
		string line;
		size_t   p = 0;
		xx.seekg(p);
		if (xx.is_open())
			while (xx.eof() == false) {
				getline(xx, line);
				myfile << line;
				myfile << "\n";
			}
		// Check not Response
		for (int i = 0; i < m_out_file.size(); i++)
			for (int j = 0; j < m_all_file.size(); j++)
				if (m_out_file[i].md_req_id == m_all_file[j].md_req_id) {
					break;
				}
				else if (j + 1 == m_all_file.size()) {
					myfile << "35=X";
					myfile << "262=";
					myfile << m_out_file[i].md_req_id;
					myfile << "\n";
					msg_type_v++;
				}
		myfile.close();
	}
	else cout << "Unable to open file";*/

	// Set count of X and Y
	for (int i = 0; i < m_all_file.size(); i++)
		if (m_all_file[i].msg_type == "X")
			msg_type_x++;
		else if (m_all_file[i].msg_type == "Y")
			msg_type_y++;

	// 
	LOGI << "X: " << msg_type_x << ", Y: " << msg_type_y << ", Not Response: " << msg_type_v;
	if (msg_type_y > 0)
		LOGW << "Y = " << msg_type_y;
	for (int i = 0; i < m_all_file.size(); i++)
		if (m_all_file[i].msg_type == "Y")
			LOGW << "(Y) symbol: " << m_all_file[i].symbol;

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
int Processor::SetFrontBackName() {
	string path = "./";
	string real_path = path;
	string max = "";

	char *cstr_front_name = new char[key_front_name.length()];
	strcpy(cstr_front_name, key_front_name.c_str());
	char *cstr_back_name = new char[key_back_name.length()];
	strcpy(cstr_back_name, key_back_name.c_str());
	for (const auto & entry : fs::directory_iterator(path)) {
		ostringstream oss;
		oss << entry;
		string path = oss.str();
		int index_front = FindField(path, cstr_front_name);
		int index_back = FindField(path, cstr_back_name);
		if ((path.substr(path.size() - 3, 3) == ".in" || path.substr(path.size() - 4, 4) == ".out")
			&& FindField(path, cstr_front_name) > -1
			&& processor.FindField(path, cstr_back_name) > -1
			&& (path.substr(path.size() - 7, 7) != ".ndx.in" && path.substr(path.size() - 8, 8) != ".ndx.out")
			) {
			if (path.substr(path.size() - 3, 3) == ".in") {
				if (max < path.substr(index_back + key_back_name.length() + 1, path.size() - index_back - 7)) {
					max = path.substr(index_back + key_back_name.length() + 1, path.size() - index_back - 7);
					real_path = path.substr(2, path.size() - 5);
				}
			}
			else
				if (path.substr(index_back + key_back_name.length() + 1, path.size() - index_back - 8) > max) {
					max = path.substr(index_back + key_back_name.length() + 1, path.size() - index_back - 8);
					real_path = path.substr(2, path.size() - 6);
				}
		}
	}

	if (real_path == "./") {
		LOGE << "Cannot find file please set key front and back name";
		return 1;
	}

	int index_front = FindField(real_path, cstr_front_name);
	int index_back = FindField(real_path, cstr_back_name);
	writeConfig("FrontName", real_path.substr(index_front, index_back - 1));
	writeConfig("BackName", real_path.substr(index_back, real_path.size()));
	return 0;
}