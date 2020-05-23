enum signalStates {INERT, GO, RESOLVE};  //00, 01, 10
byte signalState = INERT;
enum gamePhases {SETUP, KINGSELECTED, PLAY, KINGKILLED}; //game phases, 00, 01, 10, 11
byte gamePhase = SETUP;  //the default game phase when the game begins
Color colors[] = { RED, BLUE, GREEN };
byte currentColorIndex = 0;
byte clickDim = 255;
bool isKing = false;
bool kingIsSelected = false;
byte clicksToKill = 3;
byte clickCount = 0;
//byte redClicksToKill;
//byte blueClicksToKill;
//byte greenClicksToKill;

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void loop() {
  if ( buttonPressed() ){
    clickDim = 155;
  }
  if ( buttonReleased() ){
    clickDim = 255;    
  }

//this constantly fires since it's in main loop
  if ( gamePhase == PLAY && isKing && clicksToKill < clickCount){
    signalState = GO;  
    gamePhase = KINGKILLED;
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
    if ( gamePhase == PLAY) {
      //increment clickCount 
      clickCount ++;
    }
  }

  if ( buttonDoubleClicked()){
      if ( gamePhase == PLAY) {
        //increment clickCount 
        clickCount ++;
      }
  //set myself to GO when 2x clicked to tell others to switch to PLAY game phase
      if ( gamePhase == KINGSELECTED){
        //set game phase to PLAY
        gamePhase = PLAY;
        clickCount = 0;
        signalState = GO;        
      }
    }
    
  //set myself to GO when 3x clicked to tell others to switch to SETUP
  //reset King Selection & re-randomize
  if (buttonMultiClicked()){
      isKing = false;
      kingIsSelected = false;
      signalState = GO;
      gamePhase = SETUP;
      randomizeClicksToKill(); 
    }

  if ( buttonLongPressed()){
      if ( gamePhase == PLAY) {
        //increment clickCount 
        clickCount ++;
      }
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
          randomizeClicksToKill();
        }
        if (gamePhase == PLAY){
            clickCount = 0;
        }
        if (gamePhase == KINGSELECTED){
            isKing = false;
            kingIsSelected = true;
        }
//        if (gamePhase = KINGKILLED){
//          start rainbow display
//        }
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
//    setColor(dim(colors[currentColorIndex],clickDim));  
  //default OFF to stop color lingering
  setColor(OFF);
    
  switch (signalState) {
  case INERT:
    switch (gamePhase) {
      case SETUP:
        FOREACH_FACE(f) {
          if (f <= clicksToKill) {
            setColorOnFace(dim(colors[currentColorIndex],clickDim),f);
          }
        }
        break;
      case PLAY:
        FOREACH_FACE(f) {
          if (f <= clicksToKill - clickCount) {
            setColorOnFace(dim(colors[currentColorIndex],clickDim),f);
          }
        }
        break;
      case KINGSELECTED:
        if ( isKing ){
          setColor(YELLOW);
        }
        else {
          FOREACH_FACE(f) {
            if (f <= clicksToKill) {
              setColorOnFace(dim(colors[currentColorIndex],clickDim),f);
            }
          }
        }
        break;
      case KINGKILLED:
        if ( isKing ){
          setColor(WHITE);
        }
        else {
          setColor(MAGENTA);
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

byte getGamePhase(byte data) {
    return (data & 3);//returns bits E and F
}

byte getSignalState(byte data) {
    return ((data >> 2) & 3);//returns bits C and D
}
