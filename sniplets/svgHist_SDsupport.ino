
void WriteHistSVG()
//Write SVG File with history data
{
  File svgFile;
  unsigned char _iDay,_iHour,_idxRd;
  unsigned int _idxPic;
  unsigned int _yValPic;
  
  if (SD_State == SD_OK) {
    #if LOGLEVEL & LOGLVL_NORMAL
      WriteSystemLog(MSG_DEBUG,F("Writing svg File to SD Card"));
    #endif
        
    Workaround_CS_ETH2SD(70);        

    //Header
    CopyFile(SVGHEADER,SVGFILE);

    //Open File    
    svgFile = SD.open(SVGFILE, FILE_WRITE);  

    //Date 
    svgFile.println(F("<!-- Day scale: Fill dynamically -->"));
    svgFile.println(F("<g font-family=\"Verdana\" font-size=\"6\" fill=\"black\" text-anchor=\"middle\">"));
    for (_iDay=0; _iDay<10; _iDay++) {
      svgFile.print(F("<text x=\""));
      svgFile.print(String(228-24*_iDay)); 
      svgFile.print(F("\" y=\"180\">"));
      svgFile.print(String((-1)*_iDay)); //Hier noch durch das Datum ersetzen
      svgFile.println(F(".</text>"));
    }
    svgFile.println(F("</g>"));
   
    //Level
     svgFile.println(F("<!-- Level Line: Fill dynamically -->"));
     svgFile.println(F("<!-- Formula: y=216+hour()-nPoint, y=100-prcActual(nPoint) -->"));
     svgFile.println(F("<polyline fill=\"none\" stroke=\"blue\" stroke-width=\"1\"points="));
     for(_iHour=0;_iHour<240;_iHour++) {
      _idxPic = 216+hour()-_iHour;
      if (_idxPic>0) {
        _idxRd = (EEP_NUM_HIST+SettingsEEP.settings.iWrPtrHist-_iHour)%EEP_NUM_HIST;
        _yValPic = max(min(100-SettingsEEP.settings.prcActual[_idxRd],100),0);
        //Serial.println(String(_idxRd)+" ->"+ SettingsEEP.settings.prcActual[_idxRd]);
        svgFile.print("\""+String(_idxPic)+"\",\""+String(_yValPic)+"\" ");
      }
     }
     svgFile.println(F("\"/>")); 
 
    //Rain
    svgFile.println(F("<!-- Rain bar series: fill dynamically -->"));
    svgFile.println(F("<!-- Formula: y=216+hour()-nPoint, y=170-Rain(nPoint) -->"));
    svgFile.println(F("<g fill=\"none\" stroke=\"blue\" stroke-width=\"1\">"));
    for(_iHour=0;_iHour<240;_iHour++) {
      _idxPic = 216+hour()-_iHour;
      if (_idxPic>0) {
        _idxRd = (SettingsEEP.settings.iWrPtrHist-_iHour)%EEP_NUM_HIST;
        _yValPic = max(min(170-SettingsEEP.settings.volRain1h[_idxRd],170),100);
        svgFile.println("<polyline points=\""+String(_idxPic)+"\",\"170\" \""+String(_idxPic)+"\",\""+String(_yValPic)+"\"/>");
      } 
    }
    svgFile.println(F("</g>"));

    //Trailer
    svgFile.print("</svg>");

    //Newline and Close
    svgFile.println();
    svgFile.flush();
    svgFile.close();

    Workaround_CS_SD2ETH(70);   
  }
}

//--- SD Card support ---------------------------------------------------------------------------------------------------
void CopyFile(String srcFile,String dstFile)
{
  //Copy file on SD Card  
  File src,dst;

  //Open source
  src = SD.open(srcFile,FILE_READ);
  
  //Remove target if it exists already
  if (SD.exists(dstFile)) {
    SD.remove(dstFile);
  }

  //Open destination
  dst = SD.open(dstFile,FILE_WRITE);

  //Copy
  while (src.available()) {
    dst.write(src.read());
    // Trigger Watchdog
    TriggerWDT();
  }

  dst.flush();
  dst.close();
  src.close();
}

// in webserver:
          else if (pageStr.equals(SVGFILE)>0) {
            //Serve svg-File
            Workaround_CS_ETH2SD(40);
            File webFile = SD.open(pageStr);        // open file
            Workaround_CS_SD2ETH(40);
            client2.println(F("HTTP/1.1 200 OK"));
            client2.println(F("Content-Type: text/html"));            
            client2.println(F("Connection: close"));
            client2.println();
            Workaround_CS_ETH2SD(41);
            
            if (webFile) {             
              //If file is available: start transfer
              while (webFile.available()) { 
                //Read file content                
                clientBuf[clientCount] = webFile.read();                
                clientCount++;                
                if (clientCount > SD_BLOCK_SIZE - 1) {             
                  //If a full block is read: transfer 
                  //Don't activate folowing line if LOGLEVEL TRAP is active
                  Workaround_CS_SD2ETH(42);     
                  //Serial.println("Packet");
                  client2.write(clientBuf, SD_BLOCK_SIZE);                  
                  //do not call .flush() here otherwise tranfer wil be slow
                  clientCount = 0;                  
                }
                // Trigger Watchdog
                TriggerWDT();  
                Workaround_CS_ETH2SD(43);                                                                              
              }                                            
              webFile.close(); 
              //final <SD_BLOCK_SIZE byte cleanup packet              
              if (clientCount > 0) {
                Workaround_CS_SD2ETH(43);
                client2.write(clientBuf, clientCount);                            
                client2.flush();
              }                      
            }
          }