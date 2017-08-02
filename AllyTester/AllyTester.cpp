// AllyTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include "AllyInvest.h"


int main()
{
	AllyInvest::TestAssetList();
	AllyInvest::TestGetAllyAsset();
	AllyInvest::TestMakeStockOrder();
	AllyInvest::TestMakeOptionOrder();
	AllyInvest::TestMakeMultiLegOptionOrder();
	AllyInvest::TestDate();
	AllyInvest::TestIsCaseInsensitveMatch();
	return 0;
}

