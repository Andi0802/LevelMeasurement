//Display functions

//Definitions for display
#define DISP_WIDTH       320
#define DISP_HEIGHT      240
#define DISP_ORIENTATION  90     //Landscape

void DispInit(void)
{
  //Init display  
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);  
  
  //Set orientation
  switch(DISP_ORIENTATION) {
    case 0: 
      ucg.undoRotate();
      break;
    case 90:
      ucg.setRotate90();
      break;
    case 180:
      ucg.setRotate180();
      break;
    case 270:
      ucg.setRotate270();
      break;
  }

  //Clear screen
  ucg.clearScreen();
  
  //Init message
  ucg.setPrintPos(0,25);
  ucg.print("Screen init");
}

void DispStatus(unsigned long tiMeas, int prcActual, int volActual, int volDiff1h)
{
  //Displays status in status window
  
}

void DispStatusDetails(int rSigHealth, float volRain1h, float volRain24h)
{
  //Displays status details in details window
  
}

void DispHeadline(unsigned long tiActual)
{
  //Display of headline with date and time
  
}


void DispMessage(String Message)
{
  //Display message in message area
  
}

void DispHistVol(void)
{
  //Displays history values
  
}

void DispHistRain(void)
{
  //Displays rain history 
  
}


