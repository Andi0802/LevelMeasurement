//hm_access: Functions to access HOMEMATIC CCU

#if HM_ACCESS_ACTIVE==1   
  void hm_set_state(int ise_id, double value)
  // Change state of channel in Homematic CCU
  // Use XML-API; use ise_id of channel
  {
    String _cmd,_ret;
  
    //Generate command
    _cmd = "GET /addons/xmlapi/statechange.cgi?ise_id=";
    _cmd = _cmd + ise_id;
    _cmd = _cmd + "&new_value=";
    _cmd = _cmd + value;
        
    //Connect and send
    if (hm_client.connect(hm_ccu, 80)) {    
      hm_client.print(_cmd);
      hm_client.println(" HTTP/1.1");
      hm_client.print(F("Host: "));
      hm_client.print(hm_ccu[0]); hm_client.print(F("."));
      hm_client.print(hm_ccu[1]); hm_client.print(F("."));
      hm_client.print(hm_ccu[2]); hm_client.print(F("."));
      hm_client.println(hm_ccu[3]);
      hm_client.println(F("Connection: close"));
      hm_client.println();
      hm_client.flush();

      //Receive answer
      while (hm_client.connected()) {          
          if (hm_client.available()) {
            char c = hm_client.read();            
    
            //read char by char HTTP request
            if (_ret.length() < 1024) {
    
              //store characters to string
              _ret += c;           
            }
         }
      }
      TriggerWDT();            
      hm_client.stop();      
      #if LOGLEVEL & LOGLVL_CCU
        WriteSystemLog(MSG_DEBUG,_cmd);
        WriteSystemLog(MSG_DEBUG,_ret);
      #endif              
    } else {    
      WriteSystemLog(MSG_WARNING,F("Connection to CCU failed"));
    }
  }
  
  void hm_run_program(int ise_id)
  // Runs program in Homematic CCU
  // Use XML-API; use ise_id of program
  {
    String _cmd,_ret;
  
    //Generate command
    _cmd = F("GET /addons/xmlapi/runprogram.cgi?ise_id=");
    _cmd = _cmd + ise_id;
  
    //Connect and send
    if (hm_client.connect(hm_ccu, 80)) {    
      hm_client.print(_cmd);
      hm_client.println(" HTTP/1.1");
      hm_client.print(F("Host: "));
      hm_client.print(hm_ccu[0]); hm_client.print(F("."));
      hm_client.print(hm_ccu[1]); hm_client.print(F("."));
      hm_client.print(hm_ccu[2]); hm_client.print(F("."));
      hm_client.println(hm_ccu[3]);
      hm_client.println(F("Connection: close"));
      hm_client.println();      
      hm_client.flush();

      //Receive answer
      while (hm_client.connected()) {          
          if (hm_client.available()) {
            char c = hm_client.read();            
    
            //read char by char HTTP request
            if (_ret.length() < 1024) {
    
              //store characters to string
              _ret += c;           
            }
         }
      }
      TriggerWDT();            
      
      hm_client.stop();
      #if LOGVEVEL & LOGLVL_CCU
        WriteSystemLog(MSG_DEBUG,_cmd);
      #endif
    } else {    
        WriteSystemLog(MSG_WARNING,F("Connection to CCU failed"));
    }
  }
  
  double hm_get_datapoint(int dp_number)
  // Gets information to Homematic CCU
  // Using XML-API
  {
    String _cmd, _ret, _cmpStr, _repStr;
    int idxStart, idxEnd;
    char c;
    int tOutCtr;
  
    //Generate command
    _cmd = F("GET /addons/xmlapi/state.cgi?datapoint_id=");
    _cmd = _cmd + dp_number;
    _cmd = _cmd + F(" HTTP/1.1");
  
    //Connect and send
    if (hm_client.connect(hm_ccu, 80)) {
      //WriteSystemLog(MSG_DEBUG,_cmd);
      hm_client.println(_cmd);
      hm_client.print(F("Host: "));
      hm_client.print(hm_ccu[0]); hm_client.print(F("."));
      hm_client.print(hm_ccu[1]); hm_client.print(F("."));
      hm_client.print(hm_ccu[2]); hm_client.print(F("."));
      hm_client.println(hm_ccu[3]);
      hm_client.println(F("Connection: close"));
      hm_client.println();
      hm_client.flush();
  
      unsigned long _timeStart=micros();
      unsigned char _trace=0;
      while (hm_client.connected()) {          
          _trace=1;
          if (hm_client.available()) {
            char c = hm_client.read();
            _trace++;
    
            //read char by char HTTP request
            if (_ret.length() < 1024) {
    
              //store characters to string
              _ret += c;           
            }
         }
      }
      TriggerWDT();  
      hm_client.stop();
      unsigned long _timeStop=micros();
      
      //Analyse return string
      _cmpStr = F("<datapoint ise_id='");
      _cmpStr = _cmpStr + dp_number;
      _cmpStr = _cmpStr + F("' value='");
      idxStart = _ret.indexOf(_cmpStr) + _cmpStr.length();
      _cmpStr = F("'/></state>");
      idxEnd = _ret.indexOf(_cmpStr);
      if ((idxStart > -1) && (idxEnd > -1)) {
        _ret = _ret.substring(idxStart, idxEnd);
        _repStr = F("Datapoint ");
        _repStr = _repStr + dp_number;
        _repStr = _repStr + F(" received ") + _ret;
        #if LOGLEVEL & LOGLVL_CCU
          WriteSystemLog(MSG_DEBUG,_repStr);
        #endif
      }
      else {      
        WriteSystemLog(MSG_WARNING,F("Transmission error while receiving datapoint"));
        WriteSystemLog(MSG_DEBUG,"Time  : "+String(_timeStop-_timeStart)+" us");
        WriteSystemLog(MSG_DEBUG,"Return: "+_ret);
        WriteSystemLog(MSG_DEBUG,"Trace : "+String(_trace));
        _ret = F("NaN");
      }   
    } else {
      WriteSystemLog(MSG_WARNING,F("Connection to CCU failed"));
      _ret = F("NaN");
    }
    return _ret.toFloat();
  }
#endif
