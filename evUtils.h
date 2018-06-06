#ifndef EVUTILS_INC
#define EVUTILS_INC

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <time.h>

using namespace std;

namespace evutils
{
map<string,int> LoadFromCSVKeyMap (const string filename);
vector<vector <int>> LoadFromCSVInt(const string filename,int index);
int GetKeyIndex(string keyName, map<string,int> keyMap);
string DateTime(time_t&);
time_t AddTime(time_t&,int&);
int AddTime2(time_t&);// Jose
time_t SetTime(time_t&,int&,int&,int&);
}

#endif // EVUTILS_INC
