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
#include "stdhdrs.h"
#include "vncviewer.h"
#include "SessionDialog.h"
#include <shlobj.h>
#include <direct.h>
#include <fstream>
#include "UltraVNCHelperFunctions.h"
#include <cJSON.h>
#include <stdio.h>
#include <dirent.h>
extern char sz_K1[64];
extern char sz_K2[64];
extern bool g_disable_sponsor;
static OPENFILENAME ofn;
TCHAR m_host[MAX_HOST_NAME_LEN];

void SessionDialog::SaveConnection(HWND hwnd, bool saveAs)
{
	SettingsFromUI();
	char fname[_MAX_PATH];
	int disp = PORT_TO_DISPLAY(m_port);
	sprintf_s(fname, "%.15s-%d.vnc", m_host_dialog, (disp > 0 && disp < 100) ? disp : m_port);
	char buffer[_MAX_PATH];
	getAppData(buffer);
	strcat_s(buffer,"\\UltraVNC");
	_mkdir(buffer);

	if ( saveAs) {
		char tname[_MAX_FNAME + _MAX_EXT];
		
		static char filter[] = "VNC files (*.vnc)\0*.vnc\0" \
				"All files (*.*)\0*.*\0";
		memset((void *) &ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
		ofn.lpstrFilter = filter;
		ofn.nMaxFile = _MAX_PATH;
		ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
		ofn.lpstrDefExt = "vnc";	
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = fname;
		ofn.lpstrFileTitle = tname;
		ofn.lpstrInitialDir = buffer;
		ofn.Flags = OFN_HIDEREADONLY;
		if (!GetSaveFileName(&ofn)) {
			DWORD err = CommDlgExtendedError();
			char msg[1024]; 
			switch(err) {
			case 0:	// user cancelled
				break;
			case FNERR_INVALIDFILENAME:
				strcpy_s(msg, sz_K1);
				yesUVNCMessageBox(hwnd, msg, sz_K2, MB_ICONERROR);
				break;
			default:
				vnclog.Print(0, "Error %d from GetSaveFileName\n", err);
				break;
			}
			return;
		}
		SaveToFile(fname);
	}
	else {
		strcat_s(buffer,"\\");
		strcat_s(buffer,fname);
		SaveToFile(buffer);
	}
	
	TCHAR hostname[256];
	GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, hostname, 256);
	m_pMRU->AddItem(hostname);
	InitMRU(hwnd);
}

void SessionDialog::SettingsFromUI()
{
	ReadDlgProcEncoders();
	ReadDlgProcKeyboardMouse();
	ReadDlgProcDisplay();
	ReadDlgProcMisc();
	ReadDlgProcSecurity();
	ReadDlgProcListen();
	ReadDlgProc();
}

void SessionDialog::SettingsToUI(bool initMruNeeded)
{
	InitDlgProcEncoders();
	InitDlgProcKeyboardMouse();
	InitDlgProcDisplay();
	InitDlgProcMisc();
	InitDlgProcSecurity();	
	InitDlgProcListen();
	InitDlgProc(true, initMruNeeded);		
}

void SessionDialog::saveInt(char *name, int value, char *fname) 
{
  char buf[10];
  sprintf_s(buf, "%d", value); 
  WritePrivateProfileString("options", name, buf, fname);
}

int SessionDialog::readInt(char *name, int defval, char *fname)
{
  return GetPrivateProfileInt("options", name, defval, fname);
}

struct cJSON* SessionDialog::SaveToJson(char* fname, bool asDefault) {
	char buf[32];


	cJSON* json = cJSON_CreateObject();

	if (!asDefault) {
		cJSON_AddStringToObject(json, "host", m_host);
		sprintf_s(buf, "%d", m_port);
		cJSON_AddNumberToObject(json, "port", atoi(buf));
	}
	else
		SettingsFromUI(); //TODO: make this compatibile properly
	cJSON_AddStringToObject(json, "proxyhost", m_proxyhost);
	sprintf_s(buf, "%d", m_proxyport);
	cJSON_AddNumberToObject(json, "proxyport", atoi(buf));

	cJSON_AddStringToObject(json, "alias", m_alias);
	cJSON_AddStringToObject(json, "ipAddress", m_ipAddress);
	cJSON_AddStringToObject(json, "macAddress", m_macAddress);

	for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
		char buf[128];
		sprintf_s(buf, "use_encoding_%d", i);
		saveInt(buf, UseEnc[i], fname);
		cJSON_AddBoolToObject(json, buf, UseEnc[1]);
	}
		if (!PreferredEncodings.empty()) {
			cJSON_AddNumberToObject(json, "preferred_encoding", PreferredEncodings[0]);
		}
		//cJSON_AddBoolToObject(json, "viewonly", ViewOnly);
		cJSON_AddBoolToObject(json, "viewonly", ViewOnly);
		cJSON_AddBoolToObject(json, "showtoolbar", ShowToolbar);
		cJSON_AddBoolToObject(json, "fullscreen", FullScreen);
		cJSON_AddBoolToObject(json, "SavePos", SavePos);
		cJSON_AddBoolToObject(json, "SaveSize", SaveSize);
		cJSON_AddBoolToObject(json, "GNOME", GNOME);
		cJSON_AddBoolToObject(json, "directx", Directx);
		cJSON_AddBoolToObject(json, "autoDetect", autoDetect);
		cJSON_AddBoolToObject(json, "8bit", Use8Bit);
		cJSON_AddBoolToObject(json, "shared", Shared);
		cJSON_AddBoolToObject(json, "swapmouse", SwapMouse);
		cJSON_AddBoolToObject(json, "emulate3", Emul3Buttons);
		cJSON_AddBoolToObject(json, "JapKeyboard", JapKeyboard);
		cJSON_AddBoolToObject(json, "disableclipboard", DisableClipboard);
		cJSON_AddBoolToObject(json, "Scaling", scaling);
		cJSON_AddBoolToObject(json, "AutoScaling", fAutoScaling);
		cJSON_AddBoolToObject(json, "AutoScalingEven", fAutoScalingEven);
		cJSON_AddBoolToObject(json, "AutoScalingLimit", fAutoScalingLimit);
		cJSON_AddBoolToObject(json, "scale_num", scale_num);
		cJSON_AddBoolToObject(json, "scale_den", scale_den);
		// Tight Specific
		cJSON_AddBoolToObject(json, "cursorshape", requestShapeUpdates);
		cJSON_AddBoolToObject(json, "noremotecursor", ignoreShapeUpdates);
		if (useCompressLevel) {
			cJSON_AddBoolToObject(json, "compresslevel", compressLevel);
		}
		if (enableJpegCompression) {
			cJSON_AddBoolToObject(json, "quality", jpegQualityLevel);
		}
		// Modif sf@2002
		cJSON_AddBoolToObject(json, "ServerScale", nServerScale);
		cJSON_AddBoolToObject(json, "Reconnect", reconnectcounter);
		cJSON_AddBoolToObject(json, "EnableCache", fEnableCache);
		cJSON_AddBoolToObject(json, "EnableZstd", fEnableZstd);
		cJSON_AddBoolToObject(json, "QuickOption", quickoption);
		cJSON_AddBoolToObject(json, "UseDSMPlugin", fUseDSMPlugin);
		cJSON_AddBoolToObject(json, "UseProxy", m_fUseProxy);
		cJSON_AddBoolToObject(json, "allowMonitorSpanning", allowMonitorSpanning);
		cJSON_AddBoolToObject(json, "ChangeServerRes", changeServerRes);
		cJSON_AddBoolToObject(json, "extendDisplay", extendDisplay);
		cJSON_AddBoolToObject(json, "showExtend", showExtend);
		cJSON_AddBoolToObject(json, "use_virt", use_virt);
		cJSON_AddBoolToObject(json, "useAllMonitors", useAllMonitors);
		cJSON_AddBoolToObject(json, "requestedWidth", requestedWidth);
		cJSON_AddBoolToObject(json, "requestedHeight", requestedHeight);


		cJSON_AddStringToObject(json, "DSMPlugin", szDSMPluginFilename);
		cJSON_AddStringToObject(json, "folder", folder); //Formatted wrong i.e "\User\sesa\" etc. instead of \\ will require special attention later
		cJSON_AddStringToObject(json, "prefix", prefix);
		cJSON_AddStringToObject(json, "imageFormat", imageFormat);
		cJSON_AddStringToObject(json, "InfoMsg", InfoMsg);
		cJSON_AddBoolToObject(json, "AutoReconnect", autoReconnect);
		cJSON_AddBoolToObject(json, "FileTransferTimeout", FTTimeout);
		cJSON_AddBoolToObject(json, "ThrottleMouse", throttleMouse);
		cJSON_AddBoolToObject(json, "KeepAliveInterval", keepAliveInterval);
		cJSON_AddBoolToObject(json, "AutoAcceptIncoming", fAutoAcceptIncoming);
		cJSON_AddBoolToObject(json, "AutoAcceptNoDSM", fAutoAcceptNoDSM);
#ifdef _Gii
		cJSON_AddBoolToObject(json, "GiiEnable", giiEnable);
#endif
		cJSON_AddBoolToObject(json, "RequireEncryption", fRequireEncryption);
		cJSON_AddBoolToObject(json, "restricted", restricted);  //hide menu
		cJSON_AddBoolToObject(json, "AllowUntrustedServers", AllowUntrustedServers);
		cJSON_AddBoolToObject(json, "nostatus", NoStatus); //hide status window
		cJSON_AddBoolToObject(json, "nohotkeys", NoHotKeys); //disable hotkeys
		cJSON_AddBoolToObject(json, "sponsor", g_disable_sponsor);
		cJSON_AddBoolToObject(json, "PreemptiveUpdates", preemptiveUpdates);
		// convert the cJSON object to a JSON string 
		//char* json_str = ;
		return json;
		// write the JSON string to a file 
		

	}

void SessionDialog::SaveToFile(char *fname, bool asDefault)
{
	int ret;
	char buf[32];
	
	if (!asDefault) {
		ret = WritePrivateProfileString("connection", "host", m_host_dialog, fname);		
		sprintf_s(buf, "%d", m_port);
		WritePrivateProfileString("connection", "port", buf, fname);
	}
	else {
		ret = WritePrivateProfileString("connection", "host", m_host, fname);
		sprintf_s(buf, "%d", m_port);
		WritePrivateProfileString("connection", "port", buf, fname);
	}
		//SettingsFromUI();
	ret = WritePrivateProfileString("connection", "proxyhost", m_proxyhost, fname);
	sprintf_s(buf, "%d", m_proxyport);
	WritePrivateProfileString("connection", "proxyport", buf, fname);

	WritePrivateProfileString("connection", "alias", m_alias, fname);
	WritePrivateProfileString("connection", "ipAddress", m_ipAddress, fname);
	WritePrivateProfileString("connection", "macAddress", m_macAddress, fname);

	for (int i = rfbEncodingRaw; i<= LASTENCODING; i++) {
		char buf[128];
		sprintf_s(buf, "use_encoding_%d", i);
		saveInt(buf, UseEnc[i], fname);
	 }
	if (!PreferredEncodings.empty()) {
	  saveInt("preferred_encoding", PreferredEncodings[0], fname);
	}	
	saveInt("viewonly",				ViewOnly,			fname);	
	saveInt("showtoolbar",			ShowToolbar,		fname);
	saveInt("fullscreen",			FullScreen,		fname);
	saveInt("SavePos",				SavePos, fname);
	saveInt("SaveSize",				SaveSize, fname);
	saveInt("GNOME",				GNOME, fname);
	saveInt("directx",				Directx,		fname);
	saveInt("autoDetect",			autoDetect, fname);
	saveInt("8bit",					Use8Bit,			fname);
	saveInt("shared",				Shared,			fname);
	saveInt("swapmouse",			SwapMouse,		fname);
	saveInt("emulate3",				Emul3Buttons,		fname);
	saveInt("JapKeyboard",			JapKeyboard,		fname);
	saveInt("disableclipboard",		DisableClipboard, fname);
	saveInt("Scaling",				scaling,		fname);
	saveInt("AutoScaling",			fAutoScaling,		fname);
	saveInt("AutoScalingEven",      fAutoScalingEven, fname);
	saveInt("AutoScalingLimit",		fAutoScalingLimit, fname);
	saveInt("scale_num",			scale_num,		fname);
	saveInt("scale_den",			scale_den,		fname);
	// Tight Specific
	saveInt("cursorshape",			requestShapeUpdates, fname);
	saveInt("noremotecursor",		ignoreShapeUpdates, fname);
	if (useCompressLevel) {
		saveInt("compresslevel",	compressLevel,	fname);
	}
	if (enableJpegCompression) {
		saveInt("quality",			jpegQualityLevel,	fname);
	}
	// Modif sf@2002
	saveInt("ServerScale",			nServerScale,		fname);
	saveInt("Reconnect",			reconnectcounter,		fname);
	saveInt("EnableCache",			fEnableCache,		fname);
	saveInt("EnableZstd",			fEnableZstd, fname);
	saveInt("QuickOption",			quickoption,	fname);
	saveInt("UseDSMPlugin",			fUseDSMPlugin,	fname);
	saveInt("UseProxy",				m_fUseProxy,	fname);
	saveInt("allowMonitorSpanning", allowMonitorSpanning, fname);
	saveInt("ChangeServerRes", changeServerRes, fname);
	saveInt("extendDisplay", extendDisplay, fname);
	saveInt("showExtend", showExtend, fname);
	saveInt("use_virt", use_virt, fname);	
	saveInt("useAllMonitors", useAllMonitors, fname);
	saveInt("requestedWidth", requestedWidth, fname);
	saveInt("requestedHeight", requestedHeight, fname);


	WritePrivateProfileString("options", "DSMPlugin",	szDSMPluginFilename, fname);
	WritePrivateProfileString("options", "folder",		folder, fname);
	WritePrivateProfileString("options", "prefix",		prefix, fname);
	WritePrivateProfileString("options", "imageFormat",		imageFormat, fname);
	WritePrivateProfileString("options", "InfoMsg", InfoMsg, fname);
	saveInt("AutoReconnect",		autoReconnect,	fname);
	saveInt("FileTransferTimeout",  FTTimeout,    fname);
	saveInt("ThrottleMouse",		throttleMouse,    fname); 
	saveInt("KeepAliveInterval",    keepAliveInterval,    fname);	
	saveInt("AutoAcceptIncoming",	fAutoAcceptIncoming, fname);  
	saveInt("AutoAcceptNoDSM",		fAutoAcceptNoDSM, fname);
#ifdef _Gii
	saveInt("GiiEnable", giiEnable, fname);
#endif
	saveInt("RequireEncryption",	fRequireEncryption, fname);
	saveInt("restricted",			restricted,		fname);  //hide menu
	saveInt("AllowUntrustedServers", AllowUntrustedServers, fname);
	saveInt("nostatus",				NoStatus,			fname); //hide status window
	saveInt("nohotkeys",			NoHotKeys,		fname); //disable hotkeys
	saveInt("sponsor",				g_disable_sponsor,	fname);
	saveInt("PreemptiveUpdates",	preemptiveUpdates, fname);

	
	if (!asDefault) {
		cJSON* json = cJSON_CreateObject();
		cJSON* arrayObject = cJSON_CreateArray();
		FILE* fp = fopen("TestFile.json", "w");


		DIR* d;
		struct dirent* dir;
		char buffer[_MAX_PATH];
		char directory[_MAX_PATH];
		getAppData(buffer);
		strcat_s(buffer, "\\UltraVNC\\");
		d = opendir(buffer);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				char temp[1000];
				strcpy(temp, dir->d_name);
				strcpy(directory, buffer);
				strcat_s(directory, temp);
				if (dir->d_type == DT_REG)
				{

					LoadFromFile(directory);
					cJSON_AddItemToArray(arrayObject, SaveToJson(directory, asDefault));
				}
			}
			closedir(d);
		}
		fputs(cJSON_Print(arrayObject), fp);
		fclose(fp);
	}
}
void SessionDialog::LoadFromJson(char* fname, HWND hwnd) {
	FILE* jsonFile = fopen(fname, "r");
	char buffer[655360];
	int len = fread(buffer, 1, sizeof(buffer), jsonFile);
	fclose(jsonFile);

	cJSON* jsonParse = cJSON_Parse(buffer);


	if (jsonFile == NULL) {
		const char* error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
			printf("Error: %s\n", error_ptr);
		}
	}
	int i;
	cJSON* json;
	int n = cJSON_GetArraySize(jsonParse);
	for (i = 0; i < n; i++) {
		json = cJSON_GetArrayItem(jsonParse, i);

		// access the JSON data 


			//memset(m_host, 0, 250);
	//memset(m_proxyhost, 0, 250);
		cJSON* value = cJSON_GetObjectItemCaseSensitive(json, "host");
		if (cJSON_IsString(value) && (value->valuestring != NULL)) {
			for (int i = 0; i < strlen(value->valuestring); i++) {
				m_host[i] = value->valuestring[i];
			}
			m_host[strlen(value->valuestring)] = '\0';
		}
		else {
			value = cJSON_GetObjectItemCaseSensitive(json, "remoteHost");
			if (cJSON_IsString(value) && (value->valuestring != NULL)) {
				for (int i = 0; i < strlen(value->valuestring); i++) {
					m_host[i] = value->valuestring[i];
				}
				m_host[strlen(value->valuestring)] = '\0';
			}
		}

		value = cJSON_GetObjectItemCaseSensitive(json, "ipAddress");
		if (cJSON_IsString(value) && (value->valuestring != NULL)) {
			for (int i = 0; i < strlen(value->valuestring); i++) {
				m_ipAddress[i] = value->valuestring[i];
			}
			m_ipAddress[strlen(value->valuestring)] = '\0';
		}

		value = cJSON_GetObjectItemCaseSensitive(json, "alias");
		if (cJSON_IsString(value) && (value->valuestring != NULL)) {
			for (int i = 0; i < strlen(value->valuestring); i++) {
				m_alias[i] = value->valuestring[i];
			}
			m_alias[strlen(value->valuestring)] = '\0';
		}

		value = cJSON_GetObjectItemCaseSensitive(json, "macAddress");
		if (cJSON_IsString(value) && (value->valuestring != NULL)) {
			for (int i = 0; i < strlen(value->valuestring); i++) {
				m_macAddress[i] = value->valuestring[i];
			}
			m_macAddress[strlen(value->valuestring)] = '\0';
		}

		value = cJSON_GetObjectItemCaseSensitive(json, "port");
		if (value != NULL) {
			m_port = value->valueint;

		}
		else {
			/*value = cJSON_GetObjectItemCaseSensitive(json, "portNumber");*/
			m_port = 5900;//value->valueint;
		}

		value = cJSON_GetObjectItemCaseSensitive(json, "proxyhost");
		if (cJSON_IsString(value) && (value->valuestring != NULL)) {
			for (int i = 0; i < strlen(value->valuestring); i++) {
				m_proxyhost[i] = value->valuestring[i];
			}
			m_proxyhost[strlen(value->valuestring)] = '\0';
		}
		else {
			value = cJSON_GetObjectItemCaseSensitive(json, "hostProxy");
			if (cJSON_IsString(value) && (value->valuestring != NULL)) {
				for (int i = 0; i < strlen(value->valuestring); i++) {
					m_proxyhost[i] = value->valuestring[i];
				}
				m_proxyhost[strlen(value->valuestring)] = '\0';
			}
		}

		value = cJSON_GetObjectItemCaseSensitive(json, "proxyport");
		if (value != NULL) {
			m_proxyport = value->valueint;
		}
		else {
			value = cJSON_GetObjectItemCaseSensitive(json, "portProxy");
			m_proxyport = value->valueint;
		}
		value = cJSON_GetObjectItem(json, "UseProxy");
		m_fUseProxy = value->valueint;


		for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
			char buf[128];
			sprintf_s(buf, "use_encoding_%d", i);
			cJSON* enco = cJSON_GetObjectItemCaseSensitive(json, buf);
			if ((enco != NULL)) {
				UseEnc[i] = enco->valueint;
			}
		}



		int nExistingPreferred = PreferredEncodings.empty() ? rfbEncodingZRLE : PreferredEncodings[0];
		cJSON* nPrefEnco = cJSON_GetObjectItemCaseSensitive(json, "preferred_encoding");
		if ((nPrefEnco != NULL)) {
			int nPreferredEncoding = nPrefEnco->valueint;
			PreferredEncodings.clear();
			PreferredEncodings.push_back(nPreferredEncoding);
		}
		cJSON* restrictedJSON = cJSON_GetObjectItemCaseSensitive(json, "restricted");
		if ((restrictedJSON != NULL)) {
			restricted = restrictedJSON->valueint != 0;
		}
		cJSON* AllowUntrustedServersJ = cJSON_GetObjectItemCaseSensitive(json, "AllowUntrustedServers");
		if ((AllowUntrustedServersJ != NULL)) {
			AllowUntrustedServers = AllowUntrustedServersJ->valueint != 0;
		}
		cJSON* ViewOnlyJ = cJSON_GetObjectItemCaseSensitive(json, "viewonly");
		if ((ViewOnlyJ != NULL)) {
			ViewOnly = ViewOnlyJ->valueint != 0;
		}
		else {
			ViewOnlyJ = cJSON_GetObjectItemCaseSensitive(json, "viewsOnly");
			ViewOnly = ViewOnlyJ->valueint != 0;
		}
		cJSON* NoStatusJ = cJSON_GetObjectItemCaseSensitive(json, "nostatus");
		if ((NoStatusJ != NULL)) {
			NoStatus = NoStatusJ->valueint != 0;
		}
		cJSON* NoHotKeysJ = cJSON_GetObjectItemCaseSensitive(json, "nohotkeys");
		if ((NoHotKeysJ != NULL)) {
			NoHotKeys = NoHotKeysJ->valueint != 0;
		}
		cJSON* ShowToolbarJ = cJSON_GetObjectItemCaseSensitive(json, "showtoolbar");
		if ((ShowToolbarJ != NULL)) {
			ShowToolbar = ShowToolbarJ->valueint != 0;
		}
		cJSON* FullScreenJ = cJSON_GetObjectItemCaseSensitive(json, "fullscreen");
		if ((FullScreenJ != NULL)) {
			FullScreen = FullScreenJ->valueint != 0;
		}
		cJSON* SavePosJ = cJSON_GetObjectItemCaseSensitive(json, "SavePos");
		if ((SavePosJ != NULL)) {
			SavePos = SavePosJ->valueint != 0;
		}
		cJSON* SaveSizeJ = cJSON_GetObjectItemCaseSensitive(json, "SaveSize");
		if ((SaveSizeJ != NULL)) {
			SaveSize = SaveSizeJ->valueint != 0;
		}
		cJSON* GNOMEJ = cJSON_GetObjectItemCaseSensitive(json, "GNOME");
		if ((GNOMEJ != NULL)) {
			GNOME = GNOMEJ->valueint != 0;
		}
		cJSON* DirectxJ = cJSON_GetObjectItemCaseSensitive(json, "directx");
		if ((DirectxJ != NULL)) {
			Directx = DirectxJ->valueint != 0;
		}
		cJSON* autoDetectJ = cJSON_GetObjectItemCaseSensitive(json, "autoDetect");
		if ((autoDetectJ != NULL)) {
			autoDetect = autoDetectJ->valueint != 0;
		}
		cJSON* Use8BitJ = cJSON_GetObjectItemCaseSensitive(json, "8Bit");
		if ((Use8BitJ != NULL)) {
			Use8Bit = Use8BitJ->valueint != 0;
		}
		cJSON* SharedJ = cJSON_GetObjectItemCaseSensitive(json, "shared");
		if ((SharedJ != NULL)) {
			Shared = SharedJ->valueint != 0;
		}
		cJSON* SwapMouseJ = cJSON_GetObjectItemCaseSensitive(json, "swapmouse");
		if ((SwapMouseJ != NULL)) {
			SwapMouse = SwapMouseJ->valueint != 0;
		}
		cJSON* Emul3ButtonsJ = cJSON_GetObjectItemCaseSensitive(json, "emulate3");
		if ((Emul3ButtonsJ != NULL)) {
			Emul3Buttons = Emul3ButtonsJ->valueint != 0;
		}
		cJSON* JapKeyboardJ = cJSON_GetObjectItemCaseSensitive(json, "JapKeyboard");
		if ((JapKeyboardJ != NULL)) {
			JapKeyboard = JapKeyboardJ->valueint != 0;
		}
		cJSON* DisableClipboardJ = cJSON_GetObjectItemCaseSensitive(json, "disableclipboard");
		if ((DisableClipboardJ != NULL)) {
			DisableClipboard = DisableClipboardJ->valueint != 0;
		}
		cJSON* scalingJ = cJSON_GetObjectItemCaseSensitive(json, "Scaling");
		if ((scalingJ != NULL)) {
			scaling = scalingJ->valueint != 0;
		}
		cJSON* fAutoScalingJ = cJSON_GetObjectItemCaseSensitive(json, "AutoScaling");
		if ((fAutoScalingJ != NULL)) {
			fAutoScaling = fAutoScalingJ->valueint != 0;
		}
		cJSON* fAutoScalingEvenJ = cJSON_GetObjectItemCaseSensitive(json, "AutoScalingEven");
		if ((fAutoScalingEvenJ != NULL)) {
			fAutoScalingEven = fAutoScalingEvenJ->valueint != 0;
		}
		cJSON* fAutoScalingLimitJ = cJSON_GetObjectItemCaseSensitive(json, "AutoScalingLimit");
		if ((fAutoScalingLimitJ != NULL)) {
			fAutoScalingLimit = fAutoScalingLimitJ->valueint != 0;
		}
		cJSON* scale_numJ = cJSON_GetObjectItemCaseSensitive(json, "scale_num");
		if ((scale_numJ != NULL)) {
			scale_num = scale_numJ->valueint != 0;
		}
		cJSON* scale_denJ = cJSON_GetObjectItemCaseSensitive(json, "scale_den");
		if ((scale_denJ != NULL)) {
			scale_den = scale_denJ->valueint != 0;
		}
		// Tight specific
		cJSON* requestShapeUpdatesJ = cJSON_GetObjectItemCaseSensitive(json, "cursorshape");
		if ((requestShapeUpdatesJ != NULL)) {
			requestShapeUpdates = requestShapeUpdatesJ->valueint != 0;
		}
		cJSON* ignoreShapeUpdatesJ = cJSON_GetObjectItemCaseSensitive(json, "noremovecursor");
		if ((ignoreShapeUpdatesJ != NULL)) {
			ignoreShapeUpdates = ignoreShapeUpdatesJ->valueint != 0;
		}
		else {
			ignoreShapeUpdatesJ = cJSON_GetObjectItemCaseSensitive(json, "showRemoteCursor");
			ignoreShapeUpdates = ignoreShapeUpdatesJ->valueint != 0;
		}
		int level;
		cJSON* levelJ = cJSON_GetObjectItemCaseSensitive(json, "compressLevel");
		if ((levelJ != NULL)) {
			level = levelJ->valueint != 0;
		}
		if (level != -1) {
			useCompressLevel = true;
			compressLevel = level;
		}
		levelJ = cJSON_GetObjectItemCaseSensitive(json, "quality");
		if ((levelJ != NULL)) {
			level = levelJ->valueint != 0;
		}
		level = readInt("quality", -1, fname);
		if (level != -1) {
			enableJpegCompression = true;
			jpegQualityLevel = level;
		}
		// Modif sf@2002
		cJSON* ReadValue = cJSON_GetObjectItemCaseSensitive(json, "ServerScale");
		if ((ReadValue != NULL)) {
			nServerScale = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "Reconnect");
		if ((ReadValue != NULL)) {
			reconnectcounter = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "EnabledCache");
		if ((ReadValue != NULL)) {
			fEnableCache = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "EnableZstd");
		if ((ReadValue != NULL)) {
			fEnableZstd = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "QuickOption");
		if ((ReadValue != NULL)) {
			quickoption = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "UseDSMPlugin");
		if ((ReadValue != NULL)) {
			fUseDSMPlugin = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItem(json, "UseProxy");
		if ((ReadValue != NULL)) {
			m_fUseProxy = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "allowMonitorSpanning");
		if ((ReadValue != NULL)) {
			allowMonitorSpanning = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "ChangeServerRes");
		if ((ReadValue != NULL)) {
			changeServerRes = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "extendDisplay");
		if ((ReadValue != NULL)) {
			extendDisplay = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "showExtend");
		if ((ReadValue != NULL)) {
			showExtend = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "use_virt");
		if ((ReadValue != NULL)) {
			use_virt = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "useAllMonitors");
		if ((ReadValue != NULL)) {
			useAllMonitors = ReadValue->valueint != 0;
		}

		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "requestedWidth");
		if ((ReadValue != NULL)) {
			requestedWidth = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "requestedHeight");
		if ((ReadValue != NULL)) {
			requestedHeight = ReadValue->valueint != 0;
		}

		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "DSMPlugin");
		if (cJSON_IsString(ReadValue) && (ReadValue->valuestring != NULL)) {
			for (int i = 0; i < strlen(ReadValue->valuestring); i++) {
				szDSMPluginFilename[i] = ReadValue->valuestring[i];
			}
			szDSMPluginFilename[strlen(ReadValue->valuestring)] = '\0';
		}

		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "folder");
		if (cJSON_IsString(ReadValue) && (ReadValue->valuestring != NULL)) {
			for (int i = 0; i < strlen(ReadValue->valuestring); i++) {
				folder[i] = ReadValue->valuestring[i];
			}
			folder[strlen(ReadValue->valuestring)] = '\0';
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "prefix");
		if (cJSON_IsString(ReadValue) && (ReadValue->valuestring != NULL)) {
			for (int i = 0; i < strlen(ReadValue->valuestring); i++) {
				prefix[i] = ReadValue->valuestring[i];
			}
			prefix[strlen(ReadValue->valuestring)] = '\0';
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "imageFormat");
		if (cJSON_IsString(ReadValue) && (ReadValue->valuestring != NULL)) {
			for (int i = 0; i < strlen(ReadValue->valuestring); i++) {
				imageFormat[i] = ReadValue->valuestring[i];
			}
			imageFormat[strlen(ReadValue->valuestring)] = '\0';
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "InfoMsg");
		if (cJSON_IsString(ReadValue) && (ReadValue->valuestring != NULL)) {
			for (int i = 0; i < strlen(ReadValue->valuestring); i++) {
				InfoMsg[i] = ReadValue->valuestring[i];
			}
			InfoMsg[strlen(ReadValue->valuestring)] = '\0';
		}
		if (!g_disable_sponsor) {
			ReadValue = cJSON_GetObjectItemCaseSensitive(json, "sponsor");
			if ((ReadValue != NULL)) {
				g_disable_sponsor = ReadValue->valueint != 0;
			}
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "AutoReconnect");
		if ((ReadValue != NULL)) {
			autoReconnect = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "FileTransferTimeout");
		if ((ReadValue != NULL)) {
			FTTimeout = ReadValue->valueint != 0;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "FileTransferTimeout");
		if ((ReadValue != NULL)) {
			FTTimeout = ReadValue->valueint != 0;
		}
		if (FTTimeout > 600)
			FTTimeout = 600; // cap at 1 minute
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "KeepAliveInterval");
		if ((ReadValue != NULL)) {
			keepAliveInterval = ReadValue->valueint != 0;
		}
		if (keepAliveInterval >= (FTTimeout - KEEPALIVE_HEADROOM))
			keepAliveInterval = (FTTimeout - KEEPALIVE_HEADROOM);
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "ThrottleMouse");
		if ((ReadValue != NULL)) {
			throttleMouse = ReadValue->valueint != 0;
		}
#ifdef _Gii
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "GiiEnable");
		if ((ReadValue != NULL)) {
			giiEnable = ReadValue->valueint ? true : false;
		}
#endif
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "AutoAcceptIncoming");
		if ((ReadValue != NULL)) {
			fAutoAcceptIncoming = ReadValue->valueint ? true : false;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "AutoAcceptNoDSM");
		if ((ReadValue != NULL)) {
			fAutoAcceptNoDSM = ReadValue->valueint ? true : false;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "RequireEncryption");
		if ((ReadValue != NULL)) {
			fRequireEncryption = ReadValue->valueint ? true : false;
		}
		ReadValue = cJSON_GetObjectItemCaseSensitive(json, "PreemptiveUpdates");
		if ((ReadValue != NULL)) {
			preemptiveUpdates = ReadValue->valueint ? true : false;
		}

		char buffer[_MAX_PATH];
		char buf[_MAX_PATH];
		getAppData(buffer);
		strcat_s(buffer, "\\UltraVNC\\");
		sprintf(buf, "%.15s-%d.vnc", m_host, m_port);
		/*strcat_s(buffer, m_host);
		strcat_s(buffer, "-");
		sprintf(buf, "%d", m_port);
		strcat_s(buffer, ".vnc");*/
		strcat_s(buffer, buf);
		SaveToFile(buffer, true);
		m_pMRU->AddItem(m_host);
		InitMRU(hwnd);
		SetDlgItemText(hwnd, IDC_ALIASNAME_EDIT, m_alias);
	}
}
void SessionDialog::LoadFromFile(char *fname)
{

	TCHAR test[250];
	
	//memset(m_host, 0, 250);
	//memset(m_proxyhost, 0, 250);
		GetPrivateProfileString("connection", "host", "", test, MAX_HOST_NAME_LEN, fname);
		strcpy(m_host, test);
		int tempInt;
		if ((m_port = GetPrivateProfileInt("connection", "port", 0, fname)) == 0)

	GetPrivateProfileString("connection", "proxyhost", "", m_proxyhost, MAX_HOST_NAME_LEN, fname);
	m_proxyport = GetPrivateProfileInt("connection", "proxyport", 0, fname);
	m_fUseProxy = GetPrivateProfileInt("options", "UseProxy", 0, fname) ? true : false;


	GetPrivateProfileString("connection", "alias", "", m_alias, MAX_HOST_NAME_LEN, fname);
	GetPrivateProfileString("connection", "ipAddress", "", m_ipAddress, MAX_HOST_NAME_LEN, fname);
	GetPrivateProfileString("connection", "macAddress", "", m_macAddress, MAX_HOST_NAME_LEN, fname);
	char buf[32];

	unsigned char m_encPasswd[8]; // I added this from another file
	m_encPasswd[0] = '\0';
	if (GetPrivateProfileString("connection", "password", "", buf, 32, fname) > 0) {
		for (int i = 0; i < 12; i++) {
			int x = 0;
			sscanf_s(buf + i * 2, "%2x", &x);
			m_encPasswd[i] = (unsigned char)x;
		}
	}

  for (int i = rfbEncodingRaw; i<= LASTENCODING; i++) {
    char buf[128];
    sprintf_s(buf, "use_encoding_%d", i);
    UseEnc[i] =   readInt(buf, UseEnc[i], fname) != 0;
  }
  int nExistingPreferred = PreferredEncodings.empty() ? rfbEncodingZRLE : PreferredEncodings[0];
  int nPreferredEncoding =	readInt("preferred_encoding", nExistingPreferred,	fname);
  PreferredEncodings.clear();
  PreferredEncodings.push_back(nPreferredEncoding);

  restricted =			readInt("restricted",		restricted,	fname) != 0 ;
  AllowUntrustedServers = readInt("AllowUntrustedServers", AllowUntrustedServers, fname) != 0;
  ViewOnly =			readInt("viewonly",			ViewOnly,		fname) != 0;
  NoStatus =			readInt("nostatus",			NoStatus,		fname) != 0;
  NoHotKeys =			readInt("nohotkeys",			NoHotKeys,	fname) != 0;
  ShowToolbar =			readInt("showtoolbar",			ShowToolbar,		fname) != 0;
  FullScreen =			readInt("fullscreen",		FullScreen,	fname) != 0;
  SavePos =				readInt("SavePos", SavePos, fname) != 0;
  SaveSize =			readInt("SaveSize", SaveSize, fname) != 0;
  GNOME =				readInt("GNOME", GNOME, fname) != 0;
  Directx =				readInt("directx",		Directx,	fname) != 0;
  autoDetect =			readInt("autoDetect", autoDetect, fname) != 0;
  Use8Bit =				readInt("8bit",				Use8Bit,		fname);
  Shared =				readInt("shared",			Shared,		fname) != 0;
  SwapMouse =			readInt("swapmouse",		SwapMouse,	fname) != 0;
  Emul3Buttons =		readInt("emulate3",			Emul3Buttons, fname) != 0;
  JapKeyboard  =		readInt("JapKeyboard",			JapKeyboard, fname) != 0;
  DisableClipboard =	readInt("disableclipboard", DisableClipboard, fname) != 0;
  scaling =				readInt("Scaling", scaling,  fname) != 0;
  fAutoScaling =		readInt("AutoScaling", fAutoScaling,  fname) != 0;
  fAutoScalingEven =    readInt("AutoScalingEven", fAutoScalingEven, fname) != 0;
  fAutoScalingLimit =	readInt("AutoScalingLimit", fAutoScalingLimit, fname) != 0;
  scale_num =			readInt("scale_num",		scale_num,	fname);
  scale_den =			readInt("scale_den",		scale_den,	fname);
  // Tight specific
  requestShapeUpdates =	readInt("cursorshape",		requestShapeUpdates, fname) != 0;
  ignoreShapeUpdates =	readInt("noremotecursor",	ignoreShapeUpdates, fname) != 0;
  int level =			readInt("compresslevel",	-1,				fname);
  if (level != -1) {
	useCompressLevel = true;
	compressLevel = level;
  }
  level =				readInt("quality",			-1,				fname);
  if (level != -1) {
	enableJpegCompression = true;
	jpegQualityLevel = level;
  }
  // Modif sf@2002
  nServerScale =		readInt("ServerScale",		nServerScale,	fname);
  reconnectcounter =	readInt("Reconnect",		reconnectcounter,	fname);
  fEnableCache =		readInt("EnableCache",		fEnableCache,	fname) != 0;
  fEnableZstd =			readInt("EnableZstd",		fEnableZstd, fname);
  quickoption  =		readInt("QuickOption",		quickoption, fname);
  fUseDSMPlugin =		readInt("UseDSMPlugin",		fUseDSMPlugin, fname) != 0;
  m_fUseProxy =			readInt("UseProxy",			m_fUseProxy, fname) != 0;
  allowMonitorSpanning = readInt("allowMonitorSpanning", allowMonitorSpanning, fname);
  changeServerRes = readInt("ChangeServerRes", changeServerRes, fname);
  extendDisplay = readInt("extendDisplay", extendDisplay, fname);
  showExtend = readInt("showExtend", showExtend, fname);
  use_virt = readInt("use_virt", use_virt, fname);
  useAllMonitors = readInt("useAllMonitors", useAllMonitors, fname);

  requestedWidth = readInt("requestedWidth", requestedWidth, fname);
  requestedHeight = readInt("requestedHeight", requestedHeight, fname);

  GetPrivateProfileString("options", "DSMPlugin", "NoPlugin", szDSMPluginFilename, MAX_PATH, fname);
  GetPrivateProfileString("options", "folder", folder, folder, MAX_PATH, fname);
  GetPrivateProfileString("options", "prefix", prefix, prefix, 56, fname);
  GetPrivateProfileString("options", "imageFormat", imageFormat, imageFormat, 56, fname);  
  GetPrivateProfileString("options", "InfoMsg", InfoMsg, InfoMsg, 254, fname);
  if (!g_disable_sponsor) g_disable_sponsor=readInt("sponsor",			g_disable_sponsor, fname) != 0;
  autoReconnect =		readInt("AutoReconnect",	autoReconnect, fname);
  FTTimeout  =			readInt("FileTransferTimeout", FTTimeout, fname);
  if (FTTimeout > 600)
      FTTimeout = 600; // cap at 1 minute
  keepAliveInterval  =	readInt("KeepAliveInterval", keepAliveInterval, fname);
  if (keepAliveInterval >= (FTTimeout - KEEPALIVE_HEADROOM))
      keepAliveInterval = (FTTimeout  - KEEPALIVE_HEADROOM); 
  throttleMouse = readInt("ThrottleMouse", throttleMouse, fname); // adzm 2010-10
#ifdef _Gii
  giiEnable = readInt("GiiEnable", (int)giiEnable, fname) ? true : false;
#endif
  fAutoAcceptIncoming = readInt("AutoAcceptIncoming", (int)fAutoAcceptIncoming, fname) ? true : false;
  fAutoAcceptNoDSM = readInt("AutoAcceptNoDSM", (int)fAutoAcceptNoDSM, fname) ? true : false;
  fRequireEncryption = readInt("RequireEncryption", (int)fRequireEncryption, fname) ? true : false;
  preemptiveUpdates = readInt("PreemptiveUpdates", (int)preemptiveUpdates, fname) ? true : false;

  GetPrivateProfileString("connection", "proxyhost", "", m_proxyhost, MAX_HOST_NAME_LEN, fname);
  m_proxyport = GetPrivateProfileInt("connection", "proxyport", 0, fname);

}

void SessionDialog::getAppData(char * buffer)
{
	BOOL result = SHGetSpecialFolderPathA( NULL, buffer, CSIDL_APPDATA, false );
}

void SessionDialog::IfHostExistLoadSettings(char *hostname)
{
	
	TCHAR tmphost[MAX_HOST_NAME_LEN];
	int port;
	ParseDisplay(hostname, tmphost, MAX_HOST_NAME_LEN, &port);
	char fname[_MAX_PATH];
	int disp = PORT_TO_DISPLAY(port);
	sprintf_s(fname, "%.15s-%d.vnc", tmphost, (disp > 0 && disp < 100) ? disp : port);
	char buffer[_MAX_PATH];
	getAppData(buffer);
	strcat_s(buffer,"\\UltraVNC\\");
	strcat_s(buffer,fname);
	FILE *file = fopen(buffer, "r");
	if (strlen(hostname) != 0 && file ) {
		fclose(file);
		if (fname[strlen(fname) - 4] == 'j') {
			LoadFromJson(buffer);
		}
		else {
			LoadFromFile(buffer);
		}
	}
	else
		if (fname[strlen(fname) - 4] == 'j') {
			LoadFromJson(m_pOpt->getDefaultOptionsFileName());

		}
		else {
			LoadFromFile(m_pOpt->getDefaultOptionsFileName());
		}
}

void SessionDialog::SetDefaults()
{
	SettingsFromUI();
	ViewOnly = false;
	FullScreen = false;
	SavePos = false;
	SaveSize = false;
	GNOME = false;
	Directx = false;
	autoDetect = false;
	Use8Bit = rfbPFFullColors; //false;
	ShowToolbar = true;
	NoStatus = false;
	NoHotKeys = false;
	PreferredEncodings.clear();
	PreferredEncodings.push_back(rfbEncodingUltra2);
	JapKeyboard = false;
	SwapMouse = false;
	Emul3Buttons = true;
	Shared = true;
	DisableClipboard = false;
	scaling = false;
	fAutoScaling = false;
	fAutoScalingEven = false;
	fAutoScalingLimit = false;
	scale_num = 100;
	scale_den = 100;  
	// Modif sf@2002 - Server Scaling
	nServerScale = 1;
	reconnectcounter = 3;
	fEnableCache = false;
	fEnableZstd = true;
	listening = false;
	listenport = INCOMING_PORT_OFFSET;
	m_ipAddress[0] = '\0';
	m_macAddress[0] = '\0';
	m_alias[0] = '\0';
	restricted = false;
	AllowUntrustedServers = false;
	// Tight specific
	useCompressLevel = true;
	compressLevel = 6;		
	enableJpegCompression = true;
	jpegQualityLevel = 8;
	requestShapeUpdates = true;
	ignoreShapeUpdates = false;
	quickoption = 1;				// sf@2002 - Auto Mode as default
	fUseDSMPlugin = false;
	oldplugin=false;
	allowMonitorSpanning = 0;
	changeServerRes = 0;
	extendDisplay = 0;
	showExtend = 0;
	use_virt = 0;
	useAllMonitors =0;
	requestedWidth = 0;
	requestedHeight = 0;
	_tcscpy_s(prefix, "ultravnc_");
	_tcscpy_s(imageFormat, ".jpeg");
	fAutoAcceptIncoming = false;
	fAutoAcceptNoDSM = false;
	fRequireEncryption = false;
	preemptiveUpdates = false;
	scale_num = 100;
	scale_den = 100;
	scaling = false; 
	autoReconnect = 3; 
	fExitCheck = false; //PGM @ Advantig
	FTTimeout = FT_RECV_TIMEOUT;
	keepAliveInterval = KEEPALIVE_INTERVAL;
	throttleMouse = 0; // adzm 2010-10 
	setdefaults = true;
	SettingsToUI();
	setdefaults = false;
	giiEnable = false;

}