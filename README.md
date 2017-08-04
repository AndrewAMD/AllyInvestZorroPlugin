# Ally Invest Plugin for Zorro

Ally Invest is an low-commission stocks and options broker for USA residents.  Ally offers its users a REST API, which uses XML and JSON.  Orders are placed in a FIXML-variant language.

This plugin was written in Win32 C/C++ using Visual Studio 2017.

## Build Instructions

All dependencies have been fully integrated into the folder and slightly modified where necessary. They are:

* liboauthcpp (for OAuth 1.0a authentication)
* pugixml (for XML parsing and writing)

This project includes a Visual Studio 2017 solution.  You should be able to simply download the entire folder, open the solution, and build it, using the __**Release x86**__ build configuration.

## Installation Instructions

To install the plugin, simply place the AllyInvest.dll file in the Plugin folder where Zorro is installed.

## Login Instructions

To login with the Ally Plugin, you will first need to set up API access with Ally Invest. After you have set up your account with Ally, you must do the following:

* Go to https://www.ally.com/invest/
* Click Log In and enter your credentials
* Click on your account (usually says "individual", depending on your type of account).
* At the top, go to "Tools" -> "Ally Invest API" -> "Manage Applications"
* Create a "Personal Application", and follow the steps. Your new application should have been created instantly.
* Scroll down. Under "Developer Applications", click on the application you created.
* You should see four scrambled keys.  You will need these.
* Open Zorro.  
* Under "User", enter your account number (not to be confused with your website login username)
* For the password, paste the values in one after the other, in the exact order below (no spaces, no commas):
  1. Consumer Key
  2. Consumer Secret
  3. OAuth Token
  4. OAuth Token Secret

## Plugin Capabilities

The following standard Zorro Broker API functions have been implemented:

* BrokerOpen
* BrokerHTTP
* BrokerLogin
* BrokerTime
* BrokerAsset
* BrokerHistory2
  * Historical data is only available for stocks in M1 and M5 formats.
  * Officially, the API only provides 5 days of history. (Nonetheless, the API appears to provide 1 year of equities history.)
  * No options history is available.
* BrokerBuy
  * The broker API does not include "trade management".
  * Instead, one buys/sells to open and buys/sells to close.
* BrokerCommand standard functions:
  * GET\_COMPLIANCE
  * GET\_POSITION
  * GET\_OPTIONS
  * GET\_UNDERLYING
  * SET\_SYMBOL
  * SET\_MULTIPLIER

Two non-standard BrokerCommand functions have also been implemented:
* SET\_COMBO\_LEGS
  * #define SET_COMBO_LEGS 137
  * Input: Only accepts 2, 3, or 4 as an input, for 2-leg, 3-leg, and 4-leg orders, respectively.
  * Returns 1 if command accepted, 0 if rejected.
  * To use, call the function and then call the orders immediately.
  * For example, if you SET_COMBO_LEGS to 2 and then request two options orders, the order will finally process after the second options order is received.
  * If the order fails, the last leg will return a failure.  The script writer will be responsible for zeroing out the prior legs.
  * All of the legs must have matching **expiration months** and **underlying symbols**.  Otherwise, the order will not be submitted.
  * Below is an example of a two-leg order message the plugin might submit to the Ally Invest servers:
```
<FIXML xmlns="http://www.fixprotocol.org/FIXML-5-0-SP2">
  <NewOrdMleg TmInForce="0" Px="-3.10" OrdTyp="2" Acct="12345678">
    <Ord OrdQty="4" PosEfct="O">
      <Leg Side="1" Strk="190" Mat="2014-01-18T00:00:00.000-05:00" MMY="201401" SecTyp="OPT" CFI="OC" Sym="IBM"/>
    </Ord>
    <Ord OrdQty="4" PosEfct="O">
      <Leg Side="2" Strk="200" Mat="2014-01-18T00:00:00.000-05:00" MMY="201401" SecTyp="OPT" CFI="OC" Sym="IBM"/>
    </Ord>
  </NewOrdMleg>
</FIXML>
```
* SET\_DIAGNOSTICS
  * #define SET\_DIAGNOSTICS 138
  * Input: 1 to enable, 0 to disable.
  * Returns 1 of command accepted, 0 if command rejected.
  * When enabled, all xml communications will be dumped into the Zorro\Log folder for diagnostic purposes.

## Known Issues
1. On holidays, broker API might say that the market is open when it is closed.  
  1. Broker has not solved this issue as of 2017-05.
2. Server will sometimes return ask/bid quotes of 0.00 after hours / on weekends.
  1. Plugin might use the latest historical M1 data and treat it as a quote when quote is unavailable.  However, it will declare a spread of zero.

## License

See the LICENSE.md file.

## Resources

* [The Zorro Project](http://zorro-project.com/)
* [The Zorro Project - Manual](http://zorro-project.com/manual/)
* [Ally Invest](https://www.ally.com/invest/)
* [Ally Invest API Documentation](https://www.ally.com/api/invest/documentation/getting-started/)
