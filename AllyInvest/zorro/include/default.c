/////////////////////////////////////////////////////////////////////
// Default trading headers and functions
// (c) oP group 2012
/////////////////////////////////////////////////////////////////////
#ifndef default_c
#define default_c

#define PRAGMA_ZERO	// initialize local variables to zero
#define PRAGMA_POINTER // no pointer autodetection

#include <litec.h>	// common language header
#include <trading.h> // trading-specific structs
#include <variables.h> // trading variable re-definitions
#include <functions.h> // trading functions
#include <ta.h> // TA-Lib technical analysis functions

extern GLOBALS* g; // global variables struct

/////////////////////////////////////////////////////////////////////
// overloaded functions
var max(var a,var b);
int max(int a,int b);
var min(var a,var b);
int min(int a,int b);
var sign(var a);
int sign(int a);
var abs(var a);
int abs(int a);
var ifelse(BOOL c,var a,var b);
string ifelse(BOOL c,string a,string b);
var clamp(var a,var l,var h);
var clamp(int a,int l,int h);
BOOL between(var a,var l,var h);
BOOL between(int a,int l,int h);
var random(var limit);
var round(var val) { return round(val,0.); }

BOOL crossOver(var* a,var b); 
BOOL crossUnder(var* a,var b);
var crossOverF(var* a,var b); 
var crossUnderF(var* a,var b);

var optimize(var val,var start,var end,var step) { return optimize(val,start,end,step,0); }
var optimize(var val,var start,var end) { return optimize(val,start,end,0,0); }

int DominantPeriod(int Period) {	return DominantPeriod(series(price()),Period); }
int DominantPhase(int Period) {	return DominantPhase(series(price()),Period); }
var LowPass(var* Buffer,var Val,int Period);
var Spectrum(var* Data,int TimePeriod);
var markowitz(var* CovMatrix,var* Mean,int N,double Cap);

void assetAdd(string Name) { assetAdd(Name,50,0,0,0,0.01,0.01,0,1,1,0,Name); }

void plot(string name,int val,int type,int color) { plot(name,(var)val,type,color); }
void plot(string name,vars data,int type,int color) { plot(name,data[0],type,color); }

int memory() { return memory(0); }
int file_append(string name,string content,long size);
int panel(char* name,int color,int size);
mat matSet(mat M,int row,int col,mat A);
mat matSet(mat M,var c);

void dataSet(int Handle,int Row,int Col,int Value);
CONTRACT* contract(CONTRACT* c);
CONTRACT* contract(TRADE* tr);
var contractDays(TRADE* tr);
var contractPrice(TRADE* tr);
int contractPosition(TRADE* tr);

BOOL market(int zone);
int date();
int tod();
int ltod(int zone);
int tow();
int ltow(int zone);
var wdate();
string strdate(string format,int offset);
string strdate(string format,var date);
string strdate(string format) { return strdate(format,0); }
var strvar(string str,string name) { return strvar(str,name,0); }

bool is(long flag);
bool mode(long flag);
void set(long flag);
void reset(long flag);

/////////////////////////////////////////////////////////////////////
#define INDICATORS_H	// include headers only
#define OVERLOADED
#include <indicators.c>	// new indicator library

/////////////////////////////////////////////////////////////////////

// optimizing objective based on PRR
var objective()
{
	if(!NumWinTotal && !NumLossTotal) return 0.;
	var wFac = 1./sqrt(1.+NumWinTotal); 
	var lFac = 1./sqrt(1.+NumLossTotal);
	var win = WinTotal, loss = LossTotal;
// remove single outliers
	if(NumWinTotal > 2) win -= (NumWinTotal-2)*WinMaxTotal/NumWinTotal;
	if(NumLossTotal > 2) loss -= (NumLossTotal-2)*LossMaxTotal/NumLossTotal;
// return PRR
	return (1.-wFac)/(1.+lFac)*(1.+win)/(1.+loss);
}

// helper functions for limiting the open trades
TRADE* reverseLong(int MaxTrades)
{
// update the stops and profit targets of open trades
	var Price = priceClose();
	if(Stop > 0 && Stop < Price/2) // stop distance 
		exitLong(0,Price-Stop);
	else if(Stop >= Price/2) // stop limit
		exitLong(0,Stop); 
	if(TakeProfit > 0 && TakeProfit < Price/2) 
		exitLong(0,-(Price+TakeProfit));
	else if(TakeProfit >= Price/2) 
		exitLong(0,-TakeProfit);

// extend the exit time of open non-advise trades 
	if(ExitTime > 1)
		for(long_trades)
			if(TradeIsOpen && !(ThisTrade->flags&TR_PREDICT)) 
				TradeExitTime = TradeTime+ExitTime;

// if MaxTrades is not reached, open a new trade
	if(NumOpenLong < MaxTrades || is(TRAINMODE))
		return enterLong();
// otherwise, close any opposite trades
	else if(Hedge <= 1)
		exitShort();
	return 0;
}

TRADE* reverseShort(int MaxTrades)
{
	var Price = priceClose();
	if(Stop > 0 && Stop < Price/2) 
		exitShort(0,Price+Stop); 
	else if(Stop >= Price/2)
		exitShort(0,Stop); 
	if(TakeProfit > 0 && TakeProfit < Price/2) 
		exitShort(0,-(Price-TakeProfit));
	else if(TakeProfit >= Price/2) 
		exitShort(0,-TakeProfit);

	if(ExitTime > 1)
		for(short_trades)
			if(TradeIsOpen && !(ThisTrade->flags&TR_PREDICT)) 
				TradeExitTime = TradeTime+ExitTime;

	if(NumOpenShort < MaxTrades || is(TRAINMODE))
		return enterShort();
	else if(Hedge <= 1)
		exitLong();
	return 0;
}

TRADE* reverseLong(int MaxTrades,function f)
{
	reverseLong(0);
	if(NumOpenLong < MaxTrades)
		return enterLong(f);
	return 0;
}

TRADE* reverseShort(int MaxTrades,function f)
{
	reverseShort(0);
	if(NumOpenShort < MaxTrades)
		return enterShort(f);
	return 0;
}


// helper function for printing the date
string datetime()
{
	return strf("%02d.%02d.%02d %02d:%02d:%02d",
		day(),month(),year()-2000,hour(),minute(),(int)second());
}

int scriptVersion() { return SCRIPT_VERSION; }

#endif
