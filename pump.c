#include<reg52.h>
sbit led=P2^4;
sbit uppump=P1^0;
sbit downlevstop=P1^1;
sbit uplevstop=P1^2;
sbit downlevstart=P1^3;
sbit uplevstart=P1^4;
sbit downlevlow=P1^5;
sbit downpump=P1^6;
sbit water=P1^7;
#define update_led()(led = ~(water & downlevstart))
bit var=0,var2=0;
unsigned char var3=0;
unsigned int t0,t1,t2;

// check downpump(front) for water.
// try to make uplevstart and downlevstart as interrupts.
//upstrtrchd and upstrchd are variables that are set to 1 if level reaches uplevstart and uplevstop respectively, even once.
//var is set to 1 when backdelay() is running and cleared when finished or never run.

void secdelay(){									//rectify for 1 sec.
	unsigned int i;
	unsigned char j;
	for(j=0;j<31;j++)
	for(i=0;i<12500;i++){}
}

void shdelay(){									//rectify for 1 sec.
	unsigned int i;
	unsigned char j;
	for(j=0;j<10;j++)
	for(i=0;i<12500;i++){}
}


void timer0 (void) interrupt 1 {
	TF0=0;
	if(t0!=0)
		t0--;
	else {
		var=0;
		TR0=0;
	}
}

void timer1 (void) interrupt 3 {
	TF1=0;
	if(t1!=0)
		t1--;
	else {
		var2=0;
		TR1=0;
	}
}

void timer2 (void) interrupt 5 {
	TF2=0;
	if(t2!=0)
		t2--;
	else {
		var3=2;
		TR2=0;
	}
}

void backdelay(unsigned char del){
	if(var)
		return;
	
	t0=1860*del;		//actual number = 1860.
	var=1;
	TH0=0;
	TL0=0;
	TR0=1;
}
	
void backdelay2(unsigned char del){
	if(var2)
		return;
	
	t1=1860*del;		//actual number = 1860.
	var2=1;
	TH1=0;
	TL1=0;
	TR1=1;
}

void backdelay3(unsigned char del){
	if(var3)
		return;
	
	t2=1860*del;			//actual number = 1860.
	var3=1;
	TH2=0;
	TL2=0;
	TR2=1;
}


void contrl_init(){
	int i;
	for(i=0;i<20;i++){
		led=~led;
		shdelay();
	}
}

void init(){
	uppump = 1;
	downpump=1;
	P1=0xff;
	TMOD = 17;
	EA=1;
	ET0=ET1=ET2=1;
}

void bothstart(){	//note: uplevstart IS needed in this function for when water fills up once and then decreases in uptank.
	unsigned char i,j,x=0,y=6;
	bit ust,dst;
	dst = downlevstart;
	ust = uplevstart;
	while(ust || dst){
		update_led();  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//conditions for starting downpump ----------------------------------------------
		if(!var2 && dst)
		downpump = 0;
		//conditions for starting downpump till here ----------------------------------------------
		while(water){
			TR2 = 0;
			var3 = 0;
			update_led();  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			//conditions for starting downpump ----------------------------------------------------------------
			if(downlevstart){
				dst = 1;
			}
			if(x>=y && !var2 && dst){		//var2 is for 20 min delay for downpump.
				downpump = 0;
				x=0;
			}
			//conditions for starting downpump till here ----------------------------------------------------------------
			for(i=0;i<27;i++){
				if(!water) break;
				for(j=0;j<10;j++){
					update_led();  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
					//conditions for starting uppump -----------------------------------------------------
					if(!downlevlow && !var && ust) {
						uppump = 0;
						y = 6;
					}					
					//condtitions for starting till here --------------------------------------------------
					
					//conditions for stopping uppump -------------------------------------------------------
					if(!uplevstop){
						uppump = 1;
						ust = 0;
						backdelay(20);		//in case of controller error atleast 20 min will be in between switching								
					}
					if(uplevstart){
						ust = 1;
						
					}
					if(downlevlow){
						uppump = 1;
						backdelay(20);
						y=4;
					}
					//conditions for stopping uppump till here -------------------------------------------------------
					
					if(!water) break;
					secdelay();
				}
			}
			//conditions for stopping downpump ----------------------------------------------------------------
			if(water){
			downpump=1;
			x++;
			}
			//conditions for stopping downpump till here ----------------------------------------------------------------	
		}
		
		while(!water && (ust || dst)){
			x = 0;
			//conditions for uppump will be the same...
			update_led();  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
					//conditions for starting uppump -----------------------------------------------------
					if(!downlevlow && !var && ust){	//ust used as water should be filled to the top atleast once, and var will ensure 20 min delay.
						uppump = 0; 				
						y = 6;
					}
					//condtitions for starting uppump till here --------------------------------------------------
					
					//conditions for stopping uppump -------------------------------------------------------
					if(!uplevstop){
						uppump = 1;
						ust = 0;
						backdelay(20); //in case of controller error atleast 20 min will be in between switching
					}
					if(uplevstart){
						ust = 1;
						
					}
					if(downlevlow){
						uppump = 1;
						backdelay(20);
						y=4;
					}
					//conditions for stopping uppump till here -------------------------------------------------------
			
					//conditions for stopping downpump ----------------------------------------------------------------
					if(!downlevstart)
						backdelay3(20);	//backdelay3 is for when the level crosses the incoming water pipe level. It uses var3(unsigned char not bit).
					if(!downlevstop || var3==2){
						TR2 = 0;					//stop backdelay3.
						downpump = 1;
						dst = 0;
						backdelay2(20);			//backdelay2() is for downpump.
						var3=0; //var3 = 1(already running), 0(can run again or never ran and can run), 2(ran once and finished, need to be set to 0).
					}
					if(downlevstart){
						dst = 1;
						
					}
					//conditions for stopping downpump till here ----------------------------------------------------------------
					
		}
	}
}



void main(){
	//write interrupt vectors for external interrupts.
	init();
	contrl_init();
	while(1){
		update_led();
		if(uplevstart || downlevstart)
			bothstart();
		uppump = downpump = 1;
	}
}








