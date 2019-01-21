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

	string appPath = config.getAbsolutePath() + "logs";

	config.setValue("Application", "FrontName", "D0118__FIX__MD1");
	config.setValue("Application", "BackName", "SET_1901140211232301");
	config.setValue("Application", "TradingDate", "2019-01-14 00:00:00.000");
	config.setValue("Application", "LogPath", appPath);
	config.setValue("Application", "KeyFrontName", "D0118__FIX__MD1");
	config.setValue("Application", "KeyBackName", "SET");
	config.setValue("Application", "FilePath", appPath);

	config.setValue("Database", "Driver", "SQL Server Native Client 11.0");
	config.setValue("Database", "Server", "172.17.1.43");
	config.setValue("Database", "Database", "acc_info");
	config.setValue("Database", "Username", "sa");
	config.setValue("Database", "Password", "P@ssw0rd");
	config.setValue("Database", "LogName", "CheckSubscribeSymbol");

	key_front_name = config.getValueString("Application", "KeyFrontName");
	key_back_name = config.getValueString("Application", "KeyBackName");
	file_path = config.getValueString("Application", "FilePath");
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
		string date = to_string(now->tm_year + 1900) + "-" + to_string(now->tm_mon + 1) + "-" + to_string(now->tm_mday) + " 00:00:00";
		writeConfig(".\\CheckSubscribeSymbol.ini", "TradingDate", date);
		trading_date = date;
		return 0;
	}
}
//+------------------------------------------------------------------+
//| Read File                                                        |
//+------------------------------------------------------------------+
int Processor::ReadFile(string input) {
	fstream myfile(input.c_str(), fstream::in);
	string line;
	size_t   p = 0;
	myfile.seekg(p);
	if (myfile.is_open()) {
		int weqwewqeq = 0;
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
		LOGI << "ReadFile Success " << input;
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
	LOGI << "Execute: " << cmd_temp;
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
		LOGW << "!Not have data from database";
		return 0;
	}
	while (data.FetchNext()) {
		string tmp = data.GetField("symbol");
		if (check)
			db_symbol_acc.push_back(tmp);
		else
			db_symbol_acc_stock.push_back(tmp);
	}

	if (data.Size() > 0) {
		return 0;
	}
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
	int non_y = 0;
	int non_res = 0;
	int non_req = 0;
	for (int i = 0; i < input.size(); i++) {
		for (int j = 0; j < m_all_file.size(); j++) {
			if (input[i] == m_all_file[j].symbol && m_all_file[j].msg_type == "X") {
				count++;
				break;
			}
			else if (input[i] == m_all_file[j].symbol && m_all_file[j].msg_type == "Y") {
				non_y++;
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
	LOGI << "Done: " << count << ", Y: " << non_y << ", nonRes: " << non_res << ", nonReq: " << non_req;

	if (check) {
		db = "acc_info";
	}
	else {
		db = "acc_info_stock";
	}

	if (input.size() == count + non_res && input.size()) {
		LOGI << db << ": " << count + non_res << "/" << input.size();
		log += "Request symbol complete (" + to_string(count + non_res) + "/" + to_string(input.size()) + ")";
		InsertLogs(db_logname + " #" + to_string(run_time), 1, log, db);
	}
	else {
		if (non_y > 0) {
			log += "File .in have 35=Y (" + to_string(non_y) + ")";
		}
		LOGI << db << ": " << count + non_res << "/" << input.size();
		log += "Request symbol fail (" + to_string(count + non_res) + "/" + to_string(input.size()) + ")";
		InsertLogs(db_logname + " #" + to_string(run_time), 0, log, db);
	}
}
int Processor::CheckSymbol() {
	LOGI << ".in: (" << m_in_file.size() << ") | .out: (" << m_out_file.size() << ")";

	// Make m_all_file
	for (int i = 0; i < m_out_file.size(); i++)
		for (int j = 0; j < m_in_file.size(); j++) {
			if (m_out_file[i].md_req_id == m_in_file[j].security_res_id) {
				int check = 1;
				for (int k = 0; k < m_all_file.size(); k++)
					if (m_out_file[i].symbol == m_all_file[k].symbol && m_out_file[i].md_req_id != m_all_file[k].md_req_id) {
						if (m_all_file[k].msg_type != "V" && m_all_file[k].msg_type != "X") {
							m_all_file[k].msg_type = m_in_file[j].msg_type;
							m_all_file[k].md_req_id = m_out_file[i].md_req_id;
							m_all_file[k].security_res_id = m_in_file[j].security_res_id;
						}
						check = 0;
						break;
					}
				if (check) {
					SAll tmp;
					tmp.msg_type = m_in_file[j].msg_type;
					tmp.md_req_id = m_out_file[i].md_req_id;
					tmp.security_res_id = m_in_file[j].security_res_id;
					tmp.symbol = m_out_file[i].symbol;
					m_all_file.push_back(tmp);
				}
				break;
			}
			else if ((j + 1 == m_in_file.size())) {
				int check = 1;
				for (int k = 0; k < m_all_file.size(); k++)
					if (m_out_file[i].symbol == m_all_file[k].symbol && m_out_file[i].md_req_id != m_all_file[k].md_req_id) {
						m_all_file[k].msg_type = "V";
						m_all_file[k].md_req_id = m_out_file[i].md_req_id;
						m_all_file[k].security_res_id = m_out_file[i].md_req_id;
						check = 0;
						break;
					}
				if (check) {
					SAll tmp;
					tmp.msg_type = "V";
					tmp.md_req_id = m_out_file[i].md_req_id;
					tmp.security_res_id = m_out_file[i].md_req_id;
					tmp.symbol = m_out_file[i].symbol;
					m_all_file.push_back(tmp);
				}
			}
		}

	// Set count of X and Y and not Response
	for (int i = 0; i < m_all_file.size(); i++)
		if (m_all_file[i].msg_type == "X")
			msg_type_x++;
		else if (m_all_file[i].msg_type == "Y")
			msg_type_y++;
		else if (m_all_file[i].msg_type == "V")
			msg_type_v++;


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
//| Change TradeSeqNoSeri                                            |
//+------------------------------------------------------------------+
int Processor::ChangeTradeSeqNoSeri(string input) {
	for (int i = 0; i < m_all_file.size(); i++)
		if (m_all_file[i].msg_type == "Y") {
			std::cout << endl << m_all_file[i].symbol << " | ";
			string y_case = "55=" + m_all_file[i].symbol;
			char *cstr = new char[y_case.length()];
			strcpy(cstr, y_case.c_str());
			fstream myfile(input.c_str(), fstream::in);
			string line;
			size_t   p = 0;
			myfile.seekg(p);
			if (myfile.is_open()) {
				while (myfile.eof() == false) {
					getline(myfile, line);
					if (FindField(line, "35=AA") > -1 && FindField(line, cstr) > -1 && line.substr(FindField(line, cstr) + y_case.length() + 3, 1) == "=") {
						SChange tmp;
						tmp.trade_seq_so_series = line.substr(FindField(line, "7555=") + 5, 3);
						tmp.symbol = m_all_file[i].symbol;
						m_change_file.push_back(tmp);
					}
					p = myfile.tellg();  //*2
				}
			}
		}

	vector<string> iro;

	ifstream myfile("example.txt");
	string line;
	size_t   p = 0;
	myfile.seekg(p);
	if (myfile.is_open()) {
		while (myfile.eof() == false) {
			getline(myfile, line);
			iro.push_back(line);
			p = myfile.tellg();  //*2
		}
	}
	ofstream mywrite("example.txt");

	for (int i = 0; i < iro.size(); i++) {
		string y_case = m_change_file[0].symbol;
		char *cstr = new char[y_case.length()];
		strcpy(cstr, y_case.c_str());
		if (FindField(iro[i], cstr) > -1) {
			int q = 0; int r = 0; int o = 0;
			for (int j = 0; j < iro[i].length(); j++) {
				if (j + 1 == iro[i].length())
					std::cout << endl;
				if (iro[i][j] == ',')
					q++;
				if (q == 1) {
					r = j;
				}
				else if (q == 2) {
					o = j;
				}
			}
			std::cout << iro[i].substr(0, r + 2) + m_change_file[0].trade_seq_so_series + iro[i].substr(o + 1, iro[i].size()) << endl;
			mywrite << iro[i].substr(0, r + 2) + m_change_file[0].trade_seq_so_series + iro[i].substr(o + 1, iro[i].size());
			mywrite << "\n";
		}
		else {
			mywrite << iro[i];
			mywrite << "\n";
		}
	}

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
void Processor::writeConfig(LPCTSTR path, LPCTSTR key, string value) {
	LPCTSTR result = value.c_str();
	WritePrivateProfileString(_T("Application"), key, result, path);
}
int Processor::SetFrontBackName() {
	string path = file_path;
	string real_path = path;
	double max = -999999;

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
			// Get Modified time
			char *tmp = new char[path.length()];
			strcpy(tmp, path.c_str());
			struct stat fileInfo;
			if (stat(tmp, &fileInfo) != 0) {  // Use stat( ) to get the info
				LOGE<< "Error: " << strerror(errno);
				return 1;
			}
			// Compare last time of file
			time_t t = time(0);   // get time now
			double seconds = difftime(fileInfo.st_mtime, t);
			if (path.substr(path.size() - 3, 3) == ".in") {
				if (max < seconds) {
					max = seconds;
					real_path = path.substr(0, path.size() - 3);
				}
			}
			else
				if (max < seconds) {
					max = seconds;
					real_path = path.substr(0, path.size() - 4);
				}
		}
	}

	if (real_path == "./") {
		LOGE << "Cannot find file please set key front and back name";
		return 1;
	}

	int index_front = FindField(real_path, cstr_front_name);
	int index_back = FindField(real_path, cstr_back_name);
	writeConfig(".\\CheckSubscribeSymbol.ini", "FrontName", real_path.substr(index_front, index_back - index_front - 1));
	writeConfig(".\\CheckSubscribeSymbol.ini", "BackName", real_path.substr(index_back, real_path.size()));
	return 0;
}