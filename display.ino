//Display functions
#if DISP_ACTIVE == 1
  
  //Definitions for display
  #define DISP_WIDTH       320
  #define DISP_HEIGHT      240
  #define DISP_ORIENTATION  90     //Landscape
  
  //Boxes
  #define DISP_TITLE_X       0
  #define DISP_TITLE_Y       0
  #define DISP_TITLE_W       179
  #define DISP_TITLE_H       20
  
  #define DISP_TIME_X        DISP_WIDTH/2
  #define DISP_TIME_Y        0
  #define DISP_TIME_W        DISP_WIDTH-DISP_TIME_X
  #define DISP_TIME_H        20
  
  #define DISP_STATUS_X      20
  #define DISP_STATUS_Y      60
  #define DISP_STATUS_W      DISP_WIDTH
  #define DISP_STATUS_H      80
  
  #define DISP_MESSAGE_X     0
  #define DISP_MESSAGE_W     DISP_WIDTH
  #define DISP_MESSAGE_H     40  
  #define DISP_MESSAGE_Y     DISP_HEIGHT-DISP_MESSAGE_H


  #define DISP_PROG_X        0
  #define DISP_PROG_W        DISP_WIDTH
  #define DISP_PROG_Y        160
  #define DISP_PROG_H        8
  
  void DispInit(void)
  {
  
    //Init display  
    ucg.begin(UCG_FONT_MODE_SOLID);
  
    //Init touchscreen
    ts.begin();
    
    //Pin mode for LED and turn on
    pinMode(DISP_LED_PIN, OUTPUT);
    DispLED(true);
  
    //Set orientation
    switch (DISP_ORIENTATION) {
      case 0:
        ucg.undoRotate();
        ts.setRotation(0);
        break;
      case 90:
        ucg.setRotate90();
        ts.setRotation(1);
        break;
      case 180:
        ucg.setRotate180();
        ts.setRotation(2);
        break;
      case 270:
        ucg.setRotate270();
        ts.setRotation(3);
        break;    
    }
  
    //Clear screen
    ucg.clearScreen();
  
    DispTitle();
  }
  
  void DispLED(bool stLED)
  {
    //Turns on and off LED (active high)
    if (stLED) {
      //turn on
      digitalWrite(DISP_LED_PIN, 1);
    }
    else {
      //turn off
      digitalWrite(DISP_LED_PIN, 0);
    }
  }
  
  void DispStatus(unsigned long tiMeas, int prcActual, int rSignalHealth, int volActual, int volDiff1d)
  {
    //Displays status in status window
    char _strHelp[20];
    int _maxLen, _spaceLen, _actLen, _nSpace, _tend_x, _actHei;
  
    //Backgound color black
    ucg.setColor(1, 0, 0, 0);
  
    //Font select for Percentage
    ucg.setFont(ucg_font_fur42_hr);
  
    //Signal health as color
    if (rSignalHealth < 50) {
      //red
      ucg.setColor(0, 255, 0, 0);
    }
    else {
      //green
      ucg.setColor(0, 0, 255, 0);
    }
  
    //Get maximum length in pixel
    _maxLen = ucg.getStrWidth("100%");
    _spaceLen = ucg.getStrWidth(" ");
  
    //Print actual string
    sprintf(_strHelp, "%d%%", prcActual);
    _actLen = ucg.getStrWidth(_strHelp);
  
    //Add spaces to avoid old text in display
    if (_maxLen - _actLen > 0) {
      _nSpace = (_maxLen - _actLen) / _spaceLen + 1;
      for (int i = 0; i < _nSpace; i++) {
        sprintf(_strHelp, "%s ", _strHelp);
      }
    }
    ucg.setFontPosTop();
    ucg.setPrintPos(DISP_STATUS_X, DISP_STATUS_Y);
    ucg.print(_strHelp);
  
    //in Liter
    ucg.setFontPosTop();
    ucg.setPrintPos(DISP_STATUS_X, DISP_STATUS_Y + ucg.getFontAscent() + 16);
    ucg.setFont(ucg_font_fur17_hr);
  
    //Get maximum length in pixel
    _maxLen = ucg.getStrWidth("8888 Liter");
    _spaceLen = ucg.getStrWidth(" ");
    sprintf(_strHelp, "%d Liter", volActual);
    _actLen = ucg.getStrWidth(_strHelp);
  
    //Add spaces to avoid old text in display
    if (_maxLen - _actLen > 0) {
      _nSpace = (_maxLen - _actLen) / _spaceLen + 1;
      for (int i = 0; i < _nSpace; i++) {
        sprintf(_strHelp, "%s ", _strHelp);
      }
    }
    ucg.print(_strHelp);
  
    //get points
    _tend_x = DISP_STATUS_X + 2 * DISP_STATUS_W / 3;
  
    //Set new text size
    ucg.setFont(ucg_font_fur17_hr);
  
    //get Text length
    sprintf(_strHelp, "%d l/d", abs(volDiff1d));
    _actLen = min(ucg.getStrWidth(_strHelp),80);
    _actHei = ucg.getFontAscent() + 2;
  
    ucg.setColor(0, 0, 0, 0);
    ucg.drawBox(_tend_x, DISP_STATUS_Y, DISP_STATUS_X + DISP_STATUS_W, DISP_STATUS_Y + DISP_STATUS_H);
  
    if (volDiff1d > 0) {
      //Arrow  upwards green
      ucg.setColor(0, 0, 255, 0);
      ucg.drawTriangle(_tend_x, DISP_STATUS_Y + DISP_STATUS_H - _actHei - 4 , _tend_x + _actLen, DISP_STATUS_Y + DISP_STATUS_H - _actHei - 4, _tend_x + _actLen / 2, DISP_STATUS_Y);
  
      //Text
      ucg.setFontPosBaseline();
      ucg.setPrintPos(_tend_x, DISP_STATUS_Y + DISP_STATUS_H);
      ucg.print(_strHelp);
    }
    else {
      //Arrow downwards red
      ucg.setColor(0, 255, 0, 0);
      ucg.drawTriangle(_tend_x, DISP_STATUS_Y + _actHei + 4 , _tend_x + _actLen, DISP_STATUS_Y + _actHei + 4, _tend_x + _actLen / 2, DISP_STATUS_Y + DISP_STATUS_H);
  
      //Text
      ucg.setFontPosTop();
      ucg.setPrintPos(_tend_x, DISP_STATUS_Y);
      ucg.print(_strHelp);
    }
  
  }
  
  void DispTitle(void)
  {
    //Display of headline
    ucg.setColor(0, 0, 0, 0);
    ucg.drawBox(DISP_TITLE_X, DISP_TITLE_Y, DISP_TITLE_W, DISP_TITLE_H);
  
    //Set font type
    ucg.setFont(ucg_font_fur17_hr);
    ucg.setFontPosCenter();
    ucg.setColor(0, 255, 255, 255);
  
    ucg.setPrintPos(DISP_TITLE_X, DISP_TITLE_Y + DISP_TITLE_H / 2 + 2);
    ucg.print("Zisterne");
  }
  
  void DispTime(void)
  {
    char _datetime[20];
    time_t _t;
  
    //Get Time
    _t = getLocTime();
    sprintf(_datetime, "%02d.%02d.%4d %02d:%02d", day(_t), month(_t), year(_t), hour(_t), minute(_t));
  
    //Background color
    ucg.setColor(1, 0, 0, 0);
  
    //Set font an position
    ucg.setFont(ucg_font_fur17_hr);
    ucg.setFontPosCenter();
    ucg.setColor(0, 255, 255, 255);
  
    //Align right
    ucg.setPrintPos(DISP_TIME_X + DISP_TIME_W - ucg.getStrWidth(_datetime), DISP_TIME_Y + DISP_TIME_H / 2 + 2);
    ucg.print(_datetime);
  }
  
  
  void DispMessage(byte Type, String Message)
  {
    //Display message in message area 
    char _line[80];
    String _Line1,_Line2;
    int _sep;
  
    //delete old text
    ucg.setColor(0, 0, 0, 0);
    ucg.drawBox(DISP_MESSAGE_X, DISP_MESSAGE_Y - 2, DISP_MESSAGE_W, DISP_MESSAGE_H);
  
    //Background color
    ucg.setColor(1, 0, 0, 0);
  
    //Set font an position
    ucg.setFont(ucg_font_fur17_hr);
    ucg.setFontPosBottom();    
    switch (Type) {
    case MSG_DEBUG:
   
      break;
    case MSG_INFO:     
      ucg.setColor(0, 255, 255, 255);
      break;      
    case MSG_WARNING:     
      ucg.setColor(0, 255, 255, 0);
      break;      
    case MSG_MED_ERROR:     
      ucg.setColor(0, 255, 0, 0);
      break;      
    case MSG_SEV_ERROR:
      ucg.setColor(0, 255, 0, 0);
      break;         
    }
  
    //Print text
    if ((Type==MSG_INFO) || (Type==MSG_WARNING) || (Type==MSG_SEV_ERROR) || (Type==MSG_MED_ERROR)) {      
      _sep = Message.indexOf("] ");
      _Line1 = Message.substring(_sep + 1);
      _Line2 = "";
      //Line Break
      _sep=0;      
      while (_sep<_Line1.length()) {        
        _Line1.toCharArray(_line, _sep);
        if (ucg.getStrWidth(_line)>DISP_MESSAGE_W) {
         //Line is longer, set breake at _sep
         _Line2 = _Line1.substring(_sep);
         _Line1 = _Line1.substring(1,_sep); 
         break;
        }        
        _sep++;
      }
      ucg.setPrintPos(DISP_MESSAGE_X, DISP_MESSAGE_Y + 20);
      ucg.print(_Line1);
      ucg.setPrintPos(DISP_MESSAGE_X, DISP_MESSAGE_Y + DISP_MESSAGE_H);
      ucg.print(_Line2);
    }    
  }

  void DispMeasProg(int rMeas)
  {
    //Display measurement in Progress
   
    //Trigger WDT
    TriggerWDT(); 
    
    //Bar in White
    ucg.setColor(0, 255, 255, 255);
    ucg.drawBox(DISP_PROG_X, DISP_PROG_Y, (rMeas*DISP_PROG_W)/100, DISP_PROG_H);   
  }

  void DelMeasProg()
  {
    //Display measurement in Progress
  
    //delete old bar
    ucg.setColor(0, 0, 0, 0);
    ucg.drawBox(DISP_PROG_X, DISP_PROG_Y, DISP_PROG_W, DISP_PROG_H);

    //Trigger WDT
    TriggerWDT(); 
       
  }
#endif


