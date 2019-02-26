//Web_server: Functions to handle web-server

unsigned char getOption(String str, String Option, double minVal, double maxVal, int *curVal )
{
  // Read options from HTTP GET request  
  int idxOption, lenOption, idxOptionEnd;
  double numberIn;
  String strRem;
  unsigned char stResult;
  String strLog;

  // Is Option available?
  strLog = "Option " + Option;
  strLog = strLog + ": ";

  Option = Option + "=";
  idxOption = str.indexOf(Option);
  lenOption = Option.length();
  if (idxOption > 0) {
    //Option found, Check remaining string for end of option
    strRem = str.substring(idxOption + lenOption - 1);
    idxOptionEnd = strRem.indexOf("&");
    if (idxOptionEnd < 0) {
      //Last argument
      idxOptionEnd = strRem.indexOf(" HTTP");
    }
    #if LOGLEVEL & LOGLVL_WEB
      Serial.println("Remaining string     : " + strRem);
      Serial.println("Start index of option: " + String(idxOption));
      Serial.println("End index of option  : " + String(idxOption+idxOptionEnd));
    #endif

    if (idxOptionEnd == 1) {
      //Empty option, keep current value
      numberIn = *curVal;
      stResult = 0;
      strLog = strLog + "empty";
    }
    else {
      //Get value of option
      strRem = strRem.substring(1, idxOptionEnd);
      if (strRem.equals("on")) {
        numberIn = 1;
      }
      else {
        numberIn = strRem.toFloat();
      }
      
      //Serial.print("New value raw ");
      //Serial.println(strRem);

      //Check range
      if ((numberIn >= minVal) && (numberIn <= maxVal)) {
        strLog = strLog + F("New value ");
        strLog = strLog + numberIn;
        stResult = 1;
      }
      else {
        strLog = strLog + F("Value out of range (");
        strLog = strLog + numberIn;
        strLog = strLog + F(") keeping old value ");
        strLog = strLog + *curVal;
        numberIn = *curVal;
        stResult = 0;
      }
    }
  }
  else {
    numberIn = *curVal;
    strLog = strLog + F("No option found, keeping old value  ");
    strLog = strLog + numberIn;
    stResult = 0;
  }
  *curVal = numberIn;

  //Log entry
  #if LOGLEVEL & LOGLVL_WEB
    WriteSystemLog(MSG_DEBUG,strLog);
  #endif

  return stResult;
}

void MonitorWebServer(void)
{
  unsigned int stOption;
  byte clientBuf[SD_BLOCK_SIZE];
  int clientCount = 0;
  unsigned char _id,idxRd;

  stOption = 0;
  
  //Web server code to change settings  
  // Create a client connection
  EthernetClient client2 = server.available();
  if (client2) {
    while (client2.connected()) {
      // Trigger Watchdog
      TriggerWDT();  
      if (client2.available()) {
        char c = client2.read();

        //read char by char HTTP request
        if (inString.length() < 255) {

          //store characters to string
          inString += c;
          //Serial.print(c);
        }

        //if HTTP request has ended
        if (c == '\n') {
          //print to serial monitor for debuging
          String pageStr = "";
          if (inString.indexOf("?") < 0) {
            //Serial.println(inString);
            pageStr = inString.substring(5, inString.indexOf(" HTTP"));
          }
          #if LOGLEVEL & LOGLVL_WEB
            WriteSystemLog(MSG_DEBUG,"HTTP Request receives: " + pageStr);
            WriteSystemLog(MSG_DEBUG,inString);
          #endif 

          //Check if reset or reprogramm is requested
          if (pageStr.equals("reset")>0) {
            //Do a normal reset
            WriteSystemLog(MSG_DEBUG,"Reset requested by user");
            NetEEPROM.writeImgOk();
            WriteSystemLog(MSG_DEBUG,"EEP Status: " + String(eeprom_read_byte(NETEEPROM_IMG_STAT)));
            delay(1000);
            resetFunc();
          }
          else if (pageStr.equals("reprogram")>0) {
            //Start reprogramming
            //WriteSystemLog(MSG_DEBUG,"Reprogramming requested by user");
            //NetEEPROM.writeImgBad();
            //WriteSystemLog(MSG_DEBUG,"EEP Status: " + String(eeprom_read_byte(NETEEPROM_IMG_STAT)));
            //delay(1000);
            //resetFunc();
          }
          else if ((pageStr.equals(LOGFILE)>0) || ((pageStr.equals(DATAFILE)>0))) {           
            //Read log-file or data-file
            Workaround_CS_ETH2SD(40);
            File webFile = SD.open(pageStr);        // open file
            Workaround_CS_SD2ETH(40);
            client2.println(F("HTTP/1.1 200 OK"));
            client2.println(F("Content-Type: text/comma-separated-values"));
            client2.println(F("Content-Disposition: attachment;"));
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
              
              //Remove downloaded file
              Workaround_CS_ETH2SD(44);
              SD.remove(pageStr);
              Workaround_CS_SD2ETH(44);
              if (pageStr.equals(DATAFILE)) {
                // Create new headline
                LogDataHeader();
              }              
            }
            else {
              WriteSystemLog(MSG_WARNING,"File not found: "+pageStr);
            }
            delay(1);                    // give the web browser time to receive the data
          }          
          else {
            //Query options            
            stOption |= getOption(inString, "vSoundSet", 3000, 3500, &SettingsEEP.settings.vSound) << 2;
            stOption |= getOption(inString, "hOffsetSet", -500, 500, &SettingsEEP.settings.hOffset) << 3;
            stOption |= getOption(inString, "aBaseAreaSet", 25000, 35000, &SettingsEEP.settings.aBaseArea) << 4;
            stOption |= getOption(inString, "hSensorPosSet", 205, 300, &SettingsEEP.settings.hSensorPos) << 5;
            stOption |= getOption(inString, "hOverflowSet", 180, 220, &SettingsEEP.settings.hOverflow) << 6;
            stOption |= getOption(inString, "hRefillSet", 10, 60, &SettingsEEP.settings.hRefill) << 7;
            stOption |= getOption(inString, "volRefillSet", 10, 50, &SettingsEEP.settings.volRefill) << 8;
            stOption |= getOption(inString, "dvolRefillSet", 2, 20, &SettingsEEP.settings.dvolRefill) << 9;          
            stOption |= getOption(inString, "prcVolDvtThres", 0, 500, &SettingsEEP.settings.prcVolDvtThres) << 10;          
            stOption |= getOption(inString, "aRoof", 0, 500, &SettingsEEP.settings.aRoof) << 11;          
            stOption |= getOption(inString, "idxCur", 0, 100, &idxCur) << 15;
            stOption |= getOption(inString, "prcFiltEff", 0, 100, &SettingsEEP.settings.prcFiltEff_y[idxCur]) << 12;
            stOption |= getOption(inString, "volRainMin", 0, 10, &SettingsEEP.settings.volRainMin) << 13;
            stOption |= getOption(inString, "volRainFilt1h", 0, 100, &SettingsEEP.settings.prcFiltEff_x[idxCur]) << 14;
          }                    
   
          //New option received
          if (stOption > 0) {
            // Write to EEPROM
            WriteEEPData();          
          }

          //clearing string for next read
          inString = "";

          if (pageStr.length() == 0) {
            // Print main page

            client2.println(F("HTTP/1.1 200 OK")); //send new page
            client2.println(F("Content-Type: text/html"));
            client2.println(F("Connection: close"));
            client2.println();

            client2.println(F("<HTML><HEAD>"));            
            client2.println(F("<TITLE>Zisterne</TITLE>"));
            client2.println(F("<style>table, td, th { border: 1px solid black; }</style>"));
            client2.println(F("</HEAD>"));
            client2.println(F("<BODY>"));           
            client2.println(F("<H1>Zisterne</h1>"));            

            //Status Info
            client2.println(F("<H2>Status</h2>"));
            client2.println(F("<H3>Messwerte</h3>"));
            client2.print(F("<table><tr><td>Messzeit (UTC)</td><td>"));
            client2.print(MeasTimeStr);
            client2.print(F("</td></tr>"));
            client2.print(F("<tr><td>Messung aktiv [%]</td><td>"));
            if (stMeasAct) {
              client2.print(String(pos));  
            }
            else {
              client2.print(F("-"));  
            }
            
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Pegel [mm]</td><td>"));
            client2.print(String(hWaterActual));
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Inhalt [Liter] </td><td>"));
            client2.print(volActual);
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Tendenz [Liter] </td><td>"));
            client2.print(volDiff1h);
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Standardabweichung [Liter] </td><td>"));
            client2.print(volStdDev);
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Fuellstand [%]</td><td>"));
            client2.print(prcActual);
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Signalqualitaet [-]</td><td>"));
            client2.print(String(rSignalHealth));            
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Fehlerzustand [-]</td><td>"));
            if (stError) {
              client2.print(F("Fehler erkannt"));            
            }
            else {
              client2.print(F("Kein Fehler erkannt"));
            }
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Regenmenge 1h [Liter]</td><td>"));
            client2.print(String(volRain1h));
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Regenmenge 24h [Liter]</td><td>"));
            client2.print(String(volRain24h));            
            client2.print(F("</td></tr></table>"));  
            client2.println(F("<H3>Nachspeisung</h3>"));
            client2.print(F("<table><tr><td>Nachspeisung aktiv</td><td>"));
            if (stRefill) {
              if (stButton) {
                client2.print(F("manuell"));  
              }
              else {
                client2.print(F("automatisch"));  
              }
            }
            else {
              client2.print(F("-"));  
            }
            
            client2.print(F("</td></tr>"));                     
            client2.print(F("<tr><td>Nachspeisung [Liter]</td><td>"));
            client2.print(String(SettingsEEP.settings.volRefillTot));
            client2.print(F("</td></tr>"));                     
            client2.print(F("<tr><td>Reset Nachspeisung am</td><td>"));
            client2.print(String(time2DateTimeStr(SettingsEEP.settings.tiRefillReset)));
            client2.print(F("</td></tr></table>"));                                 

            //Filter diagnosis
            client2.println(F("<H3>Filterdiagnose</h3>"));
            client2.print(F("<table><tr><td>Diagnosezeit (UTC)</td><td>"));
            client2.print(String(DiagTimeStr));
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Regenmenge 1h [mm]</td><td>"));
            client2.print(String(volRainDiag1h));
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Nachspeisung 1h [Liter] </td><td>"));
            client2.print(String(volRefillDiag1h));
            client2.print(F("</td></tr>"));          
            client2.print(F("<tr><td>Verbrauch 1h [Liter]</td><td>"));
            client2.print(String(VOL_USAGE_MAX_1H));
            client2.print(F("</td></tr>"));                
            client2.print(F("<tr><td>Fuellstandsaenderung 1h [Liter] </td><td>"));
            client2.print(String(volDiffDiag1h));            
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Berechnete Fuellstandsaenderung 1h [Liter]</td><td>"));
            client2.print(String(volDiffCalc1h));
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Abweichung [%]</td><td>"));
            client2.print(String(prcVolDvt1h));
            client2.print(F("</td></tr>"));                          
            client2.print(F("<tr><td>Filterzustand [-]</td><td>"));
            switch (stFiltChkErr) {
              case FILT_ERR_UNKNOWN:
                client2.print(F("Unbekannt"));            
                break;
              case FILT_ERR_NOMEAS:
                client2.print(F("Ungueltige Pegelmessung"));            
                break;
              case FILT_ERR_CLOGGED:
                client2.print(F("Verstopft"));            
                break;
              case FILT_ERR_OK:
              client2.print(F("OK"));            
            }            
            client2.print(F("</td></tr>"));              
            client2.print(F("</td></tr></table>"));  

            //Verbrauchsberechnung
            client2.println(F("<H3>Verbrauchsberechnung</h3>"));
            client2.print(F("<table><tr><td>Berechungszeit (UTC)</td><td>"));
            client2.print(String(UsageTimeStr));
            client2.print(F("</td></tr>"));                        
            client2.print(F("<tr><td>Nachspeisung 24h [Liter] </td><td>"));
            client2.print(String(volRefill24h));
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Fuellstandsaenderung 24h [Liter] </td><td>"));
            client2.print(String(volDiff24h1d));
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Verbrauch 24h [Liter]</td><td>"));
            client2.print(String(SettingsEEP.settings.volUsage24h[SettingsEEP.settings.iDay]));
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Gleitender Mittelwert 10 Tage [Liter]</td><td>"));
            client2.print(String(volUsageAvrg10d));            
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Minimum 10 Tage [Liter]</td><td>"));
            client2.print(String(volUsageMin10d));            
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Maximum 10 Tage [Liter]</td><td>"));
            client2.print(String(volUsageMax10d));            
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Einzelwerte 10 Tage [Liter]</td><td>"));
            for (_id=0; _id<NUM_USAGE_AVRG; _id++) {
              idxRd = (SettingsEEP.settings.iDay-_id)%NUM_USAGE_AVRG;
              client2.print(String(SettingsEEP.settings.volUsage24h[idxRd])+" ");            
            }
            
            client2.print(F("</td></tr>"));              
            client2.print(F("</table>"));  

            //History
            client2.println(F("<H3>History</h3>"));
            client2.print(F("<table><tr><td>Index</td>"));
            client2.print(F("<td>Fuellstand [%]</td>"));
            client2.print(F("<td>Regen 1h [Liter]</td>"));
            client2.print(F("<td>Refill [Liter]</td>"));
            client2.print(F("<td>Signal US [-]</td>"));
            client2.print(F("<td>Signal RAIN [-]</td></tr>"));
            for (_id=0;_id<EEP_NUM_HIST;_id++) {
                idxRd=(SettingsEEP.settings.iWrPtrHist-_id-1)%EEP_NUM_HIST;
                client2.print(F("<tr><td>"));                        
                client2.print(idxRd);   
                client2.print(F("</td><td>"));                        
                client2.print(String(SettingsEEP.settings.prcActual[idxRd]));   
                client2.print(F("</td><td>"));   
                client2.print(String(SettingsEEP.settings.volRain1h[idxRd]));                       
                client2.print(F("</td><td>"));                   
                client2.print(String(((SettingsEEP.settings.stSignal[idxRd]) >> 2)*CONV_REFILL));    
                client2.print(F("</td><td>"));                   
                client2.print(String(SettingsEEP.settings.stSignal[idxRd] & 1));    
                client2.print(F("</td><td>"));                   
                client2.print(String((SettingsEEP.settings.stSignal[idxRd] & 2)>>1));    
                client2.print(F("</td></tr>"));                        
            }                               
            client2.print(F("</table>"));  
            
            //Settings          
            client2.println(F("<H2>Einstellungen</H2>"));
            client2.println(F("<form method='get'>"));
            client2.print(F("<table><tr><th>Parameter</th><th>Aktueller Wert</th><th>Neuer Wert</th></tr>"));            
            
            client2.print(F("<tr><td>Schallgeschwindigkeit [mm/s]: </td><td>"));
            client2.print(SettingsEEP.settings.vSound);            
            client2.println(F("</td><td> <input type='text' name='vSoundSet'></td></tr>"));

            client2.print(F("<tr><td>Messoffset [mm]: </td><td>"));
            client2.print(SettingsEEP.settings.hOffset);            
            client2.println(F("</td><td> <input type='text' name='hOffsetSet'></td></tr>"));
            
            client2.print(F("<tr><td>Grundflaeche [cm^2]: </td><td>"));
            client2.print(SettingsEEP.settings.aBaseArea);            
            client2.println(F("</td><td> <input type='text' name='aBaseAreaSet'></td></tr>"));

            client2.print(F("<tr><td>Sensor Hoehe [cm]: </td><td>"));
            client2.print(SettingsEEP.settings.hSensorPos);            
            client2.println(F("</td><td> <input type='text' name='hSensorPosSet'></td></tr>"));

            client2.print(F("<tr><td>Ueberlauf Hoehe [cm]: </td><td>"));
            client2.print(SettingsEEP.settings.hOverflow);            
            client2.println(F("</td><td> <input type='text' name='hOverflowSet'></td></tr>"));

            client2.print(F("<tr><td>Pegel fuer Nachspeisung [cm]: </td><td>"));
            client2.print(SettingsEEP.settings.hRefill);            
            client2.println(F("</td><td> <input type='text' name='hRefillSet'></td></tr>"));

            client2.print(F("<tr><td>Dachflaeche [m^2]: </td><td>"));
            client2.print(SettingsEEP.settings.aRoof);            
            client2.println(F("</td><td> <input type='text' name='aRoof'></td></tr>"));

            client2.print(F("<tr><td>Index Kurve [-]: </td><td>"));
            client2.print(idxCur);            
            client2.println(F("</td><td> <input type='text' name='idxCur'></td></tr>"));

            client2.print(F("<tr><td>Regenmenge Filter [Liter/h]: </td><td>"));
            for (_id=0;_id<5;_id++) {
              client2.print(SettingsEEP.settings.prcFiltEff_x[_id]);            
              if (_id<4) {
                client2.print(F(", "));
              }
            }
            client2.println(F("</td><td> <input type='text' name='volRainFilt1h'></td></tr>"));
            
            client2.print(F("<tr><td>Filtereffizienz [%]: </td><td>"));
            for (_id=0;_id<5;_id++) {
              client2.print(SettingsEEP.settings.prcFiltEff_y[_id]);            
              if (_id<4) {
                client2.print(F(", "));
              }
            }          
            client2.println(F("</td><td> <input type='text' name='prcFiltEff'></td></tr>"));
            
            client2.print(F("<tr><td>Schwellwert Diagnose Filter [%]: </td><td>"));
            client2.print(SettingsEEP.settings.prcVolDvtThres);            
            client2.println(F("</td><td> <input type='text' name='prcVolDvtThres'></td></tr>"));                
            
            client2.print(F("<tr><td>Nachspeisevolumen [l]: </td><td>"));
            client2.print(SettingsEEP.settings.volRefill);            
            client2.println(F("</td><td> <input type='text' name='volRefillSet'></td></tr>"));

            client2.print(F("<tr><td>Nachspeisung Volumenstrom [l/min]: </td><td>"));
            client2.print(SettingsEEP.settings.dvolRefill);            
            client2.println(F("</td><td> <input type='text' name='dvolRefillSet'></td></tr>"));                

            client2.print(F("<tr><td>Minimal erforderliche Regenmenge in 1h fuer Filterdiagnose [mm]: </td><td>"));
            client2.print(SettingsEEP.settings.volRainMin);            
            client2.println(F("</td><td> <input type='text' name='volRainMin'></td></tr>"));                
            
            // Submit button
            client2.println(F("</table><br><input type='submit' value='Submit'>"));

            //Data logger file
            client2.print(F("<h2>Logfiles</h2><a href='"));
            client2.print(DATAFILE);
            client2.print(F("' type='text/text' download='"));
            client2.print(DATAFILE);
            client2.print(F("'>Get Data Log File</a><br>"));

            //System log File
            client2.print(F("<a href='"));
            client2.print(LOGFILE);
            client2.print(F("' type='text/text' download='"));
            client2.print(LOGFILE);
            client2.print(F("'>Get System Log File</a>"));

            //Reset and Reprogram
            client2.println(F("<H2>Reset</h2>"));
            client2.println(F("<a href=\"reset\">Reset</a><br>"));
            //client2.println(F("<a href=\"reprogram\">Flash new program</a>"));

            //Version Info
            client2.println(F("<H2>Programminfo</h2>"));
            client2.println("<p>" + prgVers + "</p>");
            client2.println("<p>" + prgChng + "</p>");
            client2.println("<p>" + cfgInfo + "</p>");
            
            client2.println(F("</BODY></HTML>\n"));            
            client2.println();
          }         
          delay(1);
          // Wait to send all data
          client2.flush();
          
          //stopping client
          client2.stop();
        }
      }
    }
  }
}

