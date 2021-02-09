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


