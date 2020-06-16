//COMMS
enum signalStates {INERT, GO, RESOLVE};  //00, 01, 10
byte signalState = INERT;
enum gamePhases {SETUP, CROWNSELECTED, PLAY, CROWNKILLED}; //game phases, 00, 01, 10, 11
byte gamePhase = SETUP;  //the default game phase when the game begins
enum fortifyRoles {WAITING, FORTIFY};  //00, 01
byte fortifyRole = WAITING;
byte canFortify[6]; //Waiting
//byte isFortifying[6] = {false, false, false, false, false, false};  //Fortify
bool isFortifying;
//Aesthetics
Color colors[] = { RED, BLUE, GREEN };
byte currentColorIndex = 0;
byte clickDim = 255;
#define BUSTCOLOR makeColorRGB(162, 0, 255)
#define CROWNCOLOR makeColorRGB(255, 208, 0)
//mostly white redish purp
#define GAMEOVERSPIN makeColorRGB(255, 194, 220)
Color nonCrownBustSpinColor;
//Game logic
bool isCrown = false;
bool isBust = false;
bool endedGame = false;
bool crownIsSelected = false;
bool gameOver = false;
byte clicksToKill = 3; //life bar = clicksToKill+1 so need to - 1 to compensate
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

Timer fadeKilledCrownTimer;
bool fortifying = false;
bool balloonsOverBubbles;

void setup() {
  // seed ramomizer
  randomize();
  nonCrownBustSpinColor = GAMEOVERSPIN;
}

void loop() {
  if ( buttonPressed() ){
    clickDim = 155;
  }
  if ( buttonReleased() ){
    clickDim = 255;    
    if ( gamePhase == PLAY) {
      if ( clicksToKill > 0){
      //reduce clicksToKill
        clicksToKill --;        
      }
      if ( clicksToKill == 0) {
      //run timer & balloon pop display
        balloonPoppedTimer.set(POPPING_DURATION);
        //if this balloon is Crown that popped then also seed that info
        if ( isCrown ){
          gameOver = true;
          endedGame = true;
//          nonCrownBustSpinColor = CROWNCOLOR;
        }
        else if ( isBust ){
          gameOver = true;
          endedGame = true;
//          nonCrownBustSpinColor = BUSTCOLOR;
        }
      }
    }
  }

  if( gameOver && balloonPoppedTimer.isExpired() ){
    signalState = GO;  
    gamePhase = CROWNKILLED; 
    gameOver = false;
//    dimKilledKing = false;
    fadeKilledCrownTimer.set(5000);
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
  
//  byte sendData = (signalState << 2) + (gamePhase);
//  setValueSentOnAllFaces(sendData);
  byte sendData = (fortifyRole << 5) + (isFortifying << 4) + (signalState << 2) + (gamePhase);
  setValueSentOnAllFaces(sendData);
}

void inertLoop() {
  if ( buttonSingleClicked() ) {
    //only switch balloon color if in SETUP phase
    if ( gamePhase == SETUP ) {
        currentColorIndex = (currentColorIndex + 1) % 3;
    }
    if ( gamePhase == CROWNSELECTED ){
      toggleCrownOrBust();
  //    signalState = GO;
    }
  }

  //set myself to GO when 2x clicked to tell others to switch to SETUP
  //reset King Selection & re-randomize
  if (buttonDoubleClicked()){
    resetSetup();
    signalState = GO;
    gamePhase = SETUP;
  }

  if ( buttonMultiClicked()){
    if ( buttonClickCount() == 3){
  //set myself to GO when 3x clicked to tell others to switch to PLAY game phase
      if ( gamePhase == CROWNSELECTED ){
        //set game phase to PLAY
        gamePhase = PLAY;
        signalState = GO;
        randomizeClicksToKill();
        fortifyRole = WAITING;
      }
    }
  }
        
  if ( buttonLongPressed()){
    //requests to remove this limitation but then we need to know to 2xclick to clear Kings selected and restart
      if ( gamePhase == SETUP ){
        isCrown = true;
        crownIsSelected = true;
        gamePhase = CROWNSELECTED;
        signalState = GO;
      }
      if ( gamePhase == CROWNSELECTED ){
        isCrown = true;
      }
   }

   //Fortify in PLAY only+++++++++
  if (gamePhase == PLAY){
  
    if (isAlone() && clicksToKill > 0 ){  //we don't have a neighbor so we're in FORTIFY mode
      fortifyRole = FORTIFY;
    }
  
    switch (fortifyRole) {
      case WAITING:
        waitLoop();
        break;
      case FORTIFY:
        fortifyLoop();
        break;
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
            resetSetup();
        }
        if (gamePhase == CROWNSELECTED){
          //commenting this out to allow multiple Crowns
//            isCrown = false;
            crownIsSelected = true;
        }        
        if (gamePhase == PLAY){
            randomizeClicksToKill();
            fortifyRole = WAITING;
        }
        if (gamePhase == CROWNKILLED){
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

void waitLoop() {
    //look for balloons that are fortifying
    FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if(didValueOnFaceChange(f)){  //a neighbor giving update
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getFortifyRoles(neighborData) == FORTIFY) { //this neighbor is fortifying
          canFortify[f] = false;
          isFortifying = false;
          if(clicksToKill < 6 ){
            clicksToKill ++;
          }
        }
      }
        else {//this is a face we COULD offer ore to
            canFortify[f] = true;
            isFortifying = true;
      }
    }
    else {//no neighbor
      canFortify[f] = false;
      isFortifying = false;
    }
    if (fortifyRole = WAITING && canFortify[f] == false){
      canFortify[f] = true;
      isFortifying = true;
    }
  }
  
  //set up communication
  FOREACH_FACE(f) {
    byte sendData = (fortifyRole << 5) + (isFortifying << 4);
    setValueSentOnFace(sendData, f);
  }
}

void fortifyLoop(){
  //default is try to fortify on face0
  isFortifying = false;

      if (!isValueReceivedOnFaceExpired(0)) {//a neighbor!
        byte neighborData = getLastValueReceivedOnFace(0); //updating
        if (getFortifyRoles(neighborData) == WAITING) {//back to cluster
          if (isFortifying) { //I'm already mining here
            if (getFortifyStatus(neighborData) == 0) {//he's done, so I'm done
//              isFortifying[0] = false;
              isFortifying = false;
            }
          } else 
          {//I could be mining here if they'd let me
            if (getFortifyStatus(neighborData) == 1) {//ore is available, take it
              //why does this step get skipped? F doesnt revert to W (white face0 still) so missing 1 coming from W.
              clicksToKill --;
              fortifyRole = WAITING;
//              isFortifying[0] = true;
              isFortifying = true;
            }
          }
        }
      }
      else {//no neighbor
//        isFortifying[0] = false;
        isFortifying = false;
      }

    //set up communication
//    byte sendData = (fortifyRole << 5) + (isFortifying[0] << 4);
    byte sendData = (fortifyRole << 5) + (isFortifying << 4);
    setValueSentOnFace(sendData, 0);
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
        case CROWNSELECTED:
          if ( isCrown ){
            setColorOnFace(YELLOW, 0);
            setColorOnFace(ORANGE, 1);
            setColorOnFace(YELLOW, 2);
            setColorOnFace(ORANGE, 3);
            setColorOnFace(YELLOW, 4);
            setColorOnFace(ORANGE, 5);
          }
          else if ( isBust ){
            setColorOnFace(YELLOW, 0);
            setColorOnFace(MAGENTA, 1);
            setColorOnFace(YELLOW, 2);
            setColorOnFace(MAGENTA, 3);
            setColorOnFace(YELLOW, 4);
            setColorOnFace(MAGENTA, 5);
          }          
          else {
            displayHiddenBalloonHealth();
          }
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
              if (f < clicksToKill ) {
                setColorOnFace(dim(colors[currentColorIndex],clickDim),f);
              }
            }
            //keeping very dim color instead of all blank
            if ( clicksToKill == 0){
              setColor(dim(colors[currentColorIndex],70));
            }
            if (fortifyRole == FORTIFY){
              setColorOnFace(WHITE, 0);
            }
          }
          break;
        case CROWNKILLED:
          if ( endedGame ){
            if ( isCrown ){
              if(!fadeKilledCrownTimer.isExpired()) {
                setColorOnFace(CROWNCOLOR, 0);
                setColorOnFace(dim(WHITE,map(fadeKilledCrownTimer.getRemaining(),0,5000,0,255)), 1);
                setColorOnFace(CROWNCOLOR, 2);
                setColorOnFace(dim(WHITE,map(fadeKilledCrownTimer.getRemaining(),0,5000,0,255)), 3);
                setColorOnFace(CROWNCOLOR, 4);
                setColorOnFace(dim(WHITE,map(fadeKilledCrownTimer.getRemaining(),0,5000,0,255)), 5);
                //fade from white to dark and keep just yellow
                }
              else {
                setColorOnFace(CROWNCOLOR, 0);
                setColorOnFace(CROWNCOLOR, 2);
                setColorOnFace(CROWNCOLOR, 4);          
              }
            }
            else if ( isBust ){
              if(!fadeKilledCrownTimer.isExpired()) {
                setColorOnFace(BUSTCOLOR, 0);
                setColorOnFace(dim(WHITE,map(fadeKilledCrownTimer.getRemaining(),0,5000,0,255)), 1);
                setColorOnFace(BUSTCOLOR, 2);
                setColorOnFace(dim(WHITE,map(fadeKilledCrownTimer.getRemaining(),0,5000,0,255)), 3);
                setColorOnFace(BUSTCOLOR, 4);
                setColorOnFace(dim(WHITE,map(fadeKilledCrownTimer.getRemaining(),0,5000,0,255)), 5);
                //fade from white to dark and keep just purple
                }
              else {
                setColorOnFace(BUSTCOLOR, 0);
                setColorOnFace(BUSTCOLOR, 2);
                setColorOnFace(BUSTCOLOR, 4);          
              }
            }
          }
          else if ( isCrown ){
            //non-game ending Crown game over spin
            getDisplayFaceGradual(gameOverTimer, 20000);
            setColorOnFace(CROWNCOLOR,displayFaceI);       
          }
          else if ( isBust ){
            //non-game ending Bust game over spin
            getDisplayFaceGradual(gameOverTimer, 20000);
            setColorOnFace(BUSTCOLOR,displayFaceI);
          }
          else {
            //non-Crown or Bust game over spin
            getDisplayFaceGradual(gameOverTimer, 20000);
            setColorOnFace(nonCrownBustSpinColor,displayFaceI);
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
      clicksToKill = random( 3 ) + 3;
      break;
    case 1:
      clicksToKill = random( 2 ) + 2;
      break;
    case 2:
      clicksToKill = random( 1 ) + 1;
      break;
  }
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

//timer/divisor closest to 1 is fastest, closer to 0 is slower
int getDisplayFaceGradual(Timer timer, double divisor) {
  iterateFace = ((double) timer.getRemaining() / (double) divisor);
  cumulativeFace = cumulativeFace + iterateFace;
  displayFaceI = (int) cumulativeFace;
  displayFaceI = displayFaceI % 6;
  return displayFaceI;
}

void resetSetup(){
  isCrown = false;
  isBust = false;
  crownIsSelected = false;
  gameOver = false;
  endedGame = false;
}

void toggleCrownOrBust(){
  if (isCrown){
    isCrown = false;
    isBust = true;
  }
  else if (isBust){
    isBust = false;
    isCrown = true;
  }
  else{
    isBust = true;
  }
}

byte getGamePhase(byte data) {
    return (data & 3);//returns bits [E] [F]
}

byte getSignalState(byte data) {
    return ((data >> 2) & 3);//returns bits [C] [D]
}

byte getFortifyRoles(byte data) {
  return (data >> 5);// [A]
}

byte getFortifyStatus(byte data) { 
  return ((data >> 4));//[B]
}

byte getBubblesOrBalloons(byte data) {
  return (data >> 5);// [A]
}
