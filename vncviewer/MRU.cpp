/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


//
// MRU maintains a list of 'Most Recently Used' strings in the registry
// 

#include "MRU.h"
#include "VNCOptions.h"
#include <shlobj.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <cJSON.h>

static const TCHAR * INDEX_VAL_NAME = _T("index");
static const TCHAR DEL = char(127);
TCHAR RESERVED_CHARS[5] = _T("[;=");  //String of characters that will cause a key/value line to be parsed differently if set as a key
static const int FIRST_USEABLE_ID = 0;//_T('!');
static const int LAST_USEABLE_ID = 256;//_T('~');
static const int MRU_MAX_ITEM_LENGTH = 256; //Managed to get max length to 90. Issue is that we have 101
static const char* optionFile = "";
char buffer[4096];
//cJSON* jsonParse;


MRU::MRU(LPTSTR keyname, unsigned int maxnum)
{
    VNCOptions::setDefaultOptionsFileName(m_optionfile);
    optionFile = m_optionfile;
    m_maxnum = maxnum;
    // Read the index entry
    //ReadIndex();
}

// Add the item specified at the front of the list
// Move it there if not already. If this makes the
// list longer than the maximum, older ones are deleted.

void ofnInit();
void MRU::AddItem(LPTSTR txt)
{
    // We don't add empty items.
    if (_tcslen(txt) == 0)
        return;
    // Read each value in index,
    // noting which is the first unused id
    int id = 0;
    int firstUnusedId = FIRST_USEABLE_ID;
    TCHAR itembuf[MRU_MAX_ITEM_LENGTH + 1];

    cJSON* jsonParse = OpenJson();
    int i;
    cJSON* json;
    int n = cJSON_GetArraySize(jsonParse);
    for (i = 0; i < n; i++) {
        json = cJSON_GetArrayItem(jsonParse, i);
        if(GetItem(i,txt,json)) {
            cJSON* placeHolderValue;
            char strIndex[256];
            sprintf(strIndex, "%d", i);
            cJSON* placeHolder = cJSON_GetArrayItem(jsonParse, i);
            cJSON_ReplaceItemInArray(jsonParse, i, new cJSON);
            cJSON_InsertItemInArray(jsonParse, 0, placeHolder);
            WriteToOptionFile(jsonParse);
            return;
        }
        
    }
    cJSON* tempObject = cJSON_CreateObject();
    char strIndex[256];
    sprintf(strIndex, "%d", n);
    cJSON_AddStringToObject(tempObject, strIndex, txt);
    cJSON_InsertItemInArray(jsonParse, 0, tempObject);
    WriteToOptionFile(jsonParse);
    
}
void MRU::WriteToOptionFile(cJSON* jsonParse) {
    FILE* fp = fopen(m_optionfile, "w");
    char* test = cJSON_Print(jsonParse);
    fputs(test, fp);
    fclose(fp);
}
cJSON* MRU::OpenJson() {
    FILE* jsonFile = fopen(m_optionfile, "r");
    if (jsonFile == NULL) {
        VNCOptions::setDefaultOptionsFileName(m_optionfile);
        jsonFile = fopen(m_optionfile, "r");
    }
    memset(buffer, '\0', 4096);
    int len = fread(buffer, 1, sizeof(buffer), jsonFile);
    fclose(jsonFile);

    cJSON* jsonParse2 = cJSON_Parse(buffer);


    if (jsonFile == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
    }
    if (jsonParse2 != NULL) {
        return jsonParse2;

    }
    else {
        cJSON* array = cJSON_CreateArray();

        return array;
    }
}

void MRU::IncrementList(int index, cJSON* jsonParse) 
{
    int i;
    cJSON json;
    if (index != 0) {
        char* test = cJSON_GetStringValue(cJSON_GetArrayItem(jsonParse, index-1));
        cJSON* test2 = cJSON_GetArrayItem(jsonParse, index-2);
        cJSON_InsertItemInArray(jsonParse, 0, cJSON_GetArrayItem(jsonParse, index + 1));
        for (i = index; i > 0; i--) {
            char* test = cJSON_GetStringValue(cJSON_GetArrayItem(jsonParse, i + 1));
            cJSON_InsertItemInArray(jsonParse, 0, cJSON_GetArrayItem(jsonParse, i + 1));
        }
    }
    
}

// How many items are on the list?
int MRU::NumItems()
{
    cJSON* json;
    int i;
    int n = cJSON_GetArraySize(OpenJson());
    return n;
}

cJSON* MRU::GetValue(int index, cJSON* json) {
    char strIndex[256];
    sprintf(strIndex, "%d", index);
    cJSON* array = cJSON_GetArrayItem(json, index);
    cJSON* value = json->child;//cJSON_GetObjectItem(json,strIndex);
    return value;
}
// Return them in order. 0 is the newest.
bool MRU::GetItem(int index, LPTSTR txt, cJSON* json)
{
    char placeHolder[256];
    cJSON* value = GetValue(index, json);
    if (cJSON_IsString(value) && (value->valuestring != NULL)) {
        strcpy(placeHolder, value->valuestring);
        //for (int i = 0; i < strlen(value->valuestring); i++) {
        //    placeHolder[i] = value->valuestring[i];
        //}
        //placeHolder[strlen(value->valuestring)] = '\0';
    }
    if (!strcmp(placeHolder, txt)) 
    {
        return true;
    }
    else {
        return false;
    }
}
int MRU::GetItem(int index, LPTSTR buf, int buflen)
{
    cJSON* jsonParse = OpenJson();
    if (jsonParse->child != NULL) {
        char* test = cJSON_GetStringValue(cJSON_GetArrayItem(jsonParse, index)->child);
        cJSON* test3 = cJSON_GetArrayItem(jsonParse, index);
        char* test2 = cJSON_Print(cJSON_GetArrayItem(jsonParse, index));
        if (test == NULL) {
            return 0;
        }
        else {
            strcpy(buf, test);
            return _tcslen(test);
        }
    }
}

// Remove the specified item if it exists.
// Only one copy will be removed, but nothing should occur more than once.
void MRU::RemoveItem(LPTSTR txt)
{
    cJSON* jsonParse = OpenJson();
    int i;
    cJSON* json;
    int n = cJSON_GetArraySize(OpenJson());
    for (i = 0; i < n; i++) {
        json = cJSON_GetArrayItem(jsonParse, i);
        if (GetItem(i, txt, json))
        {
            cJSON* nextValue = cJSON_GetArrayItem(jsonParse, i+1);
            cJSON* prevValue = cJSON_GetArrayItem(jsonParse, i - 1);
            cJSON* lastValue = cJSON_GetArrayItem(jsonParse, cJSON_GetArraySize(jsonParse)-1);
            if (nextValue != NULL)
            {
                nextValue->prev = prevValue;
            }
            if(prevValue != NULL)
            {
                prevValue->next = nextValue;
            }
            while(strcmp(prevValue->child->valuestring,lastValue->child->valuestring)) 
            {
                int k = atoi(prevValue->child->string);
                //char strIndex[4];
                sprintf(prevValue->child->string, "%d", k-1);
                //prevValue->child->string = strIndex;
                prevValue = prevValue->prev;
                char* debugString = cJSON_Print(jsonParse);
            }
            n--;
        }
    }
    WriteToOptionFile(jsonParse);
}

void MRU::SetPos(LPTSTR txt, int x, int y, int w, int h)
{
	char buf[32];
	sprintf_s(buf, "%d", x);
	//WritePrivateProfileString(txt, "x", buf, m_optionfile);
	sprintf_s(buf, "%d", y);
	//WritePrivateProfileString(txt, "y", buf, m_optionfile);
	sprintf_s(buf, "%d", w);
	//WritePrivateProfileString(txt, "w", buf, m_optionfile);
	sprintf_s(buf, "%d", h);
	//WritePrivateProfileString(txt, "h", buf, m_optionfile);
}

int MRU::Get_x(LPTSTR txt)
{
	char buf[32];
	//GetPrivateProfileString(txt, "x", "", buf, 32, m_optionfile);
	return atoi(buf);
}

int MRU::Get_y(LPTSTR txt)
{
	char buf[32];
	//GetPrivateProfileString(txt, "y", "", buf, 32, m_optionfile);
	return atoi(buf);
}
int MRU::Get_w(LPTSTR txt)
{
	char buf[32];
	//GetPrivateProfileString(txt, "w", "", buf, 32, m_optionfile);
	return atoi(buf);
}
int MRU::Get_h(LPTSTR txt)
{
	char buf[32];
	//GetPrivateProfileString(txt, "h", "", buf, 32, m_optionfile);
	return atoi(buf);
}
// Remove the item with the given index.
// If this is greater than NumItems()-1 it will be ignored.
void MRU::RemoveItem(int index)
{
    cJSON* jsonParse = OpenJson();
    cJSON_ReplaceItemInArray(jsonParse, index, new cJSON());
}

// Since we're doing linked list- always reading first item.
void MRU::ReadIndex()
{
    DWORD dwindexlen = sizeof(m_index);
    int size = 0;//NumItems();
}

// Index idea has been deleted. Can remove this later
void MRU::WriteIndex()
{
	//WritePrivateProfileString("connection", INDEX_VAL_NAME, m_index, m_optionfile);
}


MRU::~MRU()
{
    
}
