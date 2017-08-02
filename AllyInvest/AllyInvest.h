#pragma once

#include <windows.h>  // FARPROC

typedef double DATE;			//prerequisite for using trading.h
#include "include\trading.h"	// enter your path to trading.h (in your Zorro folder)
//#include "include\functions.h"	// <- some #define's copied from functions.h

#ifdef ALLYINVEST_EXPORTS 
#define DLLFUNC extern __declspec(dllexport)
#define DLLFUNC_C extern "C" __declspec(dllexport)
#else  
#define DLLFUNC extern __declspec(dllimport)
#define DLLFUNC_C extern "C" __declspec(dllimport)
#endif  

namespace AllyInvest
{
	int(__cdecl *BrokerError)(const char* txt) = NULL;
	int(__cdecl *BrokerProgress)(const int percent) = NULL;
	int(__cdecl *http_send)(char* url, char* data, char* header) = NULL;
	long(__cdecl *http_status)(int id) = NULL;
	long(__cdecl *http_result)(int id, char* content, long size) = NULL;
	void(__cdecl *http_free)(int id);

	////////////////////////////////////////////////////////////////
	// functions exposed for unit testing using Zorro scripts
	DLLFUNC_C bool CGetURL(char* output, int n, int calltype, int operation, int responseMode, char* args);
	DLLFUNC_C bool CGetResponse(char* output, int n, int calltype, int operation, int responseMode, char* args, char* data);
	DLLFUNC_C bool CSaveResponse(char* filename, int calltype, int operation, int responseMode, char* args, char* data);
	DLLFUNC_C void CSaveStockOrderResponse(char* filename, char* AllyAsset, int nAmount, double dStopDist);
	DLLFUNC_C void CSaveOptionOrderResponse(char* filename, char* AllyAsset, int nAmount, double dStopDist);
	DLLFUNC_C void CAddLeg(char* AllyAsset, int nAmount, double dStopDist);
	DLLFUNC_C void CSaveMultiLegOptionsOrderResponse(char* filename);
	DLLFUNC_C void CTestParseHoldings();


	// functions exposed for unit testing with AllyTester.exe console app
	DLLFUNC void TestDate();
	DLLFUNC void TestAssetList();
	DLLFUNC void TestGetAllyAsset();
	DLLFUNC void TestMakeStockOrder();
	DLLFUNC void TestMakeOptionOrder();
	DLLFUNC void TestMakeMultiLegOptionOrder();
	DLLFUNC void TestIsCaseInsensitveMatch();

	// zorro functions
	DLLFUNC_C int BrokerOpen(char* Name, FARPROC fpError, FARPROC fpProgress);
	DLLFUNC_C void BrokerHTTP(FARPROC fpSend, FARPROC fpStatus, FARPROC fpResult, FARPROC fpFree);
	DLLFUNC_C int BrokerLogin(char* User, char* Pwd, char* Type, char* Account);
	DLLFUNC_C int BrokerTime(DATE *pTimeGMT);
	DLLFUNC_C int BrokerAsset(char* Asset, double *pPrice, double *pSpread, double *pVolume, double *pPip, double *pPipCost, double *pLotAmount, double *pMarginCost, double *pRollLong, double *pRollShort);
	DLLFUNC_C int BrokerHistory2(char* Asset, DATE tStart, DATE tEnd, int nTickMinutes, int nTicks, T6* ticks);  // only supports stocks, no option history available.
	DLLFUNC_C int BrokerBuy(char* Asset, int nAmount, double dStopDist, double *pPrice);
	DLLFUNC_C double BrokerCommand(int Command, DWORD dwParameter);

	// extra BrokerCommand function for multi-leg options
	//
	// the legs must have identical underlying assets and expire in the same month - otherwise, the multi-leg order will be rejected.
	// conceptual example:
	//
	// #define SET_COMBO_LEGS  137
	//
	// brokerCommand(SET_COMBO_LEGS,2);
	// // buy/sell option 1 to open/close
	// // buy/sell option 2 to open/close  // <-- Order executes when this request is received
	//
	// brokerCommand(SET_COMBO_LEGS,3);
	// // buy/sell option 1 to open/close
	// // buy/sell option 2 to open/close
	// // buy/sell option 3 to open/close  // <-- Order executes when this request is received
	//
	// brokerCommand(SET_COMBO_LEGS,4);
	// // buy/sell option 1 to open/close
	// // buy/sell option 2 to open/close
	// // buy/sell option 3 to open/close
	// // buy/sell option 4 to open/close  // <-- Order executes when this request is received
	//
}