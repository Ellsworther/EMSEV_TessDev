#ifndef EMSUTILS_INC
#define EMSUTILS_INC

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <time.h>

using namespace std;

namespace emsutils
{
map<string,int> LoadFromCSVKeyMap (const string filename);
vector<vector <int>> LoadFromCSVInt(const string filename,int index);
int GetKeyIndex(string keyName, map<string,int> keyMap);
string DateTime(time_t&);
time_t AddTime(time_t&,int&);
time_t SetTime(time_t&,int&,int&,int&);
}

#endif // EMSUTILS_INC
