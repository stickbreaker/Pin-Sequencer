
/*
*  Pin Sequencer V0.1.0
*  Chuck Todd <ctodd@cableone.net>
*/

#define LED1 8
#define LED2 9
#define LED3 10
#define LED4 13
#define LED5 6
#define LEDCount 5 // count of LEDs, Arrays 

#define DEBUG  // uncomment this line to enable Debug display on Serial Monitor

/* each entry in this list must have Arduino Pin Number in first position,
*       at least one positive or negative value in the sequence, 
*       ending with a zero '0' to mark the end of the sequence
*       the next value is either '0' for stop, or n for step# to repeat from
* these PROGMEM arrays exist in Program memory, they do not take up any DATA memory
* they are a little tough to work with, but worth it. because of the RAM savings.
*/
const int16_t L1[] PROGMEM ={LED1,-40,2,-20,2,-36,0,1}; // negative means Low, count in seconds
const int16_t L2[] PROGMEM ={LED2,-20,2,-20,2,-56,0,1}; // pattern for each LED
const int16_t L3[] PROGMEM ={LED3,-10,2,-15,2,-71,0,1}; // zero is end, number after is where to repeat marker
const int16_t L4[] PROGMEM ={LED4,10,-10,5,-2,0,1};     // Zero, Zero is one shot   
const int16_t L5[] PROGMEM ={LED5,1000,-10,0,0};        // this led will be on for 1,000 second, off for 10, oneShot

const int16_t* const LTable[] PROGMEM ={L1,L2,L3,L4,L5}; // for each LED create a Ln[] and add it here

/* Each LED pin only consumes 4 bytes of ram, irrespective of how many sequence events is requires
*/
uint16_t L_index[LEDCount][2]; // two elements for each LED
                        // [0] is index for current element
                       // [1] is next trip point in seconds                       
                       
bool repeatedSequence(uint8_t seq){ // true if sequence is continously repeated, end maker 0,n
uint16_t base = pgm_read_word_near(&LTable[seq]); // base address for sequence table for this LED
uint16_t i=1;//i=0 is arduino pin
int16_t n=pgm_read_word_near(base+(i*2));
while(n!=0){
  i++;
  n = pgm_read_word_near(base+(i*2));
  }
i++;
n = pgm_read_word_near(base+(i*2));
return (n!=0);
}                      

void activateSequence(uint8_t seq){
#ifdef DEBUG
char ch[100];
#endif
L_index[seq][0] = 1;                              // first entry in sequence
uint16_t base = pgm_read_word_near(&LTable[seq]); // base address for sequence table for this LED
int16_t pin =pgm_read_word_near(base);          // Actual pin to change
pinMode(pin,OUTPUT);
int16_t n = pgm_read_word_near(base+2);         // time in seconds for the first state
                                                // added 2 because each element is 2 bytes 
L_index[seq][1]=abs(n);                           // duration in seconds for this state  
if(n<0) digitalWrite(pin,LOW);                  // negative for low
if(n>0) digitalWrite(pin,HIGH);                 // positive for high
#ifdef DEBUG
  sprintf(ch,"activate base=%04X LED%d =%d LI[%d][0]=%d LI[%d][1]=%d\n",base,seq,pin,seq,L_index[seq][0],seq,L_index[seq][1]);
  Serial.print(ch);
#endif
}
 
uint8_t initIndex(){ // setup the Arduio pins for output, High or Low as controlled by the Ln[] arrays.
uint8_t startedSequences=0;
for(uint8_t i = 0;i<LEDCount;i++){
  if(repeatedSequence(i)) {
    activateSequence(i);
    startedSequences++;
    }
  }
Serial.println("Exit InitIndex");
return startedSequences;
}

void deActivateSequence(uint8_t seq, int16_t state=0){// state sets digital write value 0 means leave alone
L_index[seq][0]=0;
L_index[seq][1]=0;
uint16_t base = pgm_read_word_near(&LTable[seq]);
int16_t pin = pgm_read_word_near(base); // pin number
pinMode(pin,OUTPUT);
if(state>0) digitalWrite(pin,HIGH);
if(state<0) digitalWrite(pin,LOW);
}

uint16_t findNext(){ // returns next expiration second
uint8_t i=0;
uint16_t ns=65535;  // pick max possible, nothing scheduled 
while(i<LEDCount){
  if((ns>L_index[i][1])&&(L_index[i][1]!=0)) ns = L_index[i][1]; // is this one shorter?
  i++;
  }
#ifdef DEBUG
char ch[50];
sprintf(ch," next second =%d\n",ns);
Serial.print(ch);
#endif
return ns;
}

#ifdef DEBUG
void printTables(){
char ch[50];
for(uint8_t i = 0;i<LEDCount; i++){
  uint16_t base = pgm_read_word_near(&LTable[i]);
  sprintf(ch,"base%d=0x%04X ",i,base);
  Serial.print(ch);
  int16_t j;
  uint8_t k=0;
  do{
    j=pgm_read_word_near(base+k*2);
    Serial.print(j,DEC);Serial.print(' ');
    k++;
  }while(j!=0);
  j=pgm_read_word_near(base+k*2);
  if(j==0)Serial.println("OneShot");
  else {
    sprintf(ch,"Repeated from %d",j);
    Serial.println(ch);
    }
  }
}
#endif

void lightEmUp(uint16_t tick){
#ifdef DEBUG
char ch[50];
#endif
for(uint8_t i = 0;i<LEDCount;i++){
  uint16_t base =pgm_read_word_near(&LTable[i]);
  while((tick>=L_index[i][1])&&(L_index[i][0]!=0)){
    L_index[i][0] += 1;                                      // next sequence value
    int16_t n = pgm_read_word_near(base+(L_index[i][0]*2));  // time,polarity or stop
                                                             // multipled by 2 because each element is 2 bytes. 
#ifdef DEBUG
    sprintf(ch,"\nbase=%04X LI[%d][0]=%d n=%d",base,i,L_index[i][0],n);
    Serial.print(ch);
#endif
    if(n==0){ // sequence end marker
      L_index[i][0] = pgm_read_word_near(base+((L_index[i][0]+1)*2)); // repeat or oneshot marker
      if(L_index[i][0]!=0) // Repeated from set#  
        n = pgm_read_word_near(base+(L_index[i][0]*2));
      else { // oneshot, 
        L_index[i][1] = 0; // expire sequence
        n=0;
        }
      }
    L_index[i][1] += abs(n);                    // tick value this new status is value for
    int16_t pin = pgm_read_word_near(base);     // actual Arduino pin to change
#ifdef DEBUG
    sprintf(ch," LI[%d][1]=%d n=%d pin=%d ",i,L_index[i][1],n,pin);
    Serial.print(ch);
#endif
    if(n<0){
      digitalWrite(pin,LOW);
#ifdef DEBUG
      Serial.print("L ");
#endif
      }
    if(n>0){
      digitalWrite(pin,HIGH);
#ifdef DEBUG
      Serial.print("H ");
#endif
      }
    }
  if(tick<L_index[i][1])
    L_index[i][1] -= tick;
  }
}

static uint16_t nextSec;           // how many seconds until the next change event
static unsigned long startTime=0;  // millisecond counter, continually adjusted after each event.

void setup(){
Serial.begin(19200);  //debug console
#ifdef DEBUG
printTables();    // just to verify I can access the PROGMEM correctly
#endif
startTime=millis();
uint8_t i=initIndex();
Serial.print(i,DEC);            // Set up the pins, Set their initial state, duration
Serial.print(" Sequences Started.\n");
nextSec = findNext();   // when the next change happens
}

void loop(){
unsigned long t=millis();
if(nextSec<65535){ // there is an active sequence
  if((t-startTime)>=((unsigned long)nextSec*1000L)){ // is it time yet?
    uint16_t tick = (t-startTime)/1000;  //number of seconds since last event.
  #ifdef DEBUG
    Serial.print("\n+");Serial.print(tick,DEC);Serial.print(' ');
  #endif
    startTime += (unsigned long)tick*1000; // adjust startime to last event 
    lightEmUp(tick);                       // actuall change the pins, net the next event countdown timers
    nextSec=findNext();                    // when is the next event scheduled?
    }
  }
else startTime = t; // no active sequences, prepare for sequence to be activated
}

