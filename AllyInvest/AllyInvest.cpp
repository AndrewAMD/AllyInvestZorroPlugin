// AllyInvest.cpp : Defines the exported functions for the DLL application.
//
// AllyInvest plugin for Zorro Automated Trading System
// Written by Andrew Dolder
// 
// Ally Invest (formerly Tradeking, which has been acquired by Ally Financial, Inc.)
//

#include "stdafx.h"

// all dependencies are MIT/BSD-licensed or similar
#include "pugixml.hpp"							// pugixml - write and parse xml
#include "liboauthcpp\liboauthcpp.h"			// liboauthcpp - OAuth 1.0a authentication

#include "AllyInvest.h"

// standard library
#include <string>
#include <sstream>
#include <vector>
#include <iomanip> // setprecision
#include <chrono>
#include <thread>
#include <fstream>  // for diagnostics
#include <iostream> // for unit tests only
#include <algorithm> // transform

//#define LOOP_MS	200	// repeat a failed command after this time (ms)
//#define WAIT_MS	10000	// wait this time (ms) for success of a command
#define INTERVAL_BROKERTIME_MS	15000// wait this time (ms) between clock calls
#define INTERVAL_BROKERASSET_MS 1500 // wait this time (ms) between quote calls
#define INTERVAL_QUOTA_EXCEEDED_MS 1000 // wait this time (ms) between any call when quota has been exceeded
#define ATTEMPTS_QUOTA_EXCEEDED 70  // number of attempts to make when quota has been exceeded
#define INTERVAL_BROKERACCOUNT_MS 1500 // wait this time (ms) between account calls
#define INTERVAL_GET_POSITION_MS 1500 // wait this time (ms) between GET_POSITION calls

#define PLUGIN_VERSION	2

// calltype
#define HGET	100
#define HPOST	200
#define HDELETE	300

// operation
#define ACCOUNTS					01
#define ACCOUNTS_BALANCES			02
#define ACCOUNTS_ID					03
#define ACCOUNTS_ID_BALANCES		04
#define ACCOUNTS_ID_HISTORY			05
#define ACCOUNTS_ID_HOLDINGS		06
#define ACCOUNTS_ID_ORDERS			11
#define ACCOUNTS_ID_ORDERS_PREVIEW	12
#define MARKET_CLOCK				21
#define MARKET_EXT_QUOTES			22
#define MARKET_NEWS_SEARCH			23
#define MARKET_NEWS_ID				24
#define MARKET_OPTIONS_SEARCH		25
#define MARKET_OPTIONS_STRIKES		26
#define MARKET_OPTIONS_EXPIRATIONS	27
#define	MARKET_TIMESALES			28
#define MARKET_TOPLISTS				29
#define	MEMBER_PROFILE				31
#define UTILITY_STATUS				41
#define UTILITY_VERSION				42
#define WATCHLISTS					51
#define WATCHLISTS_ID				52
#define WATCHLISTS_ID_SYMBOLS		53
#define WATCHLISTS_ID_SYMBOLS_SYM	54

#define DIAGNOSTICS_HTTPONY			91

// responseMode
#define	JSON	1000
#define XML		2000

// errors
#define ERROR_FIXML_MESSAGE_LOAD_FAIL	99001
#define ERROR_BAD_TIME_ZONE				99002
#define ERROR_BAD_STRINGMODE			99003
#define ERROR_BAD_OPTIONS_PARSE			99004
#define ERROR_BAD_GET_POSITIONS			99005

// stringMode
#define STR_CLOCK						8801
#define STR_FIX							8802
#define STR_FIX2						8803
#define STR_TS							8804
#define STR_DATEONLY					8805
#define STR_YYYYMMDD					8806
#define STR_YYYY_MM_DD					8807

//from functions.h
#define GET_TIME			5	// Last incoming tick time (last known server time in MT4 time zone)
#define GET_DIGITS		12	// Count of digits after decimal point 
#define GET_STOPLEVEL	14	// Stop level in points.
#define GET_STARTING		20	// Market starting date (usually used for futures).
#define GET_EXPIRATION	21	// Market expiration date (usually used for futures).
#define GET_TRADEALLOWED 22	// Trade is allowed for the symbol.
#define GET_MINLOT		23	// Minimum permitted amount of a lot.
#define GET_LOTSTEP		24	// Step for changing lots.
#define GET_MAXLOT		25	// Maximum permitted amount of a lot.
#define GET_MARGININIT	29	// Initial margin requirements for 1 lot.
#define GET_MARGINMAINTAIN	30	// Margin to maintain open positions calculated for 1 lot.
#define GET_MARGINHEDGED	31	// Hedged margin calculated for 1 lot.
#define GET_MARGINREQUIRED	32	// Free margin required to open 1 lot for buying.
#define GET_DELAY			41
#define GET_WAIT			42
#define GET_TYPE			50	// Asset type. 
#define GET_COMPLIANCE	51 // NFA compliance.
#define GET_NTRADES		52 // Number of open trades
#define GET_POSITION		53	// Open net lots per asset 
#define GET_ACCOUNT		54	// Account number (string)
#define GET_BOOKASKS		60	// Ask volume in the order book
#define GET_BOOKBIDS		61	// Bid volume in the order book
#define GET_BOOKPRICE	62	// Price quote per price rank
#define GET_BOOKVOL		63	// Volume per price rank
#define GET_OPTIONS		64 // Option chain
#define GET_FUTURES		65	
#define GET_FOP			66	
#define GET_UNDERLYING	67	

#define SET_PATCH		128 // Work around broker API bugs
#define SET_SLIPPAGE	129 // Max adverse slippage for orders
#define SET_MAGIC		130 // Magic number for trades
#define SET_ORDERTEXT 131 // Order comment for trades
#define SET_SYMBOL	132 // set asset symbol for subsequent commands
#define SET_MULTIPLIER 133 // set option/future multiplier filter
#define SET_CLASS		134 // set trading class filter
#define SET_LIMIT		135 // set limit price for entry limit orders
#define SET_HISTORY	136 // set file name for direct history download
	#define SET_COMBO_LEGS  137 // !!!!!!!!!!!!!!!!! *** New Function: set number of legs in a combo order, int 2,3,4 accepted. *** Returns 1 if acknowledged, 0 if failure. !!!!!!!!!!!!!!!!!
	#define SET_DIAGNOSTICS 138 // !!!!!!!!!!!!!!!!! *** New Function: enable(1) or disable(0) dumping server communications in .\Log folder, int 2,3,4 accepted. *** Returns 1 if acknowledged, 0 if failure.
#define SET_DELAY		169
#define SET_WAIT		170
#define SET_LOCK		171
#define SET_COMMENT	180 // Comment on the chart

#define PLOT_STRING	188	// send a string to a plot object
#define PLOT_REMOVE	260
#define PLOT_REMOVEALL 261
#define PLOT_HLINE	280	// plot to the MT4 chart window
#define PLOT_TEXT		281
#define PLOT_MOVE		282

#define DO_EXERCISE	300	// exercise option

// from functions.h

#define CALL			(1<<0)
#define PUT				(1<<1)
#define EUROPEAN		(1<<2)
//#define BINARY	8	// shared with mode()
#define FUTURE			(1<<5)
#define ONLYW3			(1<<7)	// only contracts in 3rd week
#define OPTION			(CALL|PUT|EUROPEAN)

// enums for FIXML
enum FixmlMessagetype_enum_t {
	mt_null,
	mt_Order,
	mt_OrdCxlRplcReq,
	mt_OrdCxlReq,
	mt_ExecRpt,
};
enum FixmlOrdType_enum_t
{
	ot_null = 0,
	ot_Market = '1',
	ot_Limit = '2',
	ot_Stop = '3',
	ot_StopLimit = '4',
	//ot_MarketOnClose = '5', //no longer used
	//ot_WithOrWithout = '6',
	//ot_LimitOrBetter = '7',
	//ot_LimitWithOrWithout = '8',
	//ot_OnBasis = '9',
	//ot_OnClose = 'A', // no longer used
	//ot_LimitOnClose = 'B',
	//ot_ForexMarket = 'C',
	//ot_PreviouslyQuoted = 'D',
	//ot_PreviouslyIndicated = 'E',
	//ot_ForexLimit = 'F',
	//ot_ForexSwap = 'G',
	//ot_ForexSwapNoLongerUsed = 'H',
	//ot_Funari = 'I',
	//ot_MarketIfTouched = 'J',
	//ot_MarketWLOAL = 'K',
	//ot_PrevFVP = 'L',
	//ot_NextFVP = 'M',
	//ot_Pegged = 'P',
	//ot_CounterOrderSelection = 'Q',
};
enum FixmlTimeInForce_enum_t
{
	tif_DayOrder = '0',
	tif_GoodTilCanceled = '1',
	tif_AtTheOpening = '2',
	//tif_ImmediateOrCancel = '3',
	//tif_FillOrKill = '4',
	//tif_GoodTillCrossing = '5',
	//tif_GoodTillDate = '6',
	tif_MarketOnClose = '7',
	//tif_GoodThroughCrossing = '8',
	//tif_AtCrossing = '9',

	tif_null = 'Z',
};
enum FixmlSide_enum_t
{
	s_null = 0,
	s_Buy = '1',
	s_Sell = '2',
	//s_BuyMinus = '3',
	//s_SellPlus = '4',
	s_SellShort = '5',
	//s_SellShortExempt = '6',
	//s_Undisclosed = '7',
	//s_Cross = '8',
	//s_CrossShort = '9',
	//s_CrossShortExempt = 'A',
	//s_AsDefined = 'B',
	//s_Opposite = 'C',
	//s_Subscribe = 'D',
	//s_Redeem = 'E',
	//s_Lend = 'F',
	//s_Borrow = 'G',
};
enum FixmlAccountType_enum_t
{
	at_null = 0,
	at_Cash = '1', // a.k.a. Cash or Customer Side
	at_MarginLong = '2', // a.k.a. Margin Long or Non-Customer Side
	//at_HouseTrader = '3',
	//at_FloorTrader = '4',
	at_MarginShort = '5', // a.k.a. Margin Short or Ally Buy to Cover
	//at_NCCrossMargin = '6',
	//at_HTCrossMargin = '7',
	//at_JBO = '8',
};
enum FixmlPositionEffect_enum_t
{
	pos_null = 0,
	pos_Close = 'C',
	//pos_Default = 'D',
	//pos_FIFO = 'F',
	//pos_CloseButNotifyOnOpen = 'N',
	pos_Open = 'O',
	//pos_Rolled = 'R',
};
enum FixmlOrdStatus_enum_t
{
	os_New = '0',
	os_PartiallyFilled = '1',
	os_Filled = '2',
	os_DoneForDay = '3',
	os_Canceled = '4',
	//os_Replaced = '5',
	os_PendingCancel = '6',
	os_Stopped = '7',
	os_Rejected = '8',
	os_Suspended = '9',
	os_PendingNew = 'A',
	os_Calculated = 'B',
	os_Expired = 'C',
	os_AcceptedForBidding = 'D',
	os_PendingReplace = 'E',
};


namespace AllyInvest
{
	// function prototypes 
	bool GetResponse(std::string* Output, int calltype, int operation, int responseMode, char* args, char* data);
	bool ConfirmAccount(std::string parseThis);
	void SaveMessage(std::string msg, std::string msgname);
	DATE convertTime(__time32_t t32);
	__time32_t convertTime(DATE date);
	void SetTimeZone(std::string TZ);
	DATE currentTime();
	DATE convertTime(std::string timeString, int stringMode);
	std::string convertTime(DATE date, int stringMode);
	int convertTimeInt(DATE date);
	bool IsErrorMessage(std::string parseThis, std::string *output);
	std::string GetDoubleString(double a);
	float GetBH2TickClose(char* Asset);
	bool isCaseInsensitveMatch(std::string a, std::string b);

	
	


	//structs, classes, rate-limiting workarounds, etc
	struct OptionLeg
	{
	public:
		std::string AllyAsset = "";
		int nAmount = 0;
		double dStopDist = 0;
	};
	class StopWatch
	{
	private:
		clock_t clock_origin = NULL;

		static double diffclock(clock_t t0, clock_t t)
		{
			return ((t - t0) / (CLOCKS_PER_SEC / 1000));
		}

	public: 
		void reset()
		{
			clock_origin = clock(); return;
		}
		void nullify()
		{
			clock_origin = NULL; return;
		}
		bool is_null()
		{
			if (!clock_origin) return true;
			else return false;
		}
		double milliseconds_passed()
		{
			if (!clock_origin) reset();
			return(diffclock(clock_origin, clock()));
		}
		DATE zorroDate_passed()
		{
			return(milliseconds_passed() / (1000.*24.*60.*60.));
		}

	};


	class AssetList
	{
	private:
		class StockEntry
		{
		public:
			std::string asset = "";
			double price = 0.;
			double spread = 0.;
		};

		class OptionEntry
		{
		public:
			std::string asset = "";
			int ExpirationDay = 0;
			double price = 0.;
			double spread = 0.;
		};
		
		std::vector<StockEntry> stockEntries;
		std::vector<OptionEntry> optionEntries;
		bool isInitialized = false;

		void initialize()
		{
			if (isInitialized) return;
			stockEntries.reserve(500);
			optionEntries.reserve(500);
			isInitialized = true;
		}

		
	public:
		
		void updateStock(std::string Asset, double pPrice, double pSpread)
		{
			initialize();
			// is asset in the list?
			for (auto & this_entry : stockEntries)
			{
				if (!isCaseInsensitveMatch(this_entry.asset,Asset)) continue;	// skip if case insensitve match fails, i.e. MSFT/msft matches

				this_entry.price = pPrice;
				this_entry.spread = pSpread;
				return;
			}
			//not found, push back
			StockEntry new_entry;
			new_entry.asset = Asset;
			new_entry.price = pPrice;
			new_entry.spread = pSpread;
			stockEntries.push_back(new_entry);

			return;
		}
		void updateOption(std::string Asset, double pPrice, double pSpread)
		{
			initialize();
			// is asset in the list?
			for (auto & this_entry : optionEntries)
			{
				if (!isCaseInsensitveMatch(this_entry.asset, Asset)) continue;	// skip if case insensitve match fails, i.e. MSFT/msft matches
				this_entry.price = pPrice;
				this_entry.spread = pSpread;
				return;
			}
			//not found, push back
			OptionEntry new_entry;

			// example: "SPY170714C00205000" = SPY, 20170714, CALL, STRIKE = 00205.000
			
			try { new_entry.ExpirationDay = 20000000 + std::stoi(Asset.substr(Asset.length() - 15, 6)); }
			catch (...) { return; }
			new_entry.asset = Asset;
			new_entry.price = pPrice;
			new_entry.spread = pSpread;
			optionEntries.push_back(new_entry);

			return;
		}


		bool CanGetValues(std::string Asset, double *outputPrice, double *outputSpread)
		{
			// is asset in the list?
			for (auto & this_entry : stockEntries)
			{
				if (!isCaseInsensitveMatch(this_entry.asset, Asset)) continue;	// skip if case insensitve match fails, i.e. MSFT/msft matches

				*outputPrice = this_entry.price;
				*outputSpread = this_entry.spread;
				return true;
			}
			for (auto & this_entry : optionEntries)
			{
				if (!isCaseInsensitveMatch(this_entry.asset, Asset)) continue;	// skip if case insensitve match fails, i.e. MSFT/msft matches

				*outputPrice = this_entry.price;
				*outputSpread = this_entry.spread;
				return true;
			}

			return false;
		}

		bool DoesAssetExist(std::string find_this_asset)
		{
			// is asset in the list?
			for (auto & this_entry : stockEntries)
			{
				if (!isCaseInsensitveMatch(this_entry.asset, find_this_asset)) continue;	// skip if case insensitve match fails, i.e. MSFT/msft matches

				return true;
			}
			for (auto & this_entry : optionEntries)
			{
				if (!isCaseInsensitveMatch(this_entry.asset, find_this_asset)) continue;	// skip if case insensitve match fails, i.e. MSFT/msft matches
				return true;
			}
			//not found
			return false;
		}

		int DeleteEntry(std::string inputAsset)
		{
			if (inputAsset == "")return false; // nothing was deleted.
			unsigned int i = 0; int entries_deleted = 0;
			while (i < stockEntries.size())
			{
				if (!isCaseInsensitveMatch(stockEntries[i].asset, inputAsset)) { i++;  continue; }  // skip if case insensitve match fails, i.e. MSFT/msft matches
				else
				{
					stockEntries.erase(stockEntries.begin() + i); 
					entries_deleted++;
				}
			}

			i = 0;
			while (i < optionEntries.size())
			{
				if (!isCaseInsensitveMatch(optionEntries[i].asset, inputAsset)) { i++;  continue; }  // skip if case insensitve match fails, i.e. MSFT/msft matches
				else
				{
					optionEntries.erase(optionEntries.begin() + i);
					entries_deleted++;
				}
			}

			return entries_deleted;
		}

		void DeleteAllOptionEntries()
		{
			optionEntries.clear();
			return;
		}

		int PurgeOldOptionEntries()
		{
			unsigned int i = 0; int entries_deleted = 0;
			int Today = convertTimeInt(currentTime());
			if (!Today) return 0;

			while (i < optionEntries.size())
			{
				if (Today<=optionEntries[i].ExpirationDay) { i++;  continue; }  // skip if options did not expire yet
				optionEntries.erase(optionEntries.begin() + i);
				entries_deleted++;
			}
			return entries_deleted;
		}

		
		std::string GetAssetCSV()
		{
			std::ostringstream oss; int i = 0;
			for (auto & this_entry : stockEntries)
			{
				oss << this_entry.asset << ",";
				i++;
			}
			for (auto & this_entry : optionEntries)
			{
				oss << this_entry.asset << ",";
				i++;
			}

			if (i == 0) return("");
			std::string csv = oss.str();
			return(csv.substr(0,csv.length()-1));
		}

	};
	
	//globals
	std::string consumerKey = "";			// 44 characters as of 2017-05
	std::string consumerSecret = "";		// 44 characters as of 2017-05
	std::string oauthToken = "";			// 44 characters as of 2017-05
	std::string oauthTokenSecret = "";		// 44 characters as of 2017-05
	std::string accid = "";					// 8 digits as of 2017-05
	bool diag = false;						// diagnostics mode.  Saves XML transmissions to and from the server under the .\Log folder as xml files.
	DATE zorroDate0 = 0;					// clock datum reference, set every time clock response received, for recording most recent response.
	bool marketIsOpen=false;				// was the market open last time clock was called?
	AssetList assetList;					// asset subscriptions are kept here.
	std::string LastBrokerAsset = "";		// for use with GET_UNDERLYING
	int numLegs = 0;							// for multi-leg options. will count down to zero for every BrokerBuy call.
	std::vector<OptionLeg> optionLegs;		// for multi-leg option orders
	int tradeCounter = 2;					// this is a counter for trade ID's. Note: Ally is NFA compliant.


	DLLFUNC void TestAssetList()
	{
		assetList.updateStock("MSFT", 125.0, 0.01); 
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.updateStock("AAPL", 150.0, 0.02);
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.updateStock("SPY", 175.0, 0.03);
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.DeleteEntry("MSFT");
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.updateStock("MSFT", 125.0, 0.01);
		std::cout << assetList.GetAssetCSV() << std::endl;

		assetList.updateOption("SPY170721C00100000", 144.19, 0.01);
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.updateOption("SPY170721C00105000", 139.87, 0.01);
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.DeleteEntry("SPY170721C00100000");
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.updateOption("SPY170714C00205000", 40.74, 0.01);
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.updateOption("SPY170721C00100000", 144.19, 0.01);
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.PurgeOldOptionEntries();
		std::cout << assetList.GetAssetCSV() << std::endl;
		assetList.DeleteAllOptionEntries();
		std::cout << assetList.GetAssetCSV() << std::endl;
		return;
	}

	////////////////////////////////////////////////////////////////
	DLLFUNC_C int BrokerOpen(char* Name, FARPROC fpError, FARPROC fpProgress)
	{
		strcpy_s(Name, 32, "Ally Invest");
		(FARPROC&)BrokerError = fpError;
		(FARPROC&)BrokerProgress = fpProgress;
		return PLUGIN_VERSION;
	}

	DLLFUNC_C void BrokerHTTP(FARPROC fpSend, FARPROC fpStatus, FARPROC fpResult, FARPROC fpFree)
	{
		(FARPROC&)http_send = fpSend;
		(FARPROC&)http_status = fpStatus;
		(FARPROC&)http_result = fpResult;
		(FARPROC&)http_free = fpFree;
		return;
	}

	DLLFUNC_C bool CGetURL(char* output, int n, int calltype, int operation, int responseMode, char* args)
	{
		std::string a = "https://api.tradeking.com/v1/";
		std::string b;
		std::string url;
		std::string argstring;
		std::string Args = args;
		std::string Output;
		if (args == "")argstring = "";
		else { argstring = "?" + Args; }

		bool oauth = true;
		if (responseMode == JSON) b = ".json"; 
		else if (responseMode == XML) b = ".xml";
		else return false; //fail

		switch (operation)
		{
		case ACCOUNTS:						url = a + "accounts" + b;									break;
		case ACCOUNTS_BALANCES:				url = a + "accounts/balances" + b;							break;
		case ACCOUNTS_ID:					url = a + "accounts/" + accid + b;							break;
		case ACCOUNTS_ID_BALANCES:			url = a + "accounts/" + accid + "/balances" + b;			break;
		case ACCOUNTS_ID_HISTORY:			url = a + "accounts/" + accid + "/history" + b;				break;
		case ACCOUNTS_ID_HOLDINGS:			url = a + "accounts/" + accid + "/holdings" + b;			break;
		case ACCOUNTS_ID_ORDERS:			url = a + "accounts/" + accid + "/orders" + b;				break; // GET and POST
		case ACCOUNTS_ID_ORDERS_PREVIEW:	url = a + "accounts/" + accid + "/orders/preview" + b;		break;
		case MARKET_CLOCK:					url = a + "market/clock" + b;		oauth = false;			break;
		case MARKET_EXT_QUOTES:				url = a + "market/ext/quotes" + b;							break;
		case MARKET_NEWS_SEARCH:			url = a + "market/news/search" + b;							break;
		//case MARKET_NEWS_ID:				url = a + "market/news/" + newsid + b;						break;
		case MARKET_OPTIONS_SEARCH:			url = a + "market/options/search" + b;						break;
		case MARKET_OPTIONS_STRIKES:		url = a + "market/options/strikes" + b;						break;
		case MARKET_OPTIONS_EXPIRATIONS:	url = a + "market/options/expirations" + b;					break;
		case MARKET_TIMESALES:				url = a + "market/timesales" + b;							break;
		case MARKET_TOPLISTS:				url = a + "market/toplists" + b;							break;
		case MEMBER_PROFILE:				url = a + "member/profile" + b;								break;
		case UTILITY_STATUS:				url = a + "utility/status" + b;		oauth = false;			break;
		case UTILITY_VERSION:				url = a + "utility/version" + b;	oauth = false;			break;
		//case WATCHLISTS:					url = a + "watchlists" + b;									break; // GET and POST
		//case WATCHLISTS_ID:				if (wlid == "")return false;//fail
		//									url = a + "watchlists/" + wlid + b;							break; // GET and DELETE
		//case WATCHLISTS_ID_SYMBOLS:		if (wlid == "")return false;//fail
		//									url = a + "watchlists/" + wlid + "/symbols" + b;			break; // POST
		//case WATCHLISTS_ID_SYMBOLS_SYM:	if (wlid == "" || wlsymbol=="" )return false;  //fail
		//									url = a + "watchlists/" + wlid + "/symbols/" +wlsymbol +b;	break; // DELETE
		case DIAGNOSTICS_HTTPONY:			url = "http://localhost:8000";		oauth = false;			break;
		default: return false;
		}

		if (oauth)
		{
			std::string oaurl = "";
			// Initialization
			OAuth::Client::initialize();
			OAuth::Consumer consumer(consumerKey, consumerSecret);
			OAuth::Token token(oauthToken, oauthTokenSecret);
			OAuth::Client oauth(&consumer, &token);
			if (calltype == HGET)			oaurl = oauth.getURLQueryString(OAuth::Http::Get,		url + argstring);
			else if (calltype == HPOST)		oaurl = oauth.getURLQueryString(OAuth::Http::Post,		url + argstring);
			else if (calltype == HDELETE)	oaurl = oauth.getURLQueryString(OAuth::Http::Delete,	url + argstring);
			Output = url + "?" + oaurl;
			strcpy_s(output, n, Output.c_str());
			return true;//success
		}
		else
		{
			Output = url + argstring;
			strcpy_s(output, n, Output.c_str());
			return true;//success
		}
	}

	bool isCaseInsensitveMatch(std::string a, std::string b)
	{
		std::transform(a.begin(), a.end(), a.begin(), toupper);
		std::transform(b.begin(), b.end(), b.begin(), toupper);
		if (a == b)return true;
		else return false;
	}
	DLLFUNC void TestIsCaseInsensitveMatch()
	{
		std::string a = "MSFT", b = "msft", c = "aapl", d = "SPY";
		std::cout << "a: " << a << ", b: " << b << ", c: " << c << ", d: " << d << std::endl;
		std::cout << "(a==b): " << std::boolalpha << isCaseInsensitveMatch(a, b) << std::endl;
		std::cout << "(c==d): " << std::boolalpha << isCaseInsensitveMatch(c, d) << std::endl;
	}

	bool IsErrorMessage(std::string parseThis, std::string *output)
	{
		//check for html document instead of expected XML
		if (parseThis.find("<!DOCTYPE html>") != std::string::npos) {
			*output = "Server returned HTML document instead of XML!";
			return true;
		}

		//check for error string in XML document
		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { return false; }
		pugi::xml_node string = doc.child("string");
		std::ostringstream oss_msg;
		oss_msg << doc.child_value("string");
		std::string msg = oss_msg.str();
		if (msg == "") return false;
		else {
			*output = msg;
			return true;
		}
	}

	bool IsQuotaExceeded(std::string parseThis)
	{
		if (parseThis.find("quota exceeded") != std::string::npos) {
			return true;
		}
		else return false;
	}

	bool GetResponse(std::string* Output, int calltype, int operation, int responseMode, char* args, char* data)
	{
		bool quota_exceeded = false;
		for (int i = 1; i <= ATTEMPTS_QUOTA_EXCEEDED; i++)
		{

			char url[10001]; int n;
			if (!CGetURL(url, 10000, calltype, operation, responseMode, args)) { BrokerError("\nError producing URL."); return false; }
			int id;
			if (calltype == HPOST && operation == ACCOUNTS_ID_ORDERS) id = http_send(url, data, "TKI_OVERRIDE: true");
			else if (calltype == HGET)id = http_send(url, 0, 0);
			else if (calltype == HPOST) id = http_send(url, data, 0);
			else if (calltype == HDELETE)
			{
				std::string Deletedata; char* deletedata;
				if (data == "") deletedata = "#DELETE";
				else { Deletedata = "#DELETE " + (std::string)data; deletedata = (char*)Deletedata.c_str(); }
				id = http_send(url, deletedata, 0);
			}
			else return false;
			if (!id)
			{
				if (diag)BrokerError("\nError: Cannot connect to server.");
				return 0; //failure
			};
			while (!http_status(id)) {
				Sleep(100); // wait for the server to reply
				if (!BrokerProgress(1))
				{
					if (diag)BrokerError("\nBrokerprogress returned zero. Aborting...");
					http_free(id); //always clean up the id!
					return 0; //failure
				} // print dots, abort if returns zero.
			}
			n = http_status(id);
			if (n > 0)  //transfer successful?
			{
				char* output;
				output = (char *)malloc(n + 1);
				http_result(id, output, n);   //get the replied IP
				*Output = output;
				free(output); //free up memory allocation
				http_free(id); //always clean up the id!

				// HACK - trim off odd characters at end of message, up to five attempts
				for (int ii = 1; ii<= 5; ii++)
				{
					if (Output->substr(Output->length()-1, 1) == ">") break;

					*Output = Output->substr(0, Output->length() - 1);
				}

				if (IsQuotaExceeded(*Output))
				{
					if (!quota_exceeded) {
						BrokerError("Quota exceeded! Please wait...");
						quota_exceeded = true;
					}
					if (!BrokerProgress(1))
					{
						if (diag)BrokerError("\nBrokerprogress returned zero. Aborting...");
						http_free(id); //always clean up the id!
						return 0; //failure
					}
					*Output = "";
					//if (diag)BrokerError("Trying again..");
					Sleep(INTERVAL_QUOTA_EXCEEDED_MS);
					continue;
				}


				return 1; //success
			}
			else
			{
				if (diag)BrokerError("\nError during transfer from server.");
				http_free(id); //always clean up the id!
				return 0; //failure
			}

		}
		return 0;
	}

	// this is just a wrapper for testing
	DLLFUNC_C bool CGetResponse(char* output, int n, int calltype, int operation, int responseMode, char* args, char* data)
	{
		std::string Output;
		if (!GetResponse(&Output, calltype, operation, responseMode, args, data)) 
		{ 
			return false; 
		}
		else
		{
			strcpy_s(output, n, Output.c_str());
			return true;
		}
	}

	// this is just a wrapper for testing
	DLLFUNC_C bool CSaveResponse(char* filename, int calltype, int operation, int responseMode, char* args, char* data)
	{
		std::string Output;
		if (!GetResponse(&Output, calltype, operation, responseMode, args, data))
		{
			return false;
		}
		else
		{
			std::ofstream myfile(filename);
			myfile << Output;
			myfile.close();
			return true;
		}

	}

	bool ConfirmAccount(std::string parseThis)
	{
		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { return false; }
		pugi::xml_node accountbalance = doc.child("response").child("accountbalance");
		std::ostringstream oss;
		oss << accountbalance.child_value("account");

		if (oss.str() == accid) return true;
		else return false;
	}

	void SaveMessage(std::string msg, std::string msgname)
	{
		std::string filename = "./Log/Diag" + msgname + ".xml";
		
		std::ofstream myfile(filename);
		myfile << msg;
		myfile.close();

		return;
	}

	DLLFUNC_C int BrokerLogin(char* User, char* Pwd, char* Type, char* Account)
	{
		if (!User) // log out
		{
			return 0;
		}
		else if (((std::string)Type) == "Demo")
		{
			BrokerError("Demo mode not supported by this plugin.");
			return 0;
		}
		else // user name is being provided
		{
			if (accid == "") { accid = User; }
			if (consumerKey == "" || consumerSecret == "" || oauthToken == "" || oauthTokenSecret == "")
			{
				std::string pwd; int len;
				pwd = Pwd; 
				if ((pwd.length() % 4 == 0)&&(pwd.length()>0))
				{
					len = pwd.length() / 4;  
					consumerKey = pwd.substr(0, len);
					consumerSecret = pwd.substr(1 * len, len);
					oauthToken = pwd.substr(2 * len, len);
					oauthTokenSecret = pwd.substr(3 * len, len);
				}
				else {
					BrokerError("Error: Password must be divisible \n\ninto four equal strings.\n\nThe password is: consumerKey,consumerSecret,\n\noauthToken,oauthTokenSecret \n\nwith each item back-to-back (no commas).");
					return 0;
				}
			}
		}

		//attempt login

		std::string response;
		if (!GetResponse(&response, HGET, ACCOUNTS_ID_BALANCES, XML, "","")) { return 0; }
		
		if (diag)SaveMessage(response, "BrokerLogin");
		
		if (ConfirmAccount(response)) return 1; // success!
		else return 0;


	}

	DATE convertTime(__time32_t t32)
	{
		return (DATE)t32 / (24.*60.*60.) + 25569.; // 25569. = DATE(1.1.1970 00:00)
	}

	__time32_t convertTime(DATE date)
	{
		return (__time32_t)((date - 25569.)*24.*60.*60.);
	}


	void SetTimeZone(std::string TZ)
	{
		_putenv_s("TZ", TZ.c_str());
		_tzset();
		return;
	}

	DATE currentTime()
	{
		SetTimeZone("EST5EDT");
		__time32_t rawtime;
		_time32(&rawtime);
		return(convertTime(rawtime));
	}

	// convert DATE to YYYYMMMDD int format
	int convertTimeInt(DATE date)
	{
		try { return std::stoi(convertTime(date, STR_YYYYMMDD)); }
		catch (...) { BrokerError("stoi failed"); return 0; }
	}

	DATE convertTime(std::string timeString, int stringMode)
	{
		// STR_CLOCK example:		"2017-01-25 17:52:29.6592502-05:00",	"EST5EDT"
		// STR_FIX example:			"1900-01-01T01:01:01-05:00",			"EST5EDT"
		// STR_FIX2 example"		"1900-01-01T01:01:01.000-05:00",		"EST5EDT"
		// STR_TS example:			"1900-01-01T01:01:01Z",					"GMT0"
		// STR_DATEONLY example:	"1900-01-01",							"GMT0"
		// STR_YYYYMMDD	example:	"19000101",								"EST5EDT"
		// STR_YYYY_MM_DD example:	"1900-01-01",							"EST5EDT"

		bool needTime = true, forceStdTime = false, squeeze = false, scoot = false;
		std::string TZ;

		switch (stringMode)
		{
		case STR_CLOCK:		TZ = "EST5EDT";											break;
		case STR_FIX:		TZ = "EST5EDT";											break;
		case STR_FIX2:		TZ = "EST5EDT";											break;
		case STR_TS:		TZ = "UTC0"; forceStdTime = true;						break;
		case STR_DATEONLY:	TZ = "UTC0"; forceStdTime = true;  needTime = false;	break;
		case STR_YYYYMMDD:	TZ = "EST5EDT";		squeeze = true;	needTime = false;	scoot = true;  break;
		case STR_YYYY_MM_DD:TZ = "EST5EDT";					needTime = false;		break;
		default: throw ERROR_BAD_STRINGMODE; //fail
		}

		SetTimeZone(TZ);
		struct tm date_tm;
		date_tm.tm_year = std::stoi(timeString.substr(0, 4)) - 1900;
		date_tm.tm_mon = std::stoi(timeString.substr(5-(squeeze*1), 2)) - 1;  // squeeze: read as if there are no dashes
		date_tm.tm_mday = std::stoi(timeString.substr(8-(squeeze*2), 2));
		if (needTime)
		{
			date_tm.tm_hour = std::stoi(timeString.substr(11, 2));
			date_tm.tm_min = std::stoi(timeString.substr(14, 2));
			date_tm.tm_sec = std::stoi(timeString.substr(17, 2));
		}
		else
		{
			date_tm.tm_hour = 0;
			date_tm.tm_min = 0;
			date_tm.tm_sec = 0 + (scoot*1);  // scoot: hack to force date to read correctly... 23:59:59 becomes 00:00:01!
		}

		if (forceStdTime) date_tm.tm_isdst = 0;
		else date_tm.tm_isdst = -1;

		return convertTime(_mktime32(&date_tm));
	}


	
	bool replaceSubString(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}


	std::string convertTime(DATE date, int stringMode)
	{
		// STR_CLOCK example:		"2017-01-25 17:52:29.6592502-05:00",	"EST5EDT"
		// STR_FIX example:			"1900-01-01T01:01:01-05:00",			"EST5EDT"
		// STR_FIX2 example"		"1900-01-01T01:01:01.000-05:00",		"EST5EDT"
		// STR_TS example:			"1900-01-01T01:01:01Z",					"GMT0"
		// STR_DATEONLY example:	"1900-01-01",							"GMT0"
		// STR_YYYYMMDD	example:	"19000101",								"EST5EDT"
		// STR_YYYY_MM_DD example:	"1900-01-01",							"EST5EDT"
		
		__time32_t t32 = convertTime(date);
		char timetext[100]; 
		char* str_template;
		std::string Timetext, suffix="", suffix_std = "", suffix_dst = "", TZ;
		struct tm *time_tm;

		bool is_dst, is_dateonly = false;
		switch (stringMode)
		{
		case STR_CLOCK:	throw ERROR_BAD_STRINGMODE;																							break; // no reason to make clock strings.
		case STR_FIX:		TZ = "EST5EDT";		str_template = "%Y-%m-%dT%H:%M:%S%Z";	suffix_std = "-05:00";	suffix_dst = "-04:00";		break;
		case STR_FIX2:		TZ = "EST5EDT";		str_template = "%Y-%m-%dT%H:%M:%S.000%Z";	suffix_std = "-05:00";	suffix_dst = "-04:00";	break;
		case STR_TS:		TZ = "UTC0";		str_template = "%Y-%m-%dT%H:%M:%S%Z";	suffix_std = "Z";		suffix_dst = "Z";			break;
		case STR_DATEONLY:	TZ = "UTC0";		str_template = "%Y-%m-%d%Z";			is_dateonly = true;									break;
		case STR_YYYYMMDD:	TZ = "EST5EDT";		str_template = "%Y%m%d%Z";			is_dateonly = true;										break;
		case STR_YYYY_MM_DD:TZ = "EST5EDT";		str_template = "%Y-%m-%d%Z";			is_dateonly = true;									break;
		default: throw ERROR_BAD_STRINGMODE;																								break;
		}
		
		SetTimeZone(TZ);
		time_tm = _localtime32(&t32);
		
		strftime(timetext, 100, str_template, time_tm);
		Timetext = timetext;
		suffix = Timetext.substr(Timetext.length() - 3, 3);
		if (suffix == "EST"|| suffix == "UTC") suffix = suffix_std;
		else if (suffix == "EDT" || suffix == "UTC") suffix = suffix_dst;
		else throw ERROR_BAD_TIME_ZONE; //fail

		replaceSubString(Timetext, "00:00:01", "00:00:00"); //hack
		return(Timetext.substr(0, Timetext.length() - 3) + suffix);

		return("");
	}

	DLLFUNC void TestDate()
	{
		std::string a = "20170723", b;
		b = convertTime(convertTime(a, STR_YYYYMMDD), STR_FIX2);
		
		std::cout << "Input: " << a << ", Output: " << b << std::endl;
		return;
	}


	void ResetClock(DATE zorroDate, StopWatch *stopwatch)
	{
		zorroDate0 = zorroDate;
		//clock0 = clock();
		stopwatch->reset();
		return;
	}
	bool NeedToGetClock(StopWatch *stopwatch)
	{
		if (stopwatch->is_null()) return true;
		else if (stopwatch->milliseconds_passed() > INTERVAL_BROKERTIME_MS) return true;
		else return false;
	}
	DATE TempDate(StopWatch *stopwatch)
	{
		return(zorroDate0 + stopwatch->zorroDate_passed());
	}


	bool ParseTime(std::string parseThis, DATE* zorroDate, StopWatch *stopwatch, bool* marketIsOpen)
	{
		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { return false; }
		pugi::xml_node response = doc.child("response");
		std::ostringstream oss_date, oss_unixtime, oss_status_current;
		oss_date << response.child_value("date");
		oss_status_current << response.child("status").child_value("current");

		try {
			//if(zorroDate)*zorroDate = (DATE)convertTime((__time32_t)convertTimeFromAPI((APITIME)oss_date.str()));
			if (zorroDate)*zorroDate = convertTime(oss_date.str(), STR_CLOCK);
			ResetClock(*zorroDate, stopwatch);

		}
		catch(...){
			if (diag)BrokerError("\nCannot parse \"date\" value.");
			return false;
		}


		if (oss_status_current.str() == "open") *marketIsOpen = true;
		else *marketIsOpen = false;


		return true;
	}





	DLLFUNC_C int BrokerTime(DATE *pTimeGMT)
	{
		static StopWatch stopwatch;
		
		if (NeedToGetClock(&stopwatch))
		{
			std::string response, errormsg;
			if (!GetResponse(&response, HGET, MARKET_CLOCK, XML, "", "")) { stopwatch.nullify();  return 0; }
			if (diag) SaveMessage(response, "BrokerTime");
			//bool marketIsOpen; // moved to global
			if (IsErrorMessage(response, &errormsg))
			{
				BrokerError(errormsg.c_str());
				stopwatch.nullify();  return 0;
			}
			if (!ParseTime(response, pTimeGMT, &stopwatch, &marketIsOpen)) {
				if (diag)BrokerError("\nParse time failed.");
				stopwatch.nullify();  return 0;
			}
			if (marketIsOpen) return 2;
			else return 1;
		}
		else
		{
			if (pTimeGMT) *pTimeGMT = TempDate(&stopwatch);
			if (marketIsOpen) return 2;
			else return 1;
		}
		
	}
	
	bool ParseGetMarketQuoteResponse(std::string parseThis)
	{
		
		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { BrokerError("XML Load failure."); return false; }

		// sanity test - does <error>Success</error> exist under response?
		pugi::xml_node response = doc.child("response");
		std::ostringstream oss_error;
		oss_error << response.child_value("error");
		if (oss_error.str() != "Success") { BrokerError("Bad XML data."); return false; }

		pugi::xml_node quotes = doc.child("response").child("quotes");
		bool quotes_found = false;

		// cycle through nodes and update assetList and optionlist
		for (pugi::xml_node quote = quotes.child("quote"); quote; quote = quote.next_sibling("quote"))
		{
			quotes_found = true; double ask = 0., bid = 0.;
			//ask, bid, name,op_flag,secclass,symbol
			std::ostringstream oss_ask, oss_bid, oss_name, oss_op_flag, oss_secclass, oss_symbol;
			oss_ask <<		quote.child_value("ask");
			oss_bid <<		quote.child_value("bid");
			oss_name <<		quote.child_value("name");
			oss_op_flag <<	quote.child_value("op_flag");
			oss_secclass << quote.child_value("secclass");
			oss_symbol <<	quote.child_value("symbol");

			// is this asset a stock?
			if (oss_secclass.str() == "0")
			{
				try 
				{
					ask = std::stod(oss_ask.str());
					if (!ask) // weekend hack: force get latest stock quote from timesales intead of ext/quotes
					{ 
						ask = GetBH2TickClose((char*)oss_symbol.str().c_str());
						assetList.updateStock(oss_symbol.str(), ask, 0.);
					}
					else 
					{
						assetList.updateStock(oss_symbol.str(), std::stod(oss_ask.str()), abs(std::stod(oss_ask.str()) - std::stod(oss_bid.str())));
					}
				}
				catch (...) { BrokerError("Bad string conversion."); continue; }
			}
			// is this asset an option?
			else if (oss_secclass.str() == "1")
			{
				
				try {
					assetList.updateOption(oss_symbol.str(), std::stod(oss_ask.str()), abs(std::stod(oss_ask.str()) - std::stod(oss_bid.str())));
				}
				catch (...) { BrokerError("Bad string conversion."); continue; }
			}
			else  //if (oss_secclass.str() == "NA") // this is not a real asset, so delete it
			{
				assetList.DeleteEntry(oss_symbol.str());
			}

		}

		if(!quotes_found)BrokerError("No quotes found.");
		return quotes_found;
	}

	bool AreDoublesIdentical(double a, double b)
	{
		return ((int)std::round(a * 1000) == (int)std::round(b * 1000));
	}
	bool AreDoublesIdentical(double a, std::string b)
	{
		try {AreDoublesIdentical(a, std::stod(b));}
		catch (...) { return false; }
	}
	bool AreDoublesIdentical(std::string a, std::string b)
	{
		try { AreDoublesIdentical(std::stod(a), std::stod(b)); }
		catch (...) { return false; }
	}


	bool ParseZorroAssetToOption(std::string za, std::string *underlying, std::string *PorC, double *strike)
	{
		
		// reformat options to Ally Invest format
		// example input:  "SPY-OPT-20170721-241.50-P"
		// example output: underlying: "SPY", PorC = "P", strike = 241.50

		try {
			size_t optpos = za.find("-OPT-");

			// if this is not an option...
			if (optpos == std::string::npos) { *underlying = za;  return false; }

			// otherwise, get to work.
			std::ostringstream oss_underlying, oss_expiry, oss_PorC;
			oss_underlying << za.substr(0, optpos); // "SPY"
			oss_expiry << za.substr(optpos + 5, 8); // "20170721"
			oss_PorC << za.substr(za.length() - 1, 1); //"P"

			int strk_pos = optpos + 14;
			int strk_len = za.length() - strk_pos - 2;
			*strike = std::stod(za.substr(strk_pos, strk_len));
			*PorC = oss_PorC.str();
			*underlying = oss_underlying.str();

			return(true);
		}
		catch (...)
		{
			*underlying = za;
			return false;
		}
	}
	std::string GetAllyAsset(const char* Asset)
	{
		std::string str= Asset;
		// reformat options to Ally Invest format
		// example input:  "SPY-OPT-20170721-241.50-P"
		// example output: "SPY170714C00205000" = (SPY, 20170714, CALL, STRIKE = 00205.000)
		
		try {
			size_t optpos = str.find("-OPT-");

			// if this is not an option...
			if (optpos == std::string::npos) { return str; }

			// otherwise, get to work.
			std::ostringstream oss;
			oss << str.substr(0, optpos); // "SPY"
			oss << str.substr(optpos + 7, 6); // "170721"
			oss << str.substr(str.length() - 1, 1); //"P"

			int qpos = optpos + 14;
			int qlen = str.length() - qpos - 2;
			double quote = std::stod(str.substr(qpos, qlen));
			int quote_i = (int)(std::round(quote * 1000));
			oss << std::setw(8) << std::setfill('0') << quote_i; //"00241500"

			return(oss.str());
		}
		catch (...)
		{
			return str;
		}
	}
	void GetAllyOptionParameters(const std::string AllyAsset, std::string *underlying, std::string *str_fix2, std::string *strike, std::string *CFI_code)
	{
		// example input:  "SPY170714C00205000"
		std::string yyyymmdd;
		int len = AllyAsset.length();
		*underlying = AllyAsset.substr(0, len - 15);
		yyyymmdd = "20" + AllyAsset.substr(len - 15, 6);
		*str_fix2 = convertTime(convertTime(yyyymmdd, STR_YYYYMMDD), STR_FIX2);
		*CFI_code = "O" + AllyAsset.substr(len - 9, 1);
		//sanity check
		if (!(*CFI_code == "OC" || *CFI_code == "OP")) throw ERROR_BAD_OPTIONS_PARSE;
		*strike = GetDoubleString(std::round(std::stod(AllyAsset.substr(len - 8, 8))) / 1000.);
		
	}
	//std::string GetAllyUnderlying(const std::string AllyAsset)
	//{
	//	// example input:  "SPY170714C00205000"
	//	return(AllyAsset.substr(0, AllyAsset.length() - 15));
	//}
	bool AssetIsAnOption(std::string AllyAsset)
	{
		if (AllyAsset.length() > 8) { return true; }
		else { return false; }
	}
	DLLFUNC void TestGetAllyAsset()
	{
		char* input = "SPY-OPT-20170721-241.50-P";
		std::cout << "Input: " << input << ", Output: " << GetAllyAsset(input) << std::endl;
		return;

	}
	float GetBH2TickClose(char* Asset) // weekend hack - get quote from timesales instead of ext/quotes
	{
		T6 ticks[1];
		if (BrokerHistory2(Asset, currentTime() - 7., currentTime(), 1, 1, ticks)!=1) return false;
		if (ticks[0].fClose) return ticks[0].fClose;
		return false;
	}

	DLLFUNC_C int BrokerAsset(char* Asset, double *pPrice, double *pSpread, double *pVolume, double *pPip, double *pPipCost, double *pLotAmount, double *pMarginCost, double *pRollLong, double *pRollShort)
	{
		LastBrokerAsset = Asset;
		std::string AllyAsset = GetAllyAsset(Asset);
		
		static StopWatch stopwatch;
		bool force_request = false;

		// is a stock not subscribed yet?
		if (!assetList.DoesAssetExist(AllyAsset))
		{
			if(!AssetIsAnOption(AllyAsset))	assetList.updateStock(AllyAsset, 0., 0.);
			else {assetList.updateOption(AllyAsset, 0., 0.);}
			force_request = true;
		}

		// is a server request required?
		if (force_request || stopwatch.milliseconds_passed() > INTERVAL_BROKERASSET_MS)
		{
			// make request, get response
			std::string response, errormsg, Query; char* query;
			double parsePrice, parseSpread;
			Query = "symbols=" + assetList.GetAssetCSV() + "&fids=ask,bid,name,op_flag,secclass,symbol";
			query = (char*)Query.c_str();

			if (!GetResponse(&response, HPOST, MARKET_EXT_QUOTES, XML, "", query))
			{
				stopwatch.reset();
				if (diag)BrokerError("Cannot acquire asset info from server.");
				return 0;
			}
			stopwatch.reset();

			if (diag) SaveMessage(response, (char*)("BrokerAsset_MarketQuote_" + AllyAsset).c_str());

			if (IsErrorMessage(response, &errormsg))
			{
				BrokerError(errormsg.c_str());
				return 0;
			}
			
			//the following updates the assetList.
			if (!ParseGetMarketQuoteResponse(response)) {

				if (diag)BrokerError("Ask Bid parse failed.");
				return 0;
			}
			
		}


		double price = 0, spread = 0;
		if (!assetList.CanGetValues(AllyAsset, &price, &spread))
		{
			return 0;
		}
		else if (price == NULL)
		{
			//BrokerError((char*)((std::string)Asset + " found, bid/ask prices not available now.").c_str());
			return 1;
		}
		
		if (!pPrice) return 1;
		
		if (pPrice) *pPrice = price;
		if (pSpread) *pSpread = spread;

		// volume needs to be retrieved with BrokerHistory2.
		if (pVolume) *pVolume = 0.;

		// fixed values for standard USD equities account
		if (pPip) *pPip = 0.01;
		if (pPipCost) *pPipCost = 0.01;
		if (pLotAmount) *pLotAmount = 1.0;

		//assuming a cash account.  User will need to manually set MarginCost, especially when handling options.
		if (pMarginCost) *pMarginCost = (*pPrice *  1.);

		// no rollovers
		if (pRollLong) *pRollLong = 0.;
		if (pRollShort) *pRollShort = 0.;

		return 1;

	}




	DLLFUNC_C int BrokerHistory2(char* Asset, DATE tStart, DATE tEnd, int nTickMinutes, int nTicks, T6* ticks)
	{
		
		if (!Asset || !ticks || !nTicks) return 0;
		std::string AllyAsset = GetAllyAsset(Asset);

		//static HistoryBuffer historybuffer;
		std::string interval; double lookback;
		if (nTickMinutes == 1) { interval = "1min"; lookback = 6.; }
		else if (nTickMinutes == 5) { interval = "5min"; lookback = 10.; }
		else if (nTickMinutes > 1)
		{
			BrokerError("Only M1 and M5 data supported at this time.");
			return 0;
		}
		else if (nTickMinutes == 0)
		{
			BrokerError("Tick data not supported at this time.");
			return 0;
		}
		else
		{
			BrokerError("Undefined data mode not supported.");
			return 0;
		}

		std::string response, Query; char* query;



		
		Query = "symbols=" + AllyAsset + "&interval=" + interval + "&startdate=" + convertTime(max(tStart,tEnd- lookback), STR_DATEONLY) + "&enddate=" + convertTime(tEnd, STR_DATEONLY); query = (char*)Query.c_str();
		if (!GetResponse(&response, HGET, MARKET_TIMESALES, XML, query, "")) { BrokerError("BrokerHistory2 HTTP error"); return 0; }

		if (diag) SaveMessage(response, (char*)("BrokerHistory2_timesales_" + AllyAsset).c_str());

		// parse response
		pugi::xml_document doc;
		if (!doc.load_string(response.c_str())) { BrokerError("XML Load failure."); return 0; }

		// sanity test - does <error>Success</error> exist under response?
		pugi::xml_node response_xml = doc.child("response");
		std::ostringstream oss_error;
		oss_error << response_xml.child_value("error");
		if (oss_error.str() != "Success") { BrokerError("Bad XML data."); return 0; }

		pugi::xml_node quotes = doc.child("response").child("quotes");
		
		// cycle through nodes and update ticks from most recent to oldest.
		int i = 0;
		for (pugi::xml_node quote = quotes.last_child(); quote; quote = quote.previous_sibling())
		{
			std::ostringstream oss_date, oss_datetime, oss_hi, oss_incr_vl, oss_last, oss_lo, oss_opn, oss_vl;
			oss_date << quote.child_value("date");
			oss_datetime << quote.child_value("datetime");
			oss_hi << quote.child_value("hi");
			oss_incr_vl << quote.child_value("incr_vl");
			oss_last << quote.child_value("last");
			oss_lo << quote.child_value("lo");
			oss_opn << quote.child_value("opn");
			oss_vl << quote.child_value("vl");

			try
			{
				DATE checktime = convertTime(oss_datetime.str(), STR_FIX);
				if (checktime < tStart) break;
				if (checktime > tEnd) continue;
				
				ticks[i].fClose = std::stof(oss_last.str());
				ticks[i].fHigh = std::stof(oss_hi.str());
				ticks[i].fLow = std::stof(oss_lo.str());
				ticks[i].fOpen = std::stof(oss_opn.str());
				ticks[i].time = checktime;

				// fVol is real volume, fVal is unused.
				ticks[i].fVol = std::stof(oss_incr_vl.str());

			}
			catch (...)
			{
				BrokerError("BrokerHistor2: Error parsing XML.");
				break;
			}

			//operation completed, tally it.
			i++; if (i >= nTicks) break;

		}

		return i;
	}



	DLLFUNC_C int BrokerAccount(char* Account, double *pdBalance, double *pdTradeVal, double *pdMarginVal)
	{
		// this plugin only supports one account. char* Account is ignored.
		static double saveBalance, saveTradeVal, saveMarginVal;
		static StopWatch stopwatch;

		if (stopwatch.is_null() || stopwatch.milliseconds_passed() > INTERVAL_BROKERACCOUNT_MS)
		{
			std::string response;

			if (!GetResponse(&response, HGET, ACCOUNTS_ID, XML, "", "")) { BrokerError("BrokerAccount HTTP error"); stopwatch.nullify(); return 0; }
			if (diag) SaveMessage(response, "BrokerAccount");

			// parse response
			pugi::xml_document doc;
			if (!doc.load_string(response.c_str())) { BrokerError("XML Load failure."); stopwatch.nullify();  return 0; }

			// sanity test - does <error>Success</error> exist under response?
			pugi::xml_node response_xml = doc.child("response");
			std::ostringstream oss_error;
			oss_error << response_xml.child_value("error");
			if (oss_error.str() != "Success") { BrokerError("Bad XML data."); stopwatch.nullify();  return 0; }

			//begin data acquisition
			pugi::xml_node accountbalance = doc.child("response").child("accountbalance");
			pugi::xml_node accountholdings = doc.child("response").child("accountholdings");

			std::ostringstream oss_accountvalue, oss_marginbalance;
			oss_accountvalue << accountbalance.child_value("accountvalue");
			oss_marginbalance << accountbalance.child("money").child_value("marginbalance");

			try {
				saveBalance = std::stod(oss_accountvalue.str());
				saveMarginVal = std::stod(oss_marginbalance.str());
			}
			catch (...) {
				BrokerError("BrokerAccount parse failure.");
				stopwatch.nullify();
				return 0;
			}

			saveTradeVal = 0.;
			// cycle through account holdings and calculate total gain/loss
			for (pugi::xml_node holding = accountholdings.child("holding"); holding; holding = holding.next_sibling("holding"))
			{
				std::ostringstream oss_gainloss;
				oss_gainloss << holding.child_value("gainloss");
				try {
					saveTradeVal += std::stod(oss_gainloss.str());
				}
				catch (...)
				{
					BrokerError("BrokerAccount parse failure.");
					stopwatch.nullify();
					return 0;
				}
			}
			stopwatch.reset();

		}

		if (pdBalance) *pdBalance = saveBalance;
		if (pdTradeVal) *pdTradeVal = saveTradeVal;
		if (pdMarginVal) *pdMarginVal = saveMarginVal;
		
		return 1;
	}




	DLLFUNC std::vector<std::string> fixml_messages(std::string parseThis)
	{
		std::vector<std::string> output;
		output.reserve(100);  // receive up to 100 messages without reallocation
		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { throw ERROR_FIXML_MESSAGE_LOAD_FAIL; }
		pugi::xml_node orderstatus = doc.child("response").child("orderstatus");
		for (pugi::xml_node order = orderstatus.child("order"); order; order = order.next_sibling("order"))
		{
			std::ostringstream oss;
			//oss << "Order number " << i+1 << ":" << std::endl;
			oss << order.child_value("fixmlmessage");
			//messages.at(i) = oss.str();
			output.push_back(oss.str());
		}
		
		return(output);
	}

	std::string GetDoubleString(double a)
	{
		std::ostringstream oss;
		oss << std::setprecision(3);
		oss << std::fixed << std::showpoint;
		oss << (std::round(a * 1000.) / 1000.);
		return oss.str();
	}
	std::string MakeStockOrder(std::string AllyAsset, int nAmount, double dStopDist, FixmlTimeInForce_enum_t terms = tif_DayOrder, FixmlOrdType_enum_t type=ot_Market, double entryPx = 0.00, double stopPx = 0.00) // the last four variables are for testing purposes - create pending orders
	{
		char ch[2] = { '\0','\0' };  // enum char-to-string hack
		
		pugi::xml_document doc;
		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";
		pugi::xml_node FIXML = doc.append_child("FIXML");
		FIXML.append_attribute("xmlns") = "http://www.fixprotocol.org/FIXML-5-0-SP2";
		
		pugi::xml_node Order = FIXML.append_child("Order");
		ch[0] = terms; Order.append_attribute("TmInForce") = ch;
		ch[0] = type; Order.append_attribute("Typ") = ch;
		if (dStopDist != -1)//to open
		{
			if (nAmount >= 0) { 
				ch[0] = s_Buy; Order.append_attribute("Side") = ch; 
			}
			else if (nAmount < 0) { 
				ch[0] = s_SellShort; Order.append_attribute("Side") = ch; 
			}
		}
		else if (dStopDist == -1) //to close
		{
			if (nAmount >= 0) {
				ch[0] = s_Buy; Order.append_attribute("Side") = ch;
				ch[0] = at_MarginShort; Order.append_attribute("AcctTyp")=ch;
			}
			else if (nAmount < 0) { 
				ch[0] = s_Sell; Order.append_attribute("Side") = ch; }
		}
		
		switch (type)
		{
		case ot_Market: break;
		case ot_Limit: Order.append_attribute("Px") = GetDoubleString(entryPx).c_str();			break;
		case ot_Stop: Order.append_attribute("StopPx") = GetDoubleString(stopPx).c_str();		break;
		case ot_StopLimit: Order.append_attribute("Px") = GetDoubleString(entryPx).c_str();
			Order.append_attribute("StopPx") = GetDoubleString(stopPx).c_str();					break;
		default: break;
		}
		Order.append_attribute("Acct") = accid.c_str();

		pugi::xml_node Instrmt = Order.append_child("Instrmt");
		Instrmt.append_attribute("SecTyp") = "CS";
		Instrmt.append_attribute("Sym") = AllyAsset.c_str();

		pugi::xml_node OrdQty = Order.append_child("OrdQty");
		OrdQty.append_attribute("Qty") = std::to_string(abs(nAmount)).c_str();

		std::ostringstream output;
		doc.save(output); // pretty print
		//doc.save(output, "", pugi::format_raw); // unformatted
		return(output.str());

	}
	DLLFUNC void TestMakeStockOrder()
	{
		accid = "12345678";
		std::cout << MakeStockOrder("MSFT", 23, 0, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	//buy to open
		std::cout << MakeStockOrder("MSFT", -14, 0, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	//sell short to open
		std::cout << MakeStockOrder("MSFT", 12, -1, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	// buy to close
		std::cout << MakeStockOrder("MSFT", -11, -1, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	//sell to close
		std::cout << MakeStockOrder("MSFT", -11, -1, tif_GoodTilCanceled, ot_StopLimit, 21.1, 19.1) << std::endl << std::endl;	//sell to close, w/ stop limit
		std::cout << std::endl;
		return;
	}
	DLLFUNC_C void CSaveStockOrderResponse(char* filename, char* AllyAsset, int nAmount, double dStopDist)
	{
		std::string response, Order, Filename = filename; //char order[10000];
		Order = MakeStockOrder((std::string)AllyAsset, nAmount, dStopDist, tif_DayOrder, ot_Market, 0.00, 0.00);
		SaveMessage(Order, Filename + "_order");
		//strcpy(order, Order.c_str());
		//GetResponse(&response, HPOST, DIAGNOSTICS_HTTPONY, XML, "", order);
		if (!GetResponse(&response, HPOST, ACCOUNTS_ID_ORDERS_PREVIEW, XML, "", (char*)Order.c_str())) { BrokerError("There was a problem getting a response."); return; }
		SaveMessage(response, Filename + "_response");
		return;
	}

	std::string MakeOptionOrder(std::string AllyAsset, int nAmount, double dStopDist, FixmlTimeInForce_enum_t terms = tif_DayOrder, FixmlOrdType_enum_t type = ot_Market, double entryPx = 0.00, double stopPx = 0.00) // the last four variables are for testing purposes - create pending orders
	{
		std::string underlying = "", str_fix2 = "", CFI_code = "", strike ="";
		char ch[2] = { '\0','\0' };  // enum char-to-string
		GetAllyOptionParameters(AllyAsset, &underlying, &str_fix2, &strike, &CFI_code); //throws exception if no good
		
		
		pugi::xml_document doc;
		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";
		pugi::xml_node FIXML = doc.append_child("FIXML");
		FIXML.append_attribute("xmlns") = "http://www.fixprotocol.org/FIXML-5-0-SP2";

		pugi::xml_node Order = FIXML.append_child("Order");
		ch[0] = terms; Order.append_attribute("TmInForce") = ch;
		ch[0] = type; Order.append_attribute("Typ") = ch;
		if (nAmount >= 0) { 
			ch[0] = s_Buy; Order.append_attribute("Side") = ch; }
		else if (nAmount < 0) { 
			ch[0] = s_Sell; Order.append_attribute("Side") = ch; }
		if (dStopDist != -1)//to open
		{
			ch[0] = pos_Open; Order.append_attribute("PosEfct") = ch;
		}
		else if (dStopDist == -1) //to close
		{
			ch[0] = pos_Close; Order.append_attribute("PosEfct") = ch;
		}

		switch (type)
		{
		case ot_Market: break;
		case ot_Limit: Order.append_attribute("Px") = GetDoubleString(entryPx).c_str();			break;
		case ot_Stop: Order.append_attribute("StopPx") = GetDoubleString(stopPx).c_str();		break;
		case ot_StopLimit: Order.append_attribute("Px") = GetDoubleString(entryPx).c_str();
			Order.append_attribute("StopPx") = GetDoubleString(stopPx).c_str();					break;
		default: throw ERROR_FIXML_MESSAGE_LOAD_FAIL; break;
		}
		Order.append_attribute("Acct") = accid.c_str();

		pugi::xml_node Instrmt = Order.append_child("Instrmt");
		Instrmt.append_attribute("CFI") = CFI_code.c_str();
		Instrmt.append_attribute("SecTyp") = "OPT";
		Instrmt.append_attribute("MatDt") = str_fix2.c_str();
		Instrmt.append_attribute("StrkPx") = strike.c_str();
		Instrmt.append_attribute("Sym") = underlying.c_str();

		pugi::xml_node OrdQty = Order.append_child("OrdQty");
		OrdQty.append_attribute("Qty") = std::to_string(abs(nAmount)).c_str();

		std::ostringstream output;
		doc.save(output); // pretty print
		//doc.save(output, "", pugi::format_raw); // unformatted
		return(output.str());
	}
	DLLFUNC void TestMakeOptionOrder()
	{
		accid = "87654321";
		std::cout << MakeOptionOrder("SPY170714C00205000", 23, 0, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	//buy to open
		std::cout << MakeOptionOrder("SPY170714C00204500", -14, 0, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	//sell short to open
		std::cout << MakeOptionOrder("SPY170714P00204000", 12, -1, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	// buy to close
		std::cout << MakeOptionOrder("SPY170714P00203500", -11, -1, tif_DayOrder, ot_Market, 0.0, 0.0) << std::endl << std::endl;	//sell to close
		std::cout << MakeOptionOrder("SPY170714P00203000", -11, -1, tif_GoodTilCanceled, ot_StopLimit, 21.1, 19.1) << std::endl << std::endl;	//sell to close, w/ stop limit
		std::cout << std::endl;
		return;
	}
	DLLFUNC_C void CSaveOptionOrderResponse(char* filename, char* AllyAsset, int nAmount, double dStopDist)
	{
		std::string response, Order, Filename = filename; //char order[10000];
		Order = MakeOptionOrder((std::string)AllyAsset, nAmount, dStopDist, tif_DayOrder, ot_Market, 0.00, 0.00);
		SaveMessage(Order, Filename + "_order");
		//strcpy(order, Order.c_str());
		//GetResponse(&response, HPOST, DIAGNOSTICS_HTTPONY, XML, "", order);
		if (!GetResponse(&response, HPOST, ACCOUNTS_ID_ORDERS_PREVIEW, XML, "", (char*)Order.c_str())) { BrokerError("There was a problem getting a response."); return; }
		SaveMessage(response, Filename + "_response");
		return;
	}

	
	std::string MakeMultiLegOptionOrder(FixmlTimeInForce_enum_t terms = tif_DayOrder, FixmlOrdType_enum_t type = ot_Market, double entryPx = 0.00) // the last three variables are for testing purposes - create pending orders
	{
		char ch[2] = { '\0','\0' };  // enum char-to-string
		pugi::xml_document doc;
		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";

		pugi::xml_node FIXML = doc.append_child("FIXML");
		FIXML.append_attribute("xmlns") = "http://www.fixprotocol.org/FIXML-5-0-SP2";

		pugi::xml_node NewOrdMleg = FIXML.append_child("NewOrdMleg");
		ch[0] = terms; NewOrdMleg.append_attribute("TmInForce") = ch;
		switch (type)
		{
		case ot_Market: break;
		case ot_Limit: NewOrdMleg.append_attribute("Px") = GetDoubleString(entryPx).c_str(); break;
		default: throw ERROR_FIXML_MESSAGE_LOAD_FAIL;  break;
			
		}
		ch[0] = type; NewOrdMleg.append_attribute("OrdTyp") = ch;

		NewOrdMleg.append_attribute("Acct") = accid.c_str();

		int i = 0;
		for (auto & optionleg : optionLegs)
		{
			i++; if(i>4) throw ERROR_FIXML_MESSAGE_LOAD_FAIL; // Broker supports up to five legs.
			std::string underlying = "", str_fix2 = "", CFI_code = "", strike = "";
			GetAllyOptionParameters(optionleg.AllyAsset, &underlying, &str_fix2, &strike, &CFI_code); //throws exception if no good

			pugi::xml_node Ord = NewOrdMleg.append_child("Ord");
			Ord.append_attribute("OrdQty") = std::to_string(abs(optionleg.nAmount)).c_str();
			if (optionleg.dStopDist != -1)//to open
			{
				ch[0] = pos_Open; Ord.append_attribute("PosEfct") = ch;
			}
			else if (optionleg.dStopDist == -1) //to close
			{
				ch[0] = pos_Close; Ord.append_attribute("PosEfct") = ch;
			}


			pugi::xml_node Leg = Ord.append_child("Leg");
			if (optionleg.nAmount >= 0) {
				ch[0] = s_Buy; Leg.append_attribute("Side") = ch;
			}
			else if (optionleg.nAmount < 0) {
				ch[0] = s_Sell; Leg.append_attribute("Side") = ch;
			}
			Leg.append_attribute("Strk") = strike.c_str();
			Leg.append_attribute("Mat") = str_fix2.c_str();
			Leg.append_attribute("MMY") = (str_fix2.substr(0,4)+ str_fix2.substr(5,2)).c_str();
			Leg.append_attribute("SecTyp") = "OPT";
			Leg.append_attribute("CFI") = CFI_code.c_str();
			Leg.append_attribute("Sym") = underlying.c_str();

		}
		std::ostringstream output;
		doc.save(output); // pretty print
		//doc.save(output, "", pugi::format_raw); // unformatted
		return(output.str());
	}
	DLLFUNC void TestMakeMultiLegOptionOrder()
	{
		accid = "56781234";
		std::vector <OptionLeg> test1, test2; optionLegs.reserve(4);
		OptionLeg l1, l2, l3, l4;
		l1.AllyAsset = "SPY171229C00205000"; l1.nAmount = 23; l1.dStopDist = 0; optionLegs.push_back(l1);
		l2.AllyAsset = "SPY171229C00204500"; l2.nAmount = -14; l2.dStopDist = 0; optionLegs.push_back(l2);
		l3.AllyAsset = "SPY171229P00204000"; l3.nAmount = 12; l3.dStopDist = -1; optionLegs.push_back(l3);
		l4.AllyAsset = "SPY171229P00203500"; l4.nAmount = -11; l4.dStopDist = -1; optionLegs.push_back(l4);
		test1 = optionLegs;
		test2 = optionLegs;
		std::cout << MakeMultiLegOptionOrder(tif_DayOrder, ot_Market, 0.00) << std::endl << std::endl;
		std::cout << MakeMultiLegOptionOrder(tif_DayOrder, ot_Limit, -3.10) << std::endl << std::endl; // limit, debit 3.10
		return;
	}
	DLLFUNC_C void CAddLeg(char* AllyAsset, int nAmount, double dStopDist)
	{
		OptionLeg l1;
		l1.AllyAsset = AllyAsset;
		l1.nAmount = nAmount;
		l1.dStopDist = dStopDist;
		optionLegs.push_back(l1);
	}
	DLLFUNC_C void CSaveMultiLegOptionsOrderResponse(char* filename)
	{
		std::string response, Order, Filename = filename; //char order[10000];

		Order = MakeMultiLegOptionOrder(tif_DayOrder, ot_Market, 0.00);
		SaveMessage(Order, Filename + "_order");
		//strcpy(order, Order.c_str());
		//GetResponse(&response, HPOST, DIAGNOSTICS_HTTPONY, XML, "", order);
		if (!GetResponse(&response, HPOST, ACCOUNTS_ID_ORDERS_PREVIEW, XML, "", (char*)Order.c_str())) { BrokerError("There was a problem getting a response."); return; }
		SaveMessage(response, Filename + "_response");
		return;
	}


	bool ParseOrderPlacementResponse(std::string parseThis)
	{
		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { BrokerError("XML Load failure."); return false; }

		// sanity test - does <error>Success</error> exist under response?
		pugi::xml_node response = doc.child("response");
		std::ostringstream oss_error;
		oss_error << response.child_value("error");
		if (oss_error.str() != "Success") 
		{
			if (oss_error.str() == "") { BrokerError("Bad XML data."); }
			else { BrokerError(oss_error.str().c_str()); }
			return false; 
		}

		std::ostringstream oss_clientorderid; std::string msg = ""; 
		oss_clientorderid << response.child_value("clientorderid");
		if (oss_clientorderid.str() != "")
		{
			BrokerError(("ID: "+ oss_clientorderid.str()).c_str());
		}
		else return false;
	}


	DLLFUNC_C int BrokerBuy(char* Asset, int nAmount, double dStopDist, double *pPrice)
	{
		std::string AllyAsset = GetAllyAsset(Asset), fixml_message, response; char* Fixml_message; OptionLeg ol; std::string ul1, sf21, str1, cfi1;
		if (!AssetIsAnOption(AllyAsset)) // is a stock/ETF
		{
			try { fixml_message = MakeStockOrder(AllyAsset, nAmount, dStopDist, tif_DayOrder, ot_Market, 0.0, 0.0); } catch (...) { return 0; }
			if(diag)SaveMessage(fixml_message, "BrokerBuy_" + AllyAsset+"_fixml");
			if (!GetResponse(&response, HPOST, ACCOUNTS_ID_ORDERS, XML, "", (char*)fixml_message.c_str())){return 0; }
			if(diag)SaveMessage(fixml_message, "BrokerBuy_" + AllyAsset+"_response");
			if (!ParseOrderPlacementResponse(response)) { return 0; }

			if (dStopDist == -1) return -1;
			else { tradeCounter++; return tradeCounter; }
		}
		else if (numLegs >= 1) // is a multi-leg option
		{
			ol.AllyAsset = AllyAsset;
			ol.dStopDist = dStopDist;
			ol.nAmount = nAmount;
			optionLegs.push_back(ol);

			numLegs--;
			if (numLegs >= 1)
			{
				// do nothing until we have all of the legs.
				if (dStopDist == -1) return -1;
				else { tradeCounter++; return tradeCounter; }
			}
			else // execute combo trade
			{
				// sanity check - All options must have same underlying and same expiry month.
				GetAllyOptionParameters(AllyAsset, &ul1, &sf21, &str1, &cfi1);
				for (auto & optionleg : optionLegs)
				{
					std::string ul2, sf22, str2, cfi2;
					GetAllyOptionParameters(optionleg.AllyAsset, &ul2, &sf22, &str2, &cfi2);
					if (!isCaseInsensitveMatch(ul1,ul2))
					{
						BrokerError("Combo order failure! Underlying assets do not match.");
						optionLegs.clear();
						optionLegs.reserve(4);
						return 0;
					}
					if (sf21.substr(0, 7) != sf22.substr(0, 7)) // "2017-07"
					{
						BrokerError("Combo order failure! Expiry months do not match.");
						optionLegs.clear();
						optionLegs.reserve(4);
						return 0;
					}
				}
				
				try { fixml_message = MakeMultiLegOptionOrder(tif_DayOrder, ot_Market, 0.0); }
				catch (...) { 

					BrokerError("Cannot generate multi-leg option order!");
					optionLegs.clear();
					optionLegs.reserve(4);
					return 0; }
				optionLegs.clear();
				optionLegs.reserve(4);
				if (diag)SaveMessage(fixml_message, "BrokerBuy_" + AllyAsset + "_fixml");
				if (!GetResponse(&response, HPOST, ACCOUNTS_ID_ORDERS, XML, "", (char*)fixml_message.c_str())) { return 0; }
				if (diag)SaveMessage(fixml_message, "BrokerBuy_" + AllyAsset + "_response");
				if (!ParseOrderPlacementResponse(response)) { return 0; }

				if (dStopDist == -1) return -1;
				else { tradeCounter++; return tradeCounter; }
			}
		}
		else // is a standalone option
		{
			try { fixml_message = MakeOptionOrder(AllyAsset, nAmount, dStopDist, tif_DayOrder, ot_Market, 0.0, 0.0); }
			catch (...) { return 0; }
			if (diag)SaveMessage(fixml_message, "BrokerBuy_" + AllyAsset + "_fixml");
			if (!GetResponse(&response, HPOST, ACCOUNTS_ID_ORDERS, XML, "", (char*)fixml_message.c_str())) { return 0; }
			if (diag)SaveMessage(fixml_message, "BrokerBuy_" + AllyAsset + "_response");
			if (!ParseOrderPlacementResponse(response)) { return 0; }

			if (dStopDist == -1) return -1;
			else { tradeCounter++; return tradeCounter; }
		}

		return 0;
	}


	double GetUnderlyingPrice(std::string Asset)
	{
		double pPrice = 0.0, pSpread = 0.0, pVolume = 0.0, pPip = 0.0, pPipCost = 0.0, pLotAmount = 0.0, pMarginCost = 0.0, pRollLong = 0.0, pRollShort = 0.0;

		// example: "SPY-OPT-20170721-241.50-P" becomes "SPY", or "SPY" becomes "SPY"
		std::string underlyingAsset = Asset.substr(0, Asset.find("-OPT-"));
		if (assetList.CanGetValues(underlyingAsset, &pPrice, &pSpread)) return pPrice;
		return 0.;
	}

	std::vector <CONTRACT> ParseOptionsContracts(std::string parseThis)
	{
		std::vector <CONTRACT> Contracts;
		Contracts.reserve(10000);

		pugi::xml_document doc;
		if (!doc.load_string(parseThis.c_str())) { BrokerError("XML Load failure."); throw ERROR_BAD_OPTIONS_PARSE; }

		// sanity test - does <error>Success</error> exist under response?
		pugi::xml_node response = doc.child("response");
		std::ostringstream oss_error;
		oss_error << response.child_value("error");
		if (oss_error.str() != "Success") { BrokerError("Bad XML data."); throw ERROR_BAD_OPTIONS_PARSE; }

		pugi::xml_node quotes = doc.child("response").child("quotes");
		//bool quotes_found = false;

		//int i = 0;
		// cycle through nodes and update assetList and optionlist
		for (pugi::xml_node quote = quotes.child("quote"); quote; quote = quote.next_sibling("quote"))
		{
			//quotes_found = true;
			//ask, bid, name,op_flag,secclass,symbol
			std::ostringstream oss_ask, oss_bid, oss_datetime, oss_op_style, oss_prem_mult, oss_put_call, oss_secclass, oss_strikeprice, oss_symbol, oss_undersymbol, oss_xdate;
			int dd = 0;
			oss_ask << quote.child_value("ask");
			oss_bid << quote.child_value("bid");
			oss_datetime << quote.child_value("datetime");
			oss_op_style << quote.child_value("op_style");
			oss_prem_mult << quote.child_value("prem_mult");
			oss_put_call << quote.child_value("put_call");
			oss_secclass << quote.child_value("secclass");
			oss_strikeprice << quote.child_value("strikeprice");
			oss_symbol << quote.child_value("symbol");
			oss_undersymbol << quote.child_value("undersymbol");
			oss_xdate << quote.child_value("xdate");

			if (oss_secclass.str() == "1") // if asset is an option
			{
				try {
					CONTRACT contract;
					contract.time = 0.; //This broker does not use "trading classes." Set to zero.
					contract.fAsk = std::stof(oss_ask.str());
					contract.fBid = std::stof(oss_bid.str());
					contract.fVal = std::stof(oss_prem_mult.str());
					contract.fVol = 0.;
					contract.fUnl = GetUnderlyingPrice(oss_undersymbol.str());
					contract.fStrike = std::stof(oss_strikeprice.str());
					contract.Expiry = std::stol(oss_xdate.str());
					contract.Type = 0;
					
					if (oss_put_call.str() == "put") contract.Type = contract.Type | PUT;
					else if (oss_put_call.str() == "call") contract.Type = contract.Type | CALL;
					else { BrokerError("Cannot parse options info correctly."); continue; }

					if (oss_op_style.str() == "E") contract.Type = contract.Type | EUROPEAN;
					dd = std::stoi(oss_xdate.str().substr(6, 2));  // "YYYYMMDD" -> DD,int
					if ((dd >= 16) && (dd <= 21)) contract.Type = contract.Type | ONLYW3;
					
					Contracts.push_back(contract);
					//assetList.updateOption(oss_symbol.str(), contract.fAsk, abs(contract.fAsk - contract.fBid));


				}
				catch (...)
				{
					BrokerError("Cannot parse options info.");
					continue;
				}

			}
			else // not an option, nothing to do
			{
				
			}

		}

		return Contracts;

	}

	struct Holding {
	public:
		std::string AllyAsset = "";
		int net_qty = 0.;
	};
	
	
	
	// needs testing
	std::vector <Holding> ParseHoldings(std::string parseThis)
	{
		pugi::xml_document doc;
		std::vector <Holding> holdings;
		if (!doc.load_string(parseThis.c_str())) { BrokerError("XML Load failure."); throw ERROR_BAD_GET_POSITIONS; }

		// sanity test - does <error>Success</error> exist under response?
		pugi::xml_node response = doc.child("response");
		std::ostringstream oss_error;
		oss_error << response.child_value("error");
		if (oss_error.str() != "Success") { BrokerError("Bad XML data."); throw ERROR_BAD_GET_POSITIONS; }

		holdings.clear();
		holdings.reserve(50);

		pugi::xml_node accountholdings = doc.child("response").child("accountholdings");
		for (pugi::xml_node x_holding = accountholdings.child("holding"); x_holding; x_holding = x_holding.next_sibling("holding"))
		{
			Holding holding;
			std::ostringstream oss_accounttype, oss_ins_matdt, oss_ins_sym, oss_ins_sectyp, oss_ins_strkpx, oss_ins_cfi, oss_qty;

			oss_accounttype << x_holding.child_value("accounttype");
			oss_qty << x_holding.child_value("qty");
			try { holding.net_qty = stoi(oss_qty.str()); }
			catch (...) { holding.net_qty = 0; }
			if (oss_accounttype.str() == std::to_string(at_MarginShort)) holding.net_qty *= -1;


			oss_ins_sym << x_holding.child("instrument").child_value("sym");
			holding.AllyAsset = oss_ins_sym.str();

			oss_ins_sectyp << x_holding.child("instrument").child_value("sectyp");
			if (oss_ins_sectyp.str() == "OPT")
			{
				oss_ins_matdt << x_holding.child("instrument").child_value("matdt"); // YYYY-MM-DD
				oss_ins_strkpx << x_holding.child("instrument").child_value("strkpx");
				oss_ins_cfi << x_holding.child("instrument").child_value("cfi");
				// example output: "SPY170714C00205000" = (SPY, 20170714, CALL, STRIKE = 00205.000)
				try {
					std::ostringstream oss;
					oss << oss_ins_matdt.str().substr(2, 2); // YY
					oss << oss_ins_matdt.str().substr(5, 2); // MM
					oss << oss_ins_matdt.str().substr(8, 2); // DD
					if (oss_ins_cfi.str()=="OP") oss << "P";  // "P"
					else if (oss_ins_cfi.str() == "OC") oss << "C"; // "C"
					else continue;
					oss << std::setw(8) << std::setfill('0') << (int)std::round(std::stod(oss_ins_strkpx.str()) * 1000); // "00205000"
					holding.AllyAsset += oss.str();
				}
				catch (...) { continue; }
			}
			holdings.push_back(holding);
		}
		return holdings;
	}
	DLLFUNC_C void CTestParseHoldings()
	{
		std::string response;
		GetResponse(&response, HGET, ACCOUNTS_ID_HOLDINGS, XML, "", "");
		std::vector <Holding> holdings = ParseHoldings(response);
		BrokerError("Begin parsing holdings...");
		int i = 0;
		for (auto & holding : holdings)
		{
			i++;
			std::ostringstream msg;
			msg << "AllyAsset found: ";
			msg << holding.AllyAsset;
			msg << ", net qty: ";
			msg << holding.net_qty;
			BrokerError(msg.str().c_str());
		}
		std::string msg2 = "Unique assets found: " + std::to_string(i);
		BrokerError(msg2.c_str());
		return;
	}
	double GetPosition(std::string Symbol)
	{
		std::string parseThis, AllyAsset = GetAllyAsset(Symbol.c_str());

		static StopWatch stopwatch;
		static std::vector <Holding> holdings;
		
		// do we need to make a server call?
		if (stopwatch.milliseconds_passed() > INTERVAL_GET_POSITION_MS)
		{
			GetResponse(&parseThis, HGET, ACCOUNTS_ID_HOLDINGS, XML, "", "");
			if(diag)SaveMessage(parseThis, "GetHoldings");
			holdings = ParseHoldings(parseThis);
			stopwatch.reset();
		}

		// check buffer for matches
		for (auto & holding : holdings)
		{
			if (!isCaseInsensitveMatch(holding.AllyAsset, AllyAsset)) continue;	// skip  mismatch
			return holding.net_qty;
		}

		// no matches.
		return 0.;


	}

	DLLFUNC_C double BrokerCommand(int Command, DWORD dwParameter)
	{
		static int SetMultiplier;
		static std::string SetSymbol;

		std::string Data, response; 
		std::vector <CONTRACT> Contracts;
		CONTRACT* contracts_ptr = (CONTRACT*)dwParameter;  // for GET_OPTIONS, this is a pointer to a CONTRACT array with 10,000 elements.
		int i = 0;
		double price=0., spread=0.;

		switch(Command)
		{
		case GET_COMPLIANCE:
			return 15; // full NFA compliant

			break;
		case GET_POSITION: 
			try { return GetPosition(SetSymbol); }
			catch (...) { BrokerError("Unable to get positions!"); return 0.0;}
			
			
			break;
		case GET_OPTIONS: 
			assetList.PurgeOldOptionEntries();
			Data = "symbol="+ SetSymbol
				+ "&fids=ask,bid,symbol,strikeprice,put_call,prem_mult,op_style,datetime,undersymbol,xdate,xday,xmonth,xyear,secclass"
				+ "&query=xdate-gte:" + convertTime(currentTime(), STR_YYYYMMDD)
				+ " AND xdate-lte:" + convertTime(currentTime()+365., STR_YYYYMMDD)
				;
			GetResponse(&response, HPOST, MARKET_OPTIONS_SEARCH, XML, "", (char*)Data.c_str());
			if(diag)SaveMessage(response, "BC_GET_OPTIONS_" + SetSymbol);

			
			try { Contracts = ParseOptionsContracts(response); }
			catch (...) { BrokerError("Options parse failure."); return 0; }

			for (i = 0; i < Contracts.size() && i < 10000; i++)
			{
				contracts_ptr[i] = Contracts[i];
			}
			return i;
			
			break;
		case GET_UNDERLYING: 
			price = GetUnderlyingPrice(LastBrokerAsset);
			return price;
			
			break;
		case SET_SYMBOL: 
			SetSymbol = (char*)dwParameter;
			return 1;

			break;
		case SET_MULTIPLIER: 
			SetMultiplier = (int)dwParameter;
			return 1;
			
			break;
		case SET_COMBO_LEGS:
			if (((int)dwParameter != 2) && ((int)dwParameter != 3) && ((int)dwParameter != 4))	return 0;
			numLegs= (int)dwParameter;  // set global
			optionLegs.clear();
			optionLegs.reserve(4);
			return 1;

			break;

		case SET_DIAGNOSTICS:
			if ((int)dwParameter == 1 || (int)dwParameter == 0)
			{
				diag = (int)dwParameter;
				return 1;
			}
			else return 0;

			break;
		default: return 0;  break;
		}
		return 0;
	}



}