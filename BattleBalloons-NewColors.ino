//COMMS
enum gamePhases {SETUP, START, PLAY}; // 00, 01, 10 [A][B]
byte gamePhase = SETUP;  //default game phase when the game begins
enum fortifySignals {WAITING, SEND, HEARD}; // 00, 01, 10 [C][D]
byte fortifySignal[6] = {WAITING, WAITING, WAITING, WAITING, WAITING, WAITING};
enum popStates {INERT, CROWN, BUST, RESOLVE}; //game phases, 00, 01, 10, 11 [E][F]
byte popState = INERT;
enum lastPopStates {Crown, Bust, None};
byte lastPopState = None;  //Crown, Bust, Neither

//Aesthetics
//First New Color Combo
#define REDY makeColorRGB(250,10,75)
#define BLUEY makeColorRGB(0, 139, 248)
#define GREENY makeColorRGB(25,250,0) 
#define YELLOWY makeColorRGB(247, 215, 5)
#define PURPLEY makeColorRGB(83, 28, 179)
Color colors[] = { REDY, BLUEY, GREENY };
Color specialDisplay;
Color popSpecialDisplay;
byte currentColorIndex = 0;
byte clickDim = 255;

//Game logic
bool isCrown = false;
bool isBust = false;
bool popped = false;
bool speacialFaded = false;
byte clicksToKill = 3; //life bar = clicksToKill+1 so need to - 1 to compensate
double iterateFace = 0;
int simpleIterateFace = 0;
double cumulativeFace = 0;
int displayFaceI = 0;

//Timing
Timer balloonPoppedTimer;
#define POPPING_DURATION 3000
Timer specialPoppedNotificationTimer;
#define SPECIALPOPPEDNOTI_DURATION 2332
Timer fadePoppedSpecialTimer;
Timer simpleIterateFaceTimer;


void setup() {
  // seed ramomizer
  randomize();
}

void loop() {
  if ( buttonPressed() ){
    clickDim = 155;
  }

  switch (gamePhase) {
    case SETUP:
      setupLoop();
      break;
    case START:
      startLoop();
      break;
    case PLAY:
      playLoop();
      break;
  }

  switch (popState) {
    case INERT:
      inertPopLoop();
      break;
    case CROWN:
      crownPopLoop();
      break;
    case BUST:
      bustPopLoop();
      break;
    case RESOLVE:
      resolvePopLoop();
      break;
  }
  
  FOREACH_FACE(f) {
    switch (fortifySignal[f]) {
      case WAITING:
        waitingLoop();
        break;
      case SEND:
        sendLoop();
        break;
      case HEARD:
        heardLoop();
        break;
    }
  }

          //reset King Selection & switch to SETUP
  if (buttonDoubleClicked()){
    reset();
  }
  
  displayFaceColor();

  FOREACH_FACE(f) {
    byte sendData = (gamePhase << 4) + (fortifySignal[f] << 2) + (popState);
    setValueSentOnFace(sendData, f);
  }
}


//gamePhases Logic -------
void setupLoop() {
  if ( buttonReleased() ){
    clickDim = 255;
  }
  
  if ( buttonSingleClicked() ) {
    //only switch balloon color if in SETUP phase
    if ( isCrown ){
      isCrown = !isCrown;
      isBust = true;
    }
    else if (isBust){
      isBust = !isBust;
      isCrown = true;
    }
    else {
      currentColorIndex = (currentColorIndex + 1) % 3;
    }
  }
        
  if ( buttonLongPressed() ){
    if ( isCrown || isBust ){
      isCrown = false;
      isBust = false;
    }
    else {
      isCrown = true;
    }
  }

  if ( buttonMultiClicked() && buttonClickCount() == 3){
    //3x clicked so tell others to switch to PLAY game phase
        //set game phase to START that will go to PLAY
        gamePhase = START;
  }

  //listen for neighbors but not listening for SETUP, only START or PLAY
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//we have a neighbor
      if (getGamePhase(getLastValueReceivedOnFace(f)) == START) {
        gamePhase = START;
      }
    }
  }
}

void startLoop() {
  gamePhase = PLAY;
//  randomizeClicksToKill();
  switch (currentColorIndex) {
  case 0:
    clicksToKill = random( 3 ) + 3;
    break;
  case 1:
    clicksToKill = random( 2 ) + 2;
    break;
  case 2:
    clicksToKill = random( 1 ) + 1;
    break;
  }
    FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getGamePhase(getLastValueReceivedOnFace(f)) == SETUP) {
        gamePhase = START;
      }
    }
  }
}

void playLoop() {
  if ( buttonReleased() ){
    clickDim = 255;
      if ( clicksToKill > 0){
      //reduce clicksToKill
        clicksToKill --;        
      }
      if (!popped && clicksToKill == 0 ){
      //run timer & balloon pop display
        balloonPoppedTimer.set(POPPING_DURATION);
        popped = true;
       }
  }
  if ( popped && balloonPoppedTimer.isExpired() && !speacialFaded ){
    if ( isCrown ){
      popState = CROWN;
    }
    else if ( isBust ){
      popState = BUST;
    }
      fadePoppedSpecialTimer.set(6000);
      speacialFaded = true;
  }

  //want to clear 1xclick to not iterate Color when entering SETUP
  buttonSingleClicked();
  buttonLongPressed();

  //FORTIFY
  if (isAlone() && clicksToKill > 0 ){  //we don't have a neighbor so we're in FORTIFY mode
    fortifySignal[0] = SEND;
  }
    
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getGamePhase(getLastValueReceivedOnFace(f)) == SETUP) {
        reset();
      }
    }
  }
}


//fortifySignals Logic -------
void waitingLoop() {
 if( clicksToKill < 6 && clicksToKill > 0 ){
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
        if(didValueOnFaceChange(f)){  //a neighbor giving update
          if (getFortifySignal(getLastValueReceivedOnFace(f)) == SEND) {
            fortifySignal[f] = HEARD;
            clicksToKill ++;
          }
        }
      }
    }
  }
}

void sendLoop() {
  if (!isValueReceivedOnFaceExpired(0)) {//a neighbor!
    if (getFortifySignal(getLastValueReceivedOnFace(0)) == HEARD) {
      clicksToKill --;
      fortifySignal[0] = WAITING;
      //check if ballon just gave away last life and therefore popped
      if (clicksToKill == 0 ){
      //run timer & balloon pop display
        balloonPoppedTimer.set(POPPING_DURATION);
        popped = true;
       }
    }
  }
}

void heardLoop() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getFortifySignal(getLastValueReceivedOnFace(f)) == WAITING) {
        fortifySignal[f] = WAITING;
      }
    }
  }
}

//popStates Logic -------
void inertPopLoop() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getPopState(getLastValueReceivedOnFace(f)) == CROWN) {
        popState = CROWN;
      }
      else if (getPopState(getLastValueReceivedOnFace(f)) == BUST) {
        popState = BUST;
      }        
    }
  }
}

void crownPopLoop() {
  //set timer for gold swirl animation over the remaining life or do another thing?
  if ( isCrown && !fadePoppedSpecialTimer.isExpired() ){
    specialPoppedNotificationTimer.set(0);
  }
  else {
    specialPoppedNotificationTimer.set(SPECIALPOPPEDNOTI_DURATION);
  }
  
  lastPopState = Crown;
  popSpecialDisplay = YELLOWY;
  
  popState = RESOLVE;
  //look for neighbors who have not moved to CROWN
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getPopState(getLastValueReceivedOnFace(f)) == INERT) {
        popState = CROWN;
      }
    }
  }
}

void bustPopLoop() {
  //set timer for purple swirl animation over the remaining life
  //create BustNoti and CrownNoti instead of using same timer to save space in displayLoop? Only needed if duration is diff.
  if ( isBust && !fadePoppedSpecialTimer.isExpired() ){
    specialPoppedNotificationTimer.set(0);
  }
  else {
    specialPoppedNotificationTimer.set(SPECIALPOPPEDNOTI_DURATION);
  }

  lastPopState = Bust;
  popSpecialDisplay = PURPLEY;
          
  popState = RESOLVE;
  //look for neighbors who have not moved to BUST
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getPopState(getLastValueReceivedOnFace(f)) == INERT) {
        popState = BUST;
      }
    }
  }
}

void resolvePopLoop() {
  popState = INERT;

    FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getPopState(getLastValueReceivedOnFace(f)) == BUST || getPopState(getLastValueReceivedOnFace(f)) == CROWN) {
        popState = RESOLVE;
      }
    }
  }
}

//DISPLAY FACE Logic -------
void displayFaceColor() {
  //default OFF to stop color lingering
  setColor(OFF);

  if ( isCrown ){
    specialDisplay = YELLOWY;
  }
  if ( isBust ){
    specialDisplay = PURPLEY;
  }
      
  switch (gamePhase) {
    case SETUP:
      if ( isCrown || isBust ){
        setColorOnFace(specialDisplay, 0);
        setColorOnFace(WHITE, 1);
        setColorOnFace(specialDisplay, 2);
        setColorOnFace(WHITE, 3);
        setColorOnFace(specialDisplay, 4);
        setColorOnFace(WHITE, 5);
      }
      else {
        displayHiddenBalloonHealth();
      }
      break;
    case PLAY:
    //popping
      if(!balloonPoppedTimer.isExpired()) {
        getDisplayFaceGradual(balloonPoppedTimer, 15000);
        //run color wheel pattern
        switch(currentColorIndex) {
          case 0:
            setColorOnFace(ORANGE,displayFaceI);
            setColorOnFace(colors[0],(displayFaceI+1) % 6);
            setColorOnFace(colors[0],(displayFaceI+2) % 6);
            setColorOnFace(colors[0],(displayFaceI+3) % 6);
            break;
          case 1:
            setColorOnFace(ORANGE,displayFaceI);
            setColorOnFace(colors[1],(displayFaceI+1) % 6);
            setColorOnFace(colors[1],(displayFaceI+2) % 6);
            break;
          case 2:
            setColorOnFace(ORANGE,displayFaceI);
            setColorOnFace(colors[2],(displayFaceI+1) % 6);
            break;
        }
      }
      //popped
      else if ( popped ) {
        if (isCrown || isBust ) {
          if(!fadePoppedSpecialTimer.isExpired()) {
            setColorOnFace(specialDisplay, 0);
            setColorOnFace(dim(WHITE,map(fadePoppedSpecialTimer.getRemaining(),0,6000,0,255)), 1);
            setColorOnFace(specialDisplay, 2);
            setColorOnFace(dim(WHITE,map(fadePoppedSpecialTimer.getRemaining(),0,6000,0,255)), 3);
            setColorOnFace(specialDisplay, 4);
            setColorOnFace(dim(WHITE,map(fadePoppedSpecialTimer.getRemaining(),0,6000,0,255)), 5);
            //fade from white to dark and keep just yellow
          }
          else {
            setColorOnFace(specialDisplay, 0);
            setColorOnFace(specialDisplay, 2);
            setColorOnFace(specialDisplay, 4);          
          }       
        }
        else {  //keeping very dim color instead of all blank
          setColor(dim(colors[currentColorIndex],70));
        }
      }
      //balloon not popped or popping
      else {
        FOREACH_FACE(f) {
          if (f < clicksToKill ) {
            setColorOnFace(dim(colors[currentColorIndex],clickDim),f);
          }
        }
      }

      //always running during PLAY
      if( !specialPoppedNotificationTimer.isExpired() ) {
        if( simpleIterateFaceTimer.isExpired() ){
          simpleIterateFaceTimer.set(333);
          simpleIterateFace ++;
        }
        setColorOnFace(dim(popSpecialDisplay,255),(3 + simpleIterateFace) % 6);
        setColorOnFace(dim(WHITE,255),(4 + simpleIterateFace) % 6);
        setColorOnFace(dim(popSpecialDisplay,255),(5 + simpleIterateFace) % 6);
      }
      break;
  }

  if ( fortifySignal[0] == SEND ){
    setColorOnFace(WHITE,0);
  }
}

//FUNCTIONS
byte reset(){
  gamePhase = SETUP;
  fortifySignal[0] = {WAITING};
  fortifySignal[1] = {WAITING};
  fortifySignal[2] = {WAITING}; 
  fortifySignal[3] = {WAITING};  
  fortifySignal[4] = {WAITING};    
  fortifySignal[5] = {WAITING};    
  popState = INERT;
  lastPopState = None;
  fortifySignal[0] = false;
  isCrown = false;
  isBust = false;
  popped = false;
  speacialFaded = false;
}

//timer/divisor closest to 1 is fastest, closer to 0 is slower
//starts spinning fast and gradually slows until stopped
int getDisplayFaceGradual(Timer timer, double divisor) {
  iterateFace = ((double) timer.getRemaining() / (double) divisor);
  cumulativeFace = cumulativeFace + iterateFace;
  displayFaceI = (int) cumulativeFace;
  displayFaceI = displayFaceI % 6;
  return displayFaceI;
}

void displayHiddenBalloonHealth(){
    switch(currentColorIndex) {
    case 0:
      setColor(dim(colors[0],clickDim));
      break;
    case 1:
      setColorOnFace(dim(colors[1],clickDim),0);
      setColorOnFace(dim(colors[1],clickDim),1);
      setColorOnFace(dim(colors[1],clickDim),2);
      setColorOnFace(dim(colors[1],clickDim),3);
      break;
    case 2:
      setColorOnFace(dim(colors[2],clickDim),4);
      setColorOnFace(dim(colors[2],clickDim),5);
      break;
  }
}

//COMMS
byte getGamePhase(byte data) {
    return ((data >> 4) & 3);//returns bits [A] [B]
}
byte getFortifySignal(byte data) {
    return ((data >> 2) & 3);//returns bits [C] [D]
}
byte getPopState(byte data) {
    return (data & 3);//returns bits [E] [F]
}
