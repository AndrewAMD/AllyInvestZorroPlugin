// Zorro Beginner's Interface //////////////////////////
// redefine variables to less scary names //////////////

#define Slider1	g->vSlider[1]
#define Slider2	g->vSlider[2]
#define Slider3	g->vSlider[3]
#define CVolatility VolatilityC

#define PlotScale g->nPlotScale	
#define PlotWidth g->nPlotWidth
#define PlotHeight1 g->nPlotHeight1
#define PlotHeight2 g->nPlotHeight2
#define PlotDate	g->nPlotDate
#define PlotBars	g->nPlotBars
#define PlotPeriod g->nPlotPeriod
#define PlotMode	g->nPlotMode
#define ColorDn	g->dwColorDn
#define ColorUp	g->dwColorUp
#define ColorCandle	g->dwColorCandle
#define ColorEquity	g->dwColorEquity
#define ColorDD	g->dwColorDD
#define ColorWin	g->dwColorWin
#define ColorLoss	g->dwColorLoss
#define ColorBars	g->dwColorBars
#define ColorPanel g->dwColorPanel

#define PIP			g->asset->vPIP
#define PIPCost	g->asset->vPIPCost
#define MarginCost g->asset->vMarginCost
#define Leverage	g->asset->vLeverage
#define LotAmount	g->asset->vLotAmount
#define Spread		g->asset->vSpread
#define RollLong	g->asset->vRollLong
#define RollShort	g->asset->vRollShort
#define WinPayout g->asset->vWinPayout
#define LossPayout g->asset->vLossPayout
#define Slippage	g->vSlippage
#define Commission g->asset->vCommission
#define InitialPrice	g->asset->vInitialPrice
#define AskPrice	g->asset->vPrice
#define TimeAsk	g->asset->tAsk
#define TimeBid	g->asset->tBid
#define HedgeRatio g->asset->vFactor
#define Hedge		g->nHedge
#define Optimize	g->nOptimize
#define Fill		g->nFill
#define Multiplier g->asset->nMultiplier

#define Lots		g->vLots
#define Margin		g->vMargin
#define Risk		g->vRisk
#define Entry		g->vEntry
#define Stop		g->vStop
#define StopFactor g->vStopFactor
#define SpreadFactor g->vSpreadFactor
#define OrderLimit g->vOrderLimit
#define Trail		g->vTrail
#define TrailSlope g->vTrailSlope
#define TrailStep g->vTrailStep
#define TrailLock g->vTrailLock
#define TrailSpeed g->vTrailSpeed
#define TakeProfit g->vTakeProfit
#define ExitTime	g->nExitTime
#define LifeTime	g->nLifeTime
#define EntryTime	g->nEntryTime
#define EntryDelay g->vEntryDelay
#define Now			g->tNow

#define LookBack	g->nLookBack
#define UnstablePeriod	g->nUnstablePeriod
#define DataSkip	g->nDataSkip
#define DataSplit	g->nDataSplit
#define DataSlope	g->vDataSlope
#define BarPeriod g->vBarPeriod
#define BarOffset	g->nBarOffset
#define BarZone	g->nBarZone
#define FrameOffset g->nFrameOffset
#define TimeFrame	g->nTimeFrame
#define NumBarCycles	g->numSampleCycles
#define NumSampleCycles	g->numSampleCycles
#define NumOptCycles	g->numOptCycles
#define NumWFOCycles	g->numWFOCycles
#define NumTotalCycles g->numTotalCycles
#define NumDays	g->numDays
#define NumYears	g->numYears
#define StartDate	g->nStartDate
#define EndDate	g->nEndDate
#define StartWeek g->nStartWeek
#define EndWeek	g->nEndWeek
#define StartMarket g->nStartMarket
#define EndMarket	g->nEndMarket
#define UpdateDays g->nUpdateDays
#define ReTrainDays g->nReTrainDays
#define GapDays	g->nGapDays
#define MaxBars	g->nMaxBars
#define NumBars	g->numBars
#define NumMinutes g->numMinutes
#define Bar			g->nBar
#define StartBar	g->nFrameStart
#define EndBar		(g->nFrameStart+g->numFrameBars)
#define Day			g->nDay
#define SampleCycle	g->nSampleCycle
#define OptCycle	g->nOptCycle
#define WFOCycle	g->nWFOCycle
#define WFOBar		(g->nBar-g->nWFOStart)
#define WFOPeriod	g->nWFOPeriod
#define TotalCycle	g->nTotalCycle
#define TradesPerBar g->nTradesPerBar
#define MinutesPerDay g->nMinutesPerDay
#define TimeStep	g->nTimeStep

#define LoopNum1	g->nLoop[0]	
#define LoopNum2	g->nLoop[1]	
#define Loop1		g->pLoopPar[0]	
#define Loop2		g->pLoopPar[1]	
#define Cycle1		g->nLoopCycle[0]	
#define Cycle2		g->nLoopCycle[1]
#define NumLoops1	g->numLoops[0]
#define NumLoops2	g->numLoops[1]
#define SelectWFO	g->nSelectWFO
#define NumParameters g->numParTrain
#define ParCycle	g->nParCycle
#define StepCycle	g->nStepCycle
#define StepNext	g->bDoStep
#define Detrend	g->nDetrend
#define Weekend	g->nWeekend
#define FuzzyRange g->vFuzzyRange
#define FuzzyLevel g->vFuzzyLevel
#define MonteCarlo g->nMonteCarlo
#define Confidence g->nConfidence

#define Train		is(TRAINMODE)
#define Test		is(TESTMODE)
//#define Trade	is(TRADEMODE)
#define ReTrain	(is(PROCESS) && is(TRAINMODE) && !Core)
#define ReTest		(is(PROCESS) && is(TESTMODE) && !Core)
#define Command	(g->nCommand)
#define NumCores	g->numCores
#define Core		g->nCore
#define Verbose	g->nVerbose
#define SaveMode	g->nSaveMode
#define BoxState	g->nBoxState
#define TickTime	g->nTickTime
#define TockTime	g->nTockTime
#define TickSmooth g->nTickAvg
#define TickFix	g->nTimeFix
#define Outlier	g->vOutlier

#define Capital	g->vCapital
#define Balance	g->vBalance
#define Equity		g->vEquity
#define MarginVal	g->vMarginSum
#define RiskVal	g->vRiskSum
#define MarginMax	g->w.vMarginMax
#define RiskMax	g->w.vRiskMax
#define TradeVal	(g->vWinVal-g->vLossVal)

#define Assets			(g->pAssets)
#define ThisAsset		g->asset
#define Asset			g->asset->sName
#define AssetBar		g->asset->nBar
#define AssetFrame	g->asset->nFrame
#define AssetZone		g->asset->nZone
#define AssetMarket	g->asset->nMarket
#define AssetVar		(g->asset->Skill)
#define AssetStr		(string)&(g->asset->Skill)
#define AssetFactor	g->vAssetFactor
#define LotsPool		g->asset->nNetSum
#define LotsPhantom	g->asset->nPhantomSum
#define Symbol			g->asset->sSymbol
#define Component		g->statLong->nComponent

#define PatternCount	g->vAdvise[0]
#define PatternRate	g->vAdvise[1]
#define PatternPF		g->vAdvise[2]

#define AssetList		g->sAssetList
#define FactorList	g->sFactorList
#define Algo			g->sAlgo
#define Script			g->sScript
#define Factors		g->sFactors
#define Broker			g->sBroker
#define Account		g->sAccount
#define History		g->sHistory
#define WebFolder		g->sWebFolder
#define ZorroFolder	g->sZorroFolder
#define Curves			g->sCurves

#define WinLong		g->statLong->vWin
#define WinShort		g->statShort->vWin
#define WinTotal		g->w.vWin
#define LossLong		g->statLong->vLoss
#define LossShort		g->statShort->vLoss
#define LossTotal		g->w.vLoss
#define WinValLong	g->statLong->vWinVal
#define WinValShort	g->statShort->vWinVal
#define WinValTotal	g->vWinVal
#define LossValLong	g->statLong->vLossVal
#define LossValShort	g->statShort->vLossVal
#define LossValTotal	g->vLossVal
#define NumWinLong	g->statLong->numWin
#define NumWinShort	g->statShort->numWin
#define NumWinTotal	g->w.numWin
#define NumLossLong	g->statLong->numLoss
#define NumLossShort	g->statShort->numLoss
#define NumLossTotal	g->w.numLoss
#define WinStreakLong	g->statLong->nWinStreak
#define WinStreakShort	g->statShort->nWinStreak
#define WinStreakTotal	g->nWinStreak
#define LossStreakLong	g->statLong->nLossStreak
#define LossStreakShort	g->statShort->nLossStreak
#define LossStreakTotal	g->nLossStreak
#define WinStreakValLong	g->statLong->vWinStreakVal
#define WinStreakValShort	g->statShort->vWinStreakVal
#define WinStreakValTotal	g->vWinStreakVal
#define LossStreakValLong	g->statLong->vLossStreakVal
#define LossStreakValShort	g->statShort->vLossStreakVal
#define LossStreakValTotal	g->vLossStreakVal
#define NumWinningLong	g->statLong->numWinning
#define NumWinningShort	g->statShort->numWinning
#define NumLosingLong	g->statLong->numLosing
#define NumLosingShort	g->statShort->numLosing
#define NumShortTotal	g->numShort
#define NumLongTotal		g->numLong
#define NumOpenLong		(NumWinningLong+NumLosingLong)
#define NumOpenShort		(NumWinningShort+NumLosingShort)
#define NumOpenTotal		(NumShortTotal+NumLongTotal)
#define NumOpenPhantom	g->numPhantom
#define NumPendingLong	g->statLong->numPending
#define NumPendingShort	g->statShort->numPending
#define NumPendingTotal	g->numPending
#define NumRejected		g->numRejected
#define NumTrades			g->numTrades
#define WinMaxLong		g->statLong->vWinMax
#define WinMaxShort		g->statShort->vWinMax
#define WinMaxTotal		g->w.vWinMax
#define LossMaxLong		g->statLong->vLossMax
#define LossMaxShort		g->statShort->vLossMax
#define LossMaxTotal		g->w.vLossMax
#define BalanceLong		(WinLong-LossLong)
#define BalanceShort		(WinShort-LossShort)
#define EquityLong		(BalanceLong+WinValLong-LossValLong)
#define EquityShort		(BalanceShort+WinValShort-LossValShort)
#define ProfitClosed		(WinLong-LossLong+WinShort-LossShort)
#define ProfitOpen		(WinValLong-LossValLong+WinValShort-LossValShort)
#define OptimalFLong		g->statLong->vOptimalF
#define OptimalFShort	g->statShort->vOptimalF
#define OptimalF			g->statShort->vOptimalF2
#define OptimalFRatio	g->vOptimalFRatio
#define ObjectiveLong	g->statLong->vTrainPF
#define ObjectiveShort	g->statShort->vTrainPF
#define AlgoVar2			(g->statLong->Skill)
#define AlgoVar			(g->statShort->Skill)
#define ResultLong		(g->statLong->Result)
#define ResultShort		(g->statShort->Result)
#define ThisPerformance	(g->w)
//#define DailyLong		(g->statLong->pCurve)
//#define DailyShort		(g->statShort->pCurve)
//#define DailyTotal		(g->pCurve)

#define ThisTrade			g->tr
#define TradePriceOpen	g->tr->fEntryPrice
#define TradePriceClose	g->tr->fExitPrice
#define TradeResult		g->tr->fResult
#define TradeProfit		g->tr->fResult
#define TradeStopLimit	g->tr->fStopLimit
#define TradeStopDiff	g->tr->fStopDiff
#define TradeProfitLimit g->tr->fProfitLimit
#define TradeTrailLimit g->tr->fTrailLimit
#define TradeTrailDiff	g->tr->fTrailDiff
#define TradeTrailSlope g->tr->fTrailSlope
#define TradeTrailStep	g->tr->fTrailStep
#define TradeTrailLock	g->tr->fTrailLock
#define TradeEntryLimit g->tr->fEntryLimit
#define TradeUnits		g->tr->fUnits
#define TradeExitTime	g->tr->nExitTime
#define TradeTime			(g->nBar-g->tr->nBarOpen)
#define TradeLots			g->tr->nLots
#define TradeID			g->tr->nID
#define TradeBarOpen		g->tr->nBarOpen
#define TradeBarClose	g->tr->nBarClose
#define TradeMAE			g->tr->fMAE
#define TradeMFE			g->tr->fMFE
#define TradeRoll			g->tr->fRoll
#define TradeContract	g->tr->nContract
#define TradeStrike		g->tr->fStrike
#define TradeUnderlying	g->tr->fUnl
#define TradeVar			(g->tr->Skill)
#define TradeParameter	(g->tr->fArg)
#define TradeDir			ifelse(TradeIsShort,-1,1)
#define TradeStatus		((STATUS*)(g->tr->status))
#define TradeAlgo			TradeStatus->sAlgo
#define TradeAsset		TradeStatus->asset->sName
#define TradeSpread		TradeStatus->asset->vSpread
#define TradePIP			TradeStatus->asset->vPIP

// for() macros
#define long_trades		forTrade(4); g->bFor; forTrade(5) // asset/algo trades only
#define short_trades		forTrade(12); g->bFor; forTrade(13) // asset/algo trades only
#define current_trades	forTrade(20); g->bFor; forTrade(21) // asset/algo trades only
#define open_trades		forTrade(0); g->bFor; forTrade(1) // open trades only
#define all_trades		forTrade(2); g->bFor; forTrade(3) // all trades

#define TradeFlag(flag)	((g->tr->flags&(flag)) != 0)
#define TradeIs(flag)	((g->tr->flags&(flag)) == (flag))
#define TradeIsOpen		TradeFlag(TR_OPEN)
#define TradeIsPending	TradeFlag(TR_WAITBUY)
#define TradeIsClosed	(!TradeIsOpen && TradeFlag(TR_STOPPED|TR_PROFIT|TR_TIME|TR_SOLD|TR_EXIT|TR_REMOVED|TR_REVERSED))
#define TradeIsShort		TradeFlag(TR_BID)
#define TradeIsLong		(!TradeFlag(TR_BID))
#define TradeIsPhantom	TradeFlag(TR_PHANTOM)
#define TradeIsPool		((g->tr->flags&(TR_PHANTOM|TR_NET)) == TR_NET)
#define TradeIsVirtual	((g->tr->flags&(TR_PHANTOM|TR_NET)) == (TR_PHANTOM|TR_NET))
#define TradeIsNewBar	(g->tr->nBarClose != g->nBar)
#define TradeIsStop		TradeIs(TR_EVENT|TR_STOPPED)
#define TradeIsProfit	TradeIs(TR_EVENT|TR_PROFIT)
#define TradeIsEntry		TradeIs(TR_EVENT|TR_WAITBUY)
#define TradeIsContract	((g->tr->nContract&(PUT|CALL|FUTURE)) != 0)

#define YMDHMS	"%y%m%d %H:%M:%S"
#define HMS	"%H:%M:%S"
#define YMD	"%Y-%m-%d"

#define loadHistory	assetHistory
