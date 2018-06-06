/******************************************************************************
 *    Copyright (c) V2 Systems, LLC.  All rights reserved.
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
#include "evUtils.h"

namespace evutils {
#define FAIL -1


// Obtains a key:column # mapping for a matrix of data
// Assumes the text for the column is in the first (header) row
map<string,int> LoadFromCSVKeyMap(const string filename) {

    map<string,int> keyMap;
    ifstream file(filename);
    string line;

    if (file.is_open()) {
        getline (file, line);
        stringstream s(line);
        int valIdx = 0;
        string field;
        while (getline(s,field,',')) {
            keyMap.insert(map<string, int>::value_type(field,valIdx));
            valIdx++;
        }
        file.close();
    }
    return keyMap;
}

// function to load int values into a matrix from a csv file
// startIdx is used to avoid headers
vector<vector<int>> LoadFromCSVInt(const string filename, int startIdx) {

    ifstream file(filename);
    vector< vector<int>> matrix;
    vector<int> row;
    string line;

    if (file.is_open()) {
        int lineNum = 0;
        while (getline (file, line)) {
            stringstream s(line);
            row.clear();
            string field;
            while (getline(s,field,',')) {
                if (lineNum >= startIdx)
                    row.push_back(atoi(field.c_str()));
            }
            lineNum++;
            if (!row.empty())
               matrix.push_back(row);
        }
        file.close();
    }
    return matrix;
}

int GetKeyIndex(string keyName, map<string,int> keyMap) {
    map<string,int>::iterator it;

    int idx = FAIL;
    it = keyMap.find(keyName);
    if(it != keyMap.end()) {
        idx = it->second;
    }
    return idx;
}

//Get a pretty print string of time
string DateTime(time_t &tval) {
    //time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&tval);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

//Add delta seconds to a time, dsecs must not be larger than one month in seconds.

//Jose v
int AddTime2(time_t &tval)
{
     struct tm  * timeinfo;
     timeinfo = localtime(&tval);
     int dhour = timeinfo->tm_hour;

return dhour;
}
//Jose ^
time_t AddTime(time_t &tval, int &dsecs)
{
     struct tm  * timeinfo;
     //ELARTODO: Put an error check on size of dsec
     timeinfo = localtime(&tval);

     int dmin = (timeinfo->tm_sec + dsecs)/60;

     timeinfo->tm_sec = (timeinfo->tm_sec + dsecs) % 60;

     int dhour = (timeinfo->tm_min + dmin)/60;
     timeinfo->tm_min = (timeinfo->tm_min + dmin) % 60;

     int dday = (timeinfo->tm_hour + dhour)/24;
     timeinfo->tm_hour = (timeinfo->tm_hour + dhour) % 24;

     switch (timeinfo->tm_mon){
     case 1:
     case 3:
     case 5:
     case 7:
     case 8:
     case 10:
          if ((timeinfo->tm_mday + dday) > 31)
            timeinfo->tm_mon +=1;
          timeinfo->tm_mday = (timeinfo->tm_mday + dday) % 31;
          break;

     case 4:
     case 6:
     case 9:
     case 11:
          if ((timeinfo->tm_mday + dday) > 30)
            timeinfo->tm_mon +=1;
          timeinfo->tm_mday = (timeinfo->tm_mday + dday) % 31;
          break;

     case 2:
         int d;
         if (timeinfo->tm_year/4 == 0)
            d = 29;
         else d = 28;
         if ((timeinfo->tm_mday + dday) > d)
            timeinfo->tm_mon +=1;
         timeinfo->tm_mday = (timeinfo->tm_mday + dday) % d;
         break;

     case 12:
          if ((timeinfo->tm_mday + dday) > 31){
            timeinfo->tm_mon =1;
            timeinfo->tm_year+=1;
          }
          timeinfo->tm_mday = (timeinfo->tm_mday + dday) % 31;
          break;
     }
    return mktime(timeinfo);
}

//Sets the hours, minutes, seconds for the time input
time_t SetTime(time_t &tval,int &hr, int &minute, int &sec) {

    struct tm * timeinfo;

    timeinfo = localtime(&tval);

    if (hr >= 24) {
        cerr << "Unable to set hour: invalid hour value " << hr << endl;
    } else timeinfo->tm_hour = hr;

    if (minute >= 60){
        cerr << "Unable to set minute: invalid minute value " << minute << endl;
    } else timeinfo->tm_min = minute;

    if (sec >=60){
        cerr << "Unable to set seconds: invalid seconds value " << sec << endl;
    } else timeinfo->tm_sec = sec;

    return mktime(timeinfo);

}
}
