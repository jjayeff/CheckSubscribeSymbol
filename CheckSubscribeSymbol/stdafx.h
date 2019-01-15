// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS

//---
#ifndef _WIN32_WINNT          // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501   // Change this to the appropriate value to target other versions of Windows.
#endif
//---
#include <process.h>
#include <Winsock2.h>

#include <string>
#include <vector>
#include <sstream> //istringstream
#include <iostream> // cout
#include <fstream> // ifstream
#include <filesystem>
#include <msclr\marshal_cppstd.h>
#include <direct.h>
#define GetCurrentDir _getcwd

#include <cmath> 
#include <map>
#include <iomanip>
#include <list>
#include "winconfig/winconfig.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Version.lib")

using namespace std;
namespace fs = experimental::filesystem;
using namespace msclr;
//+------------------------------------------------------------------+

// TODO: reference additional headers your program requires here
