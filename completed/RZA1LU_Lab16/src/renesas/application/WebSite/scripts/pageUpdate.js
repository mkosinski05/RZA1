var	gDateStampFileName = "get_time.cgi?";
var	gDateStampElementID =	"realTimeClock";
var	gDateStampHttpRequest;

var	gADCFileName = "get_adc.cgi?";
var	gADCElementID =	"adcUpdateValue";
var	gADCValueHttpRequest;

var	gSwitchFileName = "get_switch.cgi?";
var	gSwitchElementID =	"switchMonitor";
var	gSwitchValueHttpRequest;

var	gTimerStarted = 0;
var	gTimeOut;

function CreateHttpRequest()
{
	var xmlhttp = false;
	var e;
	if (window.XMLHttpRequest)
	{
		xmlhttp	= new XMLHttpRequest();
	}
	else
	{
		xmlhttp	= new ActiveXObject("Microsoft.XMLHTTP");
		try
		{
			httprequest=new	ActiveXObject("Msxml2.XMLHTTP");
		} 
		catch (e)
		{
			try
			{
				httprequest = new ActiveXObject("Microsoft.XMLHTTP");
			} catch	(e)
			{
			}
		}
	}
	return xmlhttp
}
function DateStampRequestUpdate()
{
	gDateStampHttpRequest	= CreateHttpRequest();
	if (gDateStampHttpRequest)
	{
		gDateStampHttpRequest.onreadystatechange = DateStampUpdateContents;
		gDateStampHttpRequest.open('GET', gDateStampFileName + " -nocache" + Math.random(), true);
		gDateStampHttpRequest.send(null);
		
	}
}

function DateStampUpdateContents()
{
	if (gDateStampHttpRequest.readyState == 4)
	{
		if (gDateStampHttpRequest.status == 200)
		{
			document.getElementById(gDateStampElementID).innerHTML = gDateStampHttpRequest.responseText;

		}
	}
}
function ADCpRequestUpdate()
{
	gADCValueHttpRequest	= CreateHttpRequest();
	if (gADCValueHttpRequest)
	{
		gADCValueHttpRequest.onreadystatechange = ADCUpdateContents;
		gADCValueHttpRequest.open('GET', gADCFileName + " -nocache" + Math.random(), true);
		gADCValueHttpRequest.send(null);
	}
}

function ADCUpdateContents()
{
	if (gADCValueHttpRequest.readyState == 4)
	{
		if (gADCValueHttpRequest.status == 200)
		{
			document.getElementById(gADCElementID).innerHTML = gADCValueHttpRequest.responseText;
		}
	}
}

function SwitchpRequestUpdate()
{
	gSwitchValueHttpRequest	= CreateHttpRequest();
	if (gSwitchValueHttpRequest)
	{
		gSwitchValueHttpRequest.onreadystatechange = SwitchUpdateContents;
		gSwitchValueHttpRequest.open('GET', gSwitchFileName + " -nocache" + Math.random(), true);
		gSwitchValueHttpRequest.send(null);
	}
}
function SwitchUpdateContents()
{
	if (gSwitchValueHttpRequest.readyState == 4)
	{
		if (gSwitchValueHttpRequest.status == 200)
		{
			document.getElementById(gSwitchElementID).innerHTML = gSwitchValueHttpRequest.responseText;
		}
	}
}

function UpdateTimer()
{
	var	dateStampRefreshInterval = 1000;

	SwitchpRequestUpdate();
	ADCpRequestUpdate();
	DateStampRequestUpdate();
	if (dateStampRefreshInterval)
	{
		gTimeOut = setTimeout("UpdateTimer()", dateStampRefreshInterval);
	}
}
function DateStampScreenUpdate()
{
	if (!gTimerStarted)
	{
		gTimerStarted = 1;
		UpdateTimer();
	}
}
