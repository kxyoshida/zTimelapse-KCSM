// Use in combination with Servo-LED Program on Arduino

var ports;
var active;
var pause=100;
var baudRate=9600;
var zLevel=0;
var durA=1000; //in ms
var durB=1000;
var durC=1000;
var ampA=255; // 0-255
var ampB=255;
var ampC=255;

var motorSpeed=30;
var zTolerance=3;

function initialize() {
	run("serial ext");

	ports=Ext.ports();
	print("Available ports="+ports);
	active=Ext.alive();
	if (Ext.alive()==1) {
		print("active="+active+"");
	        return true;
	}
	else {
		Ext.open("COM4",baudRate,"");
		active=Ext.alive();
		print("active="+active+"");
		Ext.write("r");
		return true;
	}
}


function terminate() {
	Ext.close();
	active=Ext.alive();
	print("active="+active+"");
}

function readZValues(logged) {
	stream=Ext.read();
	if (lengthOf(stream)>0) {
		data=split(stream);
		encoderPos=parseInt(data[0]);
		Setpoint=parseInt(data[1]);
		showStatus("Current="+encoderPos+" Setpoint="+Setpoint+"");
		if (logged)
			print("Current="+encoderPos+" Setpoint="+Setpoint+"\n");
		return true;
	} else {
		return false;
	}
}

macro "Initialize Communication [I]" {
        initialize();
} 

macro "End Communication [E]" {
        terminate();
}

macro "Light A ON [a]" {
	Ext.write("a");
}

macro "Light B ON [b]" {
	Ext.write("b");
}


macro "Light OFF [o]" {
	Ext.write("o");
}

macro "Lock focus [l]" {
	Ext.write("l");
}

macro "Unlock focus [u]" {
	Ext.write("u");
	wait(pause);
	while (!(readZValues(true))) {
		wait(pause);
	}
}

macro "Reset setpoint to current position [r]" {
	Ext.write("r");
}

macro "Reset z-Origin [R]" {
	Ext.write("R");
}

macro "Get the position value [v]" {
	Ext.write("V");
	wait(pause);
	while (!(readZValues(true))) {
		wait(pause);
	}
}

/* This macro has a problem */
// macro "Monitor the position value[m]" {
//         while (!isKeyDown("shift")) {
//               Ext.write("V");
// 	      wait(pause);
// 	      while (!(readZValues(false))) {
// 	      	    wait(pause);
// 	      }
// 	      wait(pause);
//        }
// }

macro "Set z-position [z]" {
	zLevel=getNumber("Input z-position (<=1000 at once, 1unit~0.1um)",zLevel);
	print("z"+zLevel+"\n");
	Ext.write("z"+zLevel+"/");
}

macro "Set Duration A [d]" {
	durA=getNumber("Input Duration A",1000);
	print("durA="+durA+"\n");
	Ext.write("d"+durA+"\n");
}

macro "Flash A [A]" {
	Ext.write("A");
}

macro "Set Duration B [e]" {
	durB=getNumber("Input Duration B",1000);
	print("durB="+durB+"\n");
	Ext.write("e"+durB+"\n");
}

macro "Flash B [B]" {
	Ext.write("B");
}

macro "Set Duration C [f]" {
	durC=getNumber("Input Duration C",1000);
	print("durC="+durC+"\n");
	Ext.write("f"+durC+"\n");
}

macro "Flash C [C]" {
	Ext.write("C");
}


macro "Set Motor Speed" {
	motorSpeed=getNumber("Input Motor Speed", motorSpeed);
	Ext.write("S"+motorSpeed+"\n");
}

macro "Set Z Tolerance" {
	zTolerance=getNumber("Input Z Tolerance in units", zTolerance);
	Ext.write("Z"+zTolerance+"\n");
}

macro "back-and-forth time-lapse" {
	var interval=5000;
	var curTime=0;
	var nexTime=0;
	var nPoints=2;
	var nCapture=12;
	var zSlice;

	do { 
	   nPoints=getNumber("Number of z-point (<=5)", nPoints);
	} while (nPoints<1 || nPoints>5)

	nCapture=getNumber("Number of total capture", nCapture);
	zSlice=newArray(nPoints);

	for (j=0;j<nPoints;j++) {
		zSlice[j]=getNumber("z at Slice["+j+"]", zSlice[j]);
	}

        initialize();

	if (nPoints==5){
		Ext.write("K"+zSlice[4]+"\n");
		wait(pause);
	}
	if (nPoints>=4) {
		Ext.write("J"+zSlice[3]+"\n");
		wait(pause);
	}
	if (nPoints>=3) {
		Ext.write("I"+zSlice[2]+"\n");
		wait(pause);
	}
	if (nPoints>=2) {
		Ext.write("H"+zSlice[1]+"\n");
		wait(pause);
	}
	Ext.write("G"+zSlice[0]+"\n");
	wait(pause);

	Ext.write("l");	// lock the focus

	curTime=getTime();
	nexTime=curTime+interval;

	for (i=0;i<nCapture;i++) {
		imod=i%nPoints;
		print("i="+i+"/"+nCapture+": z="+zSlice[imod]+"\n");
		
		if (imod==0) 
		   	Ext.write("g");
		else if (imod==1)
			Ext.write("h");
		else if (imod==2)
			Ext.write("i");
		else if (imod==3)
			Ext.write("j");
		else if (imod==4)
			Ext.write("k");

		Ext.write("V");
		wait(pause);
		while (!(readZValues(true))) {
			wait(pause);
		}

		while (curTime<nexTime)
			curTime=getTime();
		nexTime=curTime+interval;

		Ext.write("A");
		wait(durA);

		Ext.write("B");
		wait(durB);
	}
	Ext.write("g");  // Back to the first position

	Ext.write("u");	// Unlock the focus
	wait(pause);
	while (!(readZValues(true))) {
		wait(pause);
	}

	terminate();
}

