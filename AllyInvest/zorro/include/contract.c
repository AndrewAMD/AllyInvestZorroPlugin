// options / futures utilities ///////////////////////////////
#include <r.h>

#define Contracts		(g->asset->pContracts)
#define NumContracts g->asset->numContracts
#define ThisContract	g->contract
#define ContractAsk	((var)(ThisContract->fAsk))
#define ContractBid	((var)(ThisContract->fBid))
#define ContractVol	((var)(ThisContract->fVol))
#define ContractStrike ((var)(ThisContract->fStrike))
#define ContractExpiry ThisContract->Expiry
#define ContractType	ThisContract->Type

// Quantlib Interface //////////////////////////////////////////
//#define DO_LOG

void contractPrint(CONTRACT* c,int To)
{
	if(!c) return; //print(To,"\n---------------");
	print(To,"\n%s,%s%s%s,%i,%.4f,%.4f,%.4f,%.4f,%i",
		ifelse(is(TRADEMODE),(string)c,strdate("%Y-%m-%d",c->time)),
		ifelse(c->Type&FUTURE,"Future",""),
		ifelse(c->Type&PUT,"Put",""),
		ifelse(c->Type&CALL,"Call",""),
		c->Expiry,(var)c->fStrike,(var)c->fUnl,(var)c->fAsk,(var)c->fBid,(int)c->fVal);
}

int RQLInitialized = 0;

void initRQL()
{
	Rstart(0,2);
	Rx("library('RQuantLib',quiet=TRUE)");
	RQLInitialized = 1;
}

var contractVal(CONTRACT* c,var Price,var HistVol,var Dividend,var RiskFree,
	var* Delta,var* Gamma,var* Vega,var* Theta,var* Rho,...)
{	
	if(!RQLInitialized) initRQL();

	Rset("Strike",(double)c->fStrike);
	Rset("Expiry",contractDays(c)/365.25);
	Rset("Price",Price);
	Rset("Volatility",HistVol);
	Rset("Dividend",Dividend);
	Rset("RiskFree",RiskFree);
	if(c->Type&PUT)
		Rset("Type","put");
	else
		Rset("Type","call");
	if(c->Type&BINARY) {
		if(c->Type&EUROPEAN)
			Rset("ExcType","european");
		else
			Rset("ExcType","american");
		Rx("Option <- BinaryOption('cash',Type,ExcType,Price,Price,Dividend,RiskFree,Expiry,Volatility,Strike)");
	} else {
		if(c->Type&EUROPEAN)
			Rx("Option <- EuropeanOption(Type,Price,Strike,Dividend,RiskFree,Expiry,Volatility)");
		else
			Rx("Option <- AmericanOption(Type,Price,Strike,Dividend,RiskFree,Expiry,Volatility,engine='CrankNicolson')");
	}
	var Value = Rd("Option$value");
	if(Delta) *Delta = Rd("Option$delta");
	if(Gamma) *Gamma = Rd("Option$gamma");
	if((Vega || Theta || Rho) && !(c->Type&EUROPEAN)) // calculate Vega etc. always with Black-Scholes
		Rx("Option <- EuropeanOption(Type,Price,Strike,Dividend,RiskFree,Expiry,Volatility)");
	if(Vega) *Vega = Rd("Option$vega");
	if(Theta) *Theta = Rd("Option$theta");
	if(Rho) *Rho = Rd("Option$rho");
#ifdef DO_LOG
	printf("\n%s: %i(%.2f) %.2f %.2f %.2f => %.2f",
		strdate("%Y-%m-%d",c->time),c->Expiry,contractDays(c)/365.25,
		(var)c->fStrike,Price,HistVol,Value);
#endif	
	return Value;
}

var contractVol(CONTRACT* c,var Price,var HistVol,var Value,var Dividend,var RiskFree)
{	
	if(!RQLInitialized) initRQL();
	var Strike = (double)c->fStrike;
	if((c->Type&PUT) && Price+Value <= Strike) return 0.;	// otherwise, RQL crash
	if((c->Type&CALL) && Price-Value >= Strike) return 0.;

	Rset("Strike",Strike);
	Rset("Expiry",contractDays(c)/365.25);
	Rset("Price",Price);
	Rset("Value",Value);
	Rset("Volatility",HistVol);
	Rset("Dividend",Dividend);
	Rset("RiskFree",RiskFree);

	if(c->Type&PUT)
		Rset("Type","put");
	else
		Rset("Type","call");

//	printf("#\nInput: %.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
//		Rd("Value"),Rd("Price"),Rd("Strike"),Rd("Dividend"),Rd("RiskFree"),Rd("Expiry"),Rd("Volatility"));
		
	if(c->Type&EUROPEAN)
		Rx("Vol <- EuropeanOptionImpliedVolatility(Type,Value,Price,Strike,Dividend,RiskFree,Expiry,Volatility)");
	else
		Rx("Vol <- AmericanOptionImpliedVolatility(Type,Value,Price,Strike,Dividend,RiskFree,Expiry,Volatility)");

	var Vol = Rd("Vol");
#ifdef DO_LOG
	static int warned = 0;
	if(!warned++ && (Verbose&4))
		printf("#\nVol %.2f: %.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
			Rd("Vol"),Rd("Value"),Rd("Price"),Rd("Strike"),Rd("Dividend"),Rd("RiskFree"),Rd("Expiry"),Rd("Volatility"));
			//Vol,Value,Price,Strike,Dividend,RiskFree,contractDays(c)/365.25,HistVol);
#endif
	return Vol;
}

var contractIntrinsic(CONTRACT* c,var Price)
{
	if(!c) return 0;
	if(c->Type&CALL) return Price - c->fStrike;
	else if(c->Type&PUT) return c->fStrike - Price;
	else return 0;
}

var contractIntrinsic(TRADE* tr,var Price)
{
	return contractIntrinsic(contract(tr),Price);
}

var dataFromQuandl(int Handle, string Format,string Code,int Column)
{
	string Filename = strxc(Code,'/','-');
	if(dataFind(Handle,0) < 0) { // data array not yet loaded
		dataDownload(Code,FROM_QUANDL,12*60);
		if(!dataParse(Handle,Format,Filename)) return -0.01;
	}
	if(is(TRADEMODE) && !is(LOOKBACK)) {
		strcat(Filename,"1");
		int Rows = dataDownload(Code,FROM_QUANDL|1,60);
		if(Rows) dataParse(Handle,Format,Filename);	// add new record to the end of the array
		return dataVar(Handle,-1,Column);
	} else {
		int Row = dataFind(Handle,wdate());
		return dataVar(Handle,Row,Column); 
	}
}

// US treasury 3-months interest rate
var yield() { 
	return dataFromQuandl(801,"%Y-%m-%d,f","FRED/DTB3",1); 
}

// empirical volatility for options
var VolatilityOV(int Days)
{
	var a = 1, alpha = 0.92, numer = 0, denom = 0;
	int i;
	for(i=1; i<=Days; i++) {
		var SV = .627 * sqrt(365.25) * log(dayHigh(ET,i)/max(1,dayLow(ET,i)));
		numer += a*SV;
		denom += a;
		a *= alpha;
	}
	return numer/denom;
}

// convert DATE to YYYYMMDD
int ymd(var Date)
{
	var Prev = Now; Now = Date;
	int Result = day(NOW) + 100*month(NOW) + 10000*year(NOW);
	Now = Prev; 
	return Result;
}

// convert YYYYMMDD to DATE
var dmy(int ymd)
{
	int Year = ymd/10000;
	int Month = ymd/100 - 100*Year;
	int MDay = ymd - 100*(ymd/100);
	return wdatef("%Y %m %d",strf("%i %i %i",Year,Month,MDay));
}


// return the date of the Nth given weekday of the given month 
var nthDay(int Year,int Month,int Dow,int N)
{
	var Prev = Now;
	Now = wdatef("%Y %m",strf("%i %i",Year,Month));
	while(N > 0) {
		Now += 1.; // next day
		if(dow(NOW) == Dow) N--;
	}
	var Result = Now;	Now = Prev;
	return Result;
}

// open a duplicate of a closed trade
TRADE* contractRoll(TRADE* tr,int Days,var Strike,function TMF)
{
	if(Strike == 0.) Strike = tr->fStrike;
	CONTRACT* c = contract(tr->nContract&(FUTURE|PUT|CALL),Days,Strike);
	var Price = contractPrice(c);
	if(Price == 0.) {
		printf("\nCan't roll - no contract at strike %.2f",Strike);
		return 0;
	}
	Lots = tr->nLots;
	Stop = tr->fStopLimit*Price/tr->fEntryPrice;
	TakeProfit = tr->fProfitLimit*Price/tr->fEntryPrice;
	if(tr->flags&TR_BID)
		return enterShort(TMF);
	else
		return enterLong(TMF);	
}

TRADE* contractRoll(TRADE* tr,int Days)
{
	return contractRoll(tr,Days,0,0);
}

// print contracts to the log for testing
void contractPrint(int Handle, int To)
{
	set(LOGFILE);
	CONTRACT* c;
	int i;
	if(is(TRADEMODE))
		print(To,"\nClass,Type,Expiry,Strike,Underlying,Ask,Bid,Multiplier");
	else
		print(To,"\nDate,Type,Expiry,Strike,Underlying,Ask,Bid,OpenInterest");
	if(Handle) {
		while(c = dataStr(Handle,i++,0))
			contractPrint(c,To);
	} else { // print current contract chain
		for(i=0; i < NumContracts; i++)
			contractPrint(Contracts + i,To);
	}
}

