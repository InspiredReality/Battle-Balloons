enum signalStates {INERT, GO, RESOLVE};  //00, 01, 10
byte signalState = INERT;
enum gamePhases {SETUP, KINGSELECTED, PLAY, KINGKILLED}; //game phases, 00, 01, 10, 11
byte gamePhase = SETUP;  //the default game phase when the game begins
Color colors[] = { RED, BLUE, GREEN };
byte currentColorIndex = 0;
byte clickDim = 255;
bool isKing = false;
bool kingIsSelected = false;
bool kingIsKilled = false;
byte clicksToKill = 3;
byte clickCount = 0;
//timers
Timer balloonPoppedTimer;
#define POPPING_DURATION 3500
bool isPopping = false;
double iterateFace = 0;
double cumulativeFace = 0;
double displayFaceD = 0;
int displayFaceI = 0;
Timer gameOverTimer;
#define GAMEOVERSPIN_DURATION 20000
Timer fadeKilledKingTimer;


void setup() {
  // seed ramomizer
  randomize();
}

void loop() {
  if ( buttonPressed() ){
    clickDim = 155;
  }
  if ( buttonReleased() ){
    clickDim = 255;    
    if ( gamePhase == PLAY) {
      //increment clickCount 
      clickCount ++;
      
      if ( clicksToKill < clickCount){
      //run timer & balloon pop display
        balloonPoppedTimer.set(POPPING_DURATION);
        //if this balloon is king that popped then also seed that info
        if ( isKing ){
          kingIsKilled = true;
//          dimKilledKing = true;
        }
      }
    }
  }

  if( kingIsKilled && balloonPoppedTimer.isExpired() ){
    signalState = GO;  
    gamePhase = KINGKILLED; 
    kingIsKilled = false;
//    dimKilledKing = false;
    fadeKilledKingTimer.set(5000);
  }
  
  switch (signalState) {
    case INERT:
      inertLoop();
      break;
    case GO:
      goLoop();
      break;
    case RESOLVE:
      resolveLoop();
      break;
  }

  displayFaceColor();
  
  byte sendData = (signalState << 2) + (gamePhase);
  setValueSentOnAllFaces(sendData);
}

void inertLoop() {
  if ( buttonSingleClicked() ) {
    //only switch balloon color if in SETUP phase
    if ( gamePhase == SETUP){
      currentColorIndex = (currentColorIndex + 1) % 3;
    }
  }

  //set myself to GO when 2x clicked to tell others to switch to SETUP
  //reset King Selection & re-randomize
  if (buttonDoubleClicked()){
      isKing = false;
      kingIsSelected = false;
      kingIsKilled = false;
      signalState = GO;
      gamePhase = SETUP;
    }

  if ( buttonMultiClicked()){
  //set myself to GO when 3x clicked to tell others to switch to PLAY game phase
      if ( gamePhase == KINGSELECTED){
        //set game phase to PLAY
        gamePhase = PLAY;
        clickCount = 0;
        signalState = GO;
        randomizeClicksToKill();
      }
    }
    
  if ( buttonLongPressed()){
      if ( gamePhase == KINGSELECTED){
        isKing = true;
        signalState = GO;
      }
      if ( gamePhase == SETUP){
        isKing = true;
        kingIsSelected = true;
        gamePhase = KINGSELECTED;
        signalState = GO;
      }
   }
   
  //listen for neighbors in GO
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//we have a neighbor
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//a neighbor saying GO!
        signalState = GO;
        //what bitwise math do I need to make this correct?
        gamePhase = getGamePhase(getLastValueReceivedOnFace(f));

        if (gamePhase == SETUP){
            kingIsKilled = false;
        }
        if (gamePhase == PLAY){
            clickCount = 0;
            randomizeClicksToKill();
        }
        if (gamePhase == KINGSELECTED){
            isKing = false;
            kingIsSelected = true;
        }
        if (gamePhase == KINGKILLED){
          gameOverTimer.set(GAMEOVERSPIN_DURATION);
          
        }
      }
    }
  }
}

void goLoop() {
    signalState = RESOLVE;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not heard the GO news
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == INERT) {//This neighbor doesn't know it's GO time. Stay in GO
        signalState = GO;
      }
    }
  }
}

void resolveLoop() {
    signalState = INERT;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not moved to RESOLVE
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//This neighbor isn't in RESOLVE. Stay in RESOLVE
        signalState = RESOLVE;
      }
    }
  }
}

  
  // display color
void displayFaceColor() {
  //default OFF to stop color lingering
  setColor(OFF);
    
  switch (signalState) {
    case INERT:
      switch (gamePhase) {
        case SETUP:
          displayHiddenBalloonHealth();
          break;
        case PLAY:
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
            else {
              FOREACH_FACE(f) {
                if (f <= clicksToKill - clickCount) {
                  setColorOnFace(dim(colors[currentColorIndex],clickDim),f);
                }
              }
            }
          break;
        case KINGSELECTED:
          if ( isKing ){
            setColorOnFace(YELLOW, 0);
            setColorOnFace(ORANGE, 1);
            setColorOnFace(YELLOW, 2);
            setColorOnFace(ORANGE, 3);
            setColorOnFace(YELLOW, 4);
            setColorOnFace(ORANGE, 5);
          }
          else {
            displayHiddenBalloonHealth();
          }
          break;
        case KINGKILLED:
          if ( isKing ){
            if(!fadeKilledKingTimer.isExpired()) {
              setColorOnFace(YELLOW, 0);
              setColorOnFace(dim(WHITE,map(fadeKilledKingTimer.getRemaining(),0,5000,0,255)), 1);
              setColorOnFace(YELLOW, 2);
              setColorOnFace(dim(WHITE,map(fadeKilledKingTimer.getRemaining(),0,5000,0,255)), 3);
              setColorOnFace(YELLOW, 4);
              setColorOnFace(dim(WHITE,map(fadeKilledKingTimer.getRemaining(),0,5000,0,255)), 5);
              //fade from white to dark and keep just yellow
              }
            else {
              setColorOnFace(YELLOW, 0);
              setColorOnFace(YELLOW, 2);
              setColorOnFace(YELLOW, 4);          
            }
          }
          else {
            //non-King game over spin
            getDisplayFaceGradual(gameOverTimer, 20000);
            setColorOnFace(MAGENTA,displayFaceI);
          }
          break;
      }
      break;
    case GO:
      setColor(MAGENTA);
      break;
    case RESOLVE:
      setColor(WHITE);
      break;
  }
}

void randomizeClicksToKill(){
  //set or reset clicksToKill based on color
  switch (currentColorIndex) {
    case 0:
      clicksToKill = random( 3 ) + 2;
      break;
    case 1:
      clicksToKill = random( 2 ) + 1;
      break;
    case 2:
      clicksToKill = random( 1 );
      break;
  }
}

void displayHiddenBalloonHealth(){
    switch(currentColorIndex) {
    case 0:
      setColor(colors[0]);
      break;
    case 1:
      setColorOnFace(colors[1],0);
      setColorOnFace(colors[1],1);
      setColorOnFace(colors[1],2);
      setColorOnFace(colors[1],3);
      break;
    case 2:
      setColorOnFace(colors[2],4);
      setColorOnFace(colors[2],5);
      break;
  }
}

//timer/divisor closest to 1 is fastest, closer to 0 is slower
int getDisplayFaceGradual(Timer timer, double divisor) {
  iterateFace = ((double) timer.getRemaining() / (double) divisor);
  cumulativeFace = cumulativeFace + iterateFace;
  displayFaceI = (int) cumulativeFace;
  displayFaceI = displayFaceI % 6;
  return displayFaceI;
}

byte getGamePhase(byte data) {
    return (data & 3);//returns bits E and F
}

byte getSignalState(byte data) {
    return ((data >> 2) & 3);//returns bits C and D
}
