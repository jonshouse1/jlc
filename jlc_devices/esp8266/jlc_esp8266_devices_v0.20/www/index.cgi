<html>

<title>
Control</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-15">
<!---
fragment_pagestart, don't directly edit these pages, use make_www instead
-->

<!--- fragment_stylesheet -->
<style>
A.type1:link    {color:#FF0000; text-decoration:overline; color:red}
A.type1:visited {color:#FF0000; text-decoration:none; color:cyan}
A.type1:active  {color:#FF0000; text-decoration:none; color:white; background:green}
A.type1:hover   {color:#FF0000; text-decoration:none; color:white; background:green}

A.type2:link    {color:#FF0000; text-decoration:none; color:white; font-size:25px}
A.type2:visited {color:#FF0000; text-decoration:none; color:white; font-size:25px}
A.type2:active  {color:#FF0000; text-decoration:none; color:white; background:red; font-size:25px}
A.type2:hover   {color:#FF0000; text-decoration:none; color:white; background:red; font-size:25px}

A.type3:link    {color:#000000; text-decoration:none;}
A.type3:visited {color:#000000; text-decoration:none;}
A.type3:active  {color:#000000; text-decoration:none;}
A.type3:hover   {color:#FF0000; text-decoration:overline underline;}

.imagered{
	border-style:solid;
	color:red;
	border-color:red;
	border-width:5px;
      }
	.imagered:hover {border-color:#FF10FF;}
.imagegreen{
	border-style:outset;
	border-color:green;
	border-width:5px;
      }
	.imagegreen:hover {border-color:#FF10FF;}
.imageblue{
	border-style:outset;
	border-color:blue;
	border-width:5px;
      }
	.imageblue:hover {border-color:#FF10FF;}
.imageyellow{
	border-style:outset;
	border-color:yellow;
	border-width:5px;
      }
	.imageyellow:hover {border-color:#FF10FF;}
.imageblack{
	border-style:outset;
	border-color:black;
	border-width:5px;
      }
	.imageblack:hover {border-color:#FF10FF;}

/* code added to have the same rows in IE and Gecko */	
body {  background-repeat: repeat-x;
	background-position: top center; }
pre {margin:0;}
}

<style type="text/css">
#bar{
	width:200px;
	height:25px;
	border:10px solid red;
	position:relative;
}
</style>

<style>
.button {
    background-color: #17ac03;
    border: none;
    color: white;
    padding: 10px 32px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    margin: 4px 2px;
    cursor: pointer;
}
.buttonc {
    background-color: #3060ff;
    border: none;
    color: white;
    padding: 10px 32px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 8px;
    margin: 4px 2px;
    cursor: pointer;
}
</style>

<!-- Generated using: http://www.cssportal.com/style-input-range/ -->
<style>
input[type=range] {
  -webkit-appearance: none;
  margin: 10px 0;
  width: 100%;
}
input[type=range]:focus {
  outline: none;
}
input[type=range]::-webkit-slider-runnable-track {
  width: 100%;
  height: 11px;
  cursor: pointer;
  animate: 0.2s;
  box-shadow: 1px 1px 1px #000000;
  background: #74A9D8;
  border-radius: 1px;
  border: 0px solid #010101;
}
input[type=range]::-webkit-slider-thumb {
  box-shadow: 1px 1px 1px #000031;
  border: 1px solid #00001E;
  height: 26px;
  width: 26px;
  border-radius: 15px;
  background: #FFFFFF;
  cursor: pointer;
  -webkit-appearance: none;
  margin-top: -8px;
}
input[type=range]:focus::-webkit-slider-runnable-track {
  background: #74A9D8;
}
input[type=range]::-moz-range-track {
  width: 100%;
  height: 11px;
  cursor: pointer;
  animate: 0.2s;
  box-shadow: 1px 1px 1px #000000;
  background: #74A9D8;
  border-radius: 1px;
  border: 0px solid #010101;
}
input[type=range]::-moz-range-thumb {
  box-shadow: 1px 1px 1px #000031;
  border: 1px solid #00001E;
  height: 26px;
  width: 26px;
  border-radius: 15px;
  background: #FFFFFF;
  cursor: pointer;
}
input[type=range]::-ms-track {
  width: 100%;
  height: 11px;
  cursor: pointer;
  animate: 0.2s;
  background: transparent;
  border-color: transparent;
  color: transparent;
}
input[type=range]::-ms-fill-lower {
  background: #74A9D8;
  border: 0px solid #010101;
  border-radius: 2px;
  box-shadow: 1px 1px 1px #000000;
}
input[type=range]::-ms-fill-upper {
  background: #74A9D8;
  border: 0px solid #010101;
  border-radius: 2px;
  box-shadow: 1px 1px 1px #000000;
}
input[type=range]::-ms-thumb {
  box-shadow: 1px 1px 1px #000031;
  border: 1px solid #00001E;
  height: 26px;
  width: 26px;
  border-radius: 15px;
  background: #FFFFFF;
  cursor: pointer;
}
input[type=range]:focus::-ms-fill-lower {
  background: #74A9D8;
}
input[type=range]:focus::-ms-fill-upper {
  background: #74A9D8;
}
</style>



<!--- fragment_menu -->
<body bgcolor="#FFFFFF" text="#000000" leftmargin=" 0" topmargin=" 0"marginwidth=" 0" marginheight=" 0" background="backwash.jpg">
<table width=" 100%" border=" 0" cellspacing=" 0" cellpadding=" 0">
<tr>
<td>
<table width=" 100% " border=" 0" cellspacing=" 0" cellpadding=" 0">
<tr>
<td width=" 17"><img src="pixel.gif" width=" 17" height=" 40"></td>
<td width=" 194">
<img src="logo.jpg" border=" 0"></td>
<td width=" 60"><img src="pixel.gif" width=" 60" height=" 40"></td>
<td>
<table width=" 100" border=" 0" cellspacing=" 0" cellpadding=" 0">
<tr>
<td><a href="index.cgi" class="type2" target="_self">&nbsp;&nbsp;CONTROL&nbsp;&nbsp;</a></td>
<td><a href="settings.cgi" class="type2" target="_self">&nbsp;&nbsp;SETTINGS&nbsp;&nbsp;</a></td>
<td><a href="help.cgi" class="type2" target="_self">&nbsp;&nbsp;HELP&nbsp;&nbsp;</a></td>
</tr>
</table>
</td>
</tr>
</table>
</td>
</tr>


<tr>
<td>
<table width=" 100%" border=" 0" cellspacing=" 0" cellpadding=" 0">
<tr>
<td width=" 17"><img src="pixel.gif" width=" 17" height=" 17"></td>
<td width=" 123"><img src="pixel.gif" width=" 123" height=" 17"></td>
<td>
<table width=" 100" border=" 0" cellspacing=" 0" cellpadding=" 0">
<tr>
<td></a></td>
<td></a></td>
</tr>
</table>
</td>
</tr>
</table>
</td>
</tr>


</table>

<script type=text/javascript>
<!--- slider4.js -->
var bar, sliderbright, slider2, btnon,btnoff, btnfifteen;
var slidermotor;
var mytimer;

function init(){
	bar = document.getElementById('bar');
	sliderbright = document.getElementById('sliderbright');
//sliderbright.value=255;
        slider2 = document.getElementById('sliderMotor');
	info = document.getElementById('info');
        btn1 = document.getElementById('btn1');
        btn2 = document.getElementById('btn2');
        btnfifteen = document.getElementById('btnfifteen');
}



function brightnesschanged(channel,newbright)
{
   newbright=newbright;
   httpGet("PWM"+channel+"="+newbright);
}


function speedchanged(newspeed,factor)
{
   var newval=parseInt(newspeed*factor)
   httpGet("MOTR="+newspeed);
   outputUpdate(newspeed);
}


function ledsoff()
{
   clearInterval(mytimer);
   httpGet("PWM0=0");
   document.getElementById("btn1").style.backgroundColor="#00ff00";   
   document.getElementById("btn2").style.backgroundColor="#00ff00";   
}


function buttonPressed(btn)
{
   //clearTimeout(mytimer);
   var property=document.getElementById(btn)
   property.style.backgroundColor="#FF0000";  
   switch (btn)
   {
        case 'btnoff': 
           httpGet("PWM0=0");	
           sliderbright.value=0;
           document.getElementById("sliderbright").value=0;
        break;

        case 'btnon': 
       	   httpGet("PWM0=255");
           sliderbright.value=255;
           document.getElementById("sliderbright").value="255";
        break;   

        case 'btnfwd': 
           httpGet("DIRE=F");
        break;

        case 'btnrev': 
           httpGet("DIRE=R");
        break;
   }

   setTimeout(function(){
		property.style.backgroundColor="#FFFFFF";
			}, 500)
   //mytimer=setInterval(function(){httpGet("brightness.cgi?brightness=-4")},1000);
   //mytimer=setInterval(function(){ledsoff()},2000);
}


function buttonTimerPressed(btn,rnum,minutes)
{

   var property=document.getElementById(btn);
   var oldcolor = '"'+document.getElementById(btn).backgroundColor+'"';
   document.getElementById(btn).backgroundColor ="red";
   //var timeinterval = parseInt(minutes) * 60 * 4;
   property.style.backgroundColor="red";   
   //alert("I am an alert box!"+minutes);
   httpGet("REL"+rnum+"="+minutes);	   
   setTimeout(function(oldcolor){
		property.style.backgroundColor="#17ac03";

		if (btn=='btncancel')
		property.style.backgroundColor="#3060ff";
			}, 500)
   //mytimer=setInterval(function(){httpGet("brightness.cgi?brightness=-4")},1000);
   //mytimer=setInterval(function(){ledsoff()},2000);
}



function volumecontrolchanged(newvolume, maxcolumn)
{
    var newbright;
    //maxcolumn = length = no of bars on control
    newbright = maxcolumn-newvolume;

    newbright=newbright*10;
    drawvolumecontroller(20,35,newvolume);
    document.getElementById("volumeindicator").innerHTML = newbright;

    var myRequest = new XMLHttpRequest();
    httpGet("PWM0="+newbright);
}


function httpGet(theURL)
{
    var xmlHttp = null;

    xmlHttp = new XMLHttpRequest();
    xmlHttp.open("GET", theURL, false);
    xmlHttp.send(null);
    return xmlHttp.responseText;
}

function outputUpdate(newval) {
	document.querySelector('#output1').value = parseInt(newval);
}

function resetButtonColour(thebtn) {
   document.getElementById(thebtn).style.backgroundColor="#ffffff";     
}


</script>
<!--- fragment_controlform_start -->
<!DOCTYPE html>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-15">

<body>
<body onload="init()">


<P> relay_opto_boardC</p>
