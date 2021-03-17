// Functions to handle SQL client 
#if SQL_CLIENT>0
  void ConnectSQL(void) {
    // Connects to SQL server
    WriteSystemLog(MSG_INFO,"Connection SQL server... ");
    if (MySQLCon.connect(sql_addr, SQL_PORT, SQL_USR, SQL_PWD)) { 
      delayWdt(1000);
      WriteSystemLog(MSG_INFO,F("SQL server connected."));
    }
    else {
      WriteSystemLog(MSG_WARNING,F("Connection SQL server failed. "));
    } 
  }
  
  void SendDataSQL(void) {
    // Sends data to SQL server 
    row_values *row = NULL;
    long head_count = 0;
    MySQL_Cursor *MySQLCur = new MySQL_Cursor(&MySQLCon);
    char *chquery;
    
    // Query head
    //Page for operational DB
    String query =F(" (Zeit, Level, Quantity, `Signal health`,Error,StDev,`Refill Quantity cum`,\
  `Rain Quantity 24h`,`Refill quantity`,`Measured quantity change`,`Calculated quantity change`,\
  `Quantity difference`,`filter diagnosis`,`Rain quantity`) VALUES (");
    
    // Build query
    query = "INSERT INTO "+String(SQL_DATABASE)+"."+String(SQL_TABLE)+query + "'" + MeasTimeStr + "'," + String(hWaterActual) + "," + String(volActual)+","+String(rSignalHealth) + ","+\
            String(stError)+"," + String(volStdDev)+"," + String(SettingsEEP.settings.volRefillTot) +","+ReplaceNAN(volRainTot,"0")+","+\
            String(volRefillDiag1h)+","+String(volDiffDiag1h)+","+String(volDiffCalc1h)+","+String(volDiff1h)+","+String(stFiltChkErr)+\
            ","+String(volRainDiag1h)+");";
    #if LOGLEVEL & LOGLVL_SQL
      WriteSystemLog(MSG_DEBUG,query);
    #endif
    
    // Execute the query, limit memory to 1K
    if (query.length()<1000) {
      // Allocate memory
      chquery = (char*) malloc(query.length());

      // Check if memory allocation was successful
      if (chquery!=NULL) {                   
        // convert to char array    
        query.toCharArray(chquery,query.length());
  
        // Connect to server
        ConnectSQL();
        
        // Send query
        if (MySQLCur->execute(chquery)) {
          // Trigger Watchdog
          TriggerWDT();
          // Note: since there are no results, we do not need to read any data
          // Deleting the cursor also frees up memory used
          delete MySQLCur;             
          WriteSystemLog(MSG_INFO,F("SQL query: Success"));
        }
        else {
          WriteSystemLog(MSG_INFO,F("SQL Server not connected."));
        }
        // Release memory
        free(chquery);
      }
      else {
        WriteSystemLog(MSG_INFO,F("Error while allocating memory"));        
      
        // Note: since there are no results, we do not need to read any data
        // Deleting the cursor also frees up memory used
        delete MySQLCur;         
      }  

      // Disconnect from server
      MySQLCon.close();
      WriteSystemLog(MSG_INFO,F("SQL server disconnected"));
    }       
    else {
      WriteSystemLog(MSG_WARNING,F("SQL query too long"));
    }     
  }
  
  String ReplaceNAN(float x, String repVal)
  // Replaces NAN, repVal and converts to String
  {
    if (isnan(x)) {
       return repVal;
    }
    else {
      return String(x);
    }
  }
#endif
