// Functions to handle SQL client 

void ConnectSQL(void) {
  // Connects to SQL server
  WriteSystemLog(MSG_INFO,"Connection SQL server... ");
  if (MySQLCon.connect(sql_addr, SQL_PORT, SQL_USR, SQL_PWD)) {
    delay(1000);
    WriteSystemLog(MSG_INFO,"SQL server connected.");
  }
  else {
    WriteSystemLog(MSG_WARNING,"Connection SQL server failed. ");
  } 
}

void SendDataSQL(void) {
  // Sends data to SQL server 
  row_values *row = NULL;
  long head_count = 0;
  MySQL_Cursor *MySQLCur = new MySQL_Cursor(&MySQLCon);
  char chquery[1024]= "";
  
  // Query head
  String query = "INSERT INTO HomeData.ZISTERNE (Zeit, Level, Quantity, `Signal health`,Error,StDev,`Refill Quantity`,\
                  `Rain Quantity 24h`,`Refill quantity 1h`,`Measured quantity change 1h`,`Calculated quantity change 1h`,\
                  `Quantity difference`,`filter diagnosis`,`Rain quantity 1h`) VALUES (";
  
  // Get Date String in format for SQL Server: "YYYY-MM-DD HH:MM:SS"
  String DateTimeSQL = MeasTimeStr;

  // Build query
  query = query + "'" + DateTimeSQL + "'," + String(hWaterActual) + "," + String(volActual)+","+String(rSignalHealth) + ","+\
          String(stError)+"," + String(volStdDev)+"," + String(SettingsEEP.settings.volRefillTot) +","+String(volRain24h)+","+\
          String(volRefillDiag1h)+","+String(volDiffDiag1h)+","+String(volDiffCalc1h)+","+String(volDiff1h)+","+String(stFiltChkErr)+\
          ","+String(volRainDiag1h)+");";
  #if LOGLEVEL & LOGLVL_SQL
    WriteSystemLog(MSG_DEBUG,query);
  #endif
  
  // Execute the query
  query.toCharArray(chquery,query.length());
  if (MySQLCur->execute(chquery)) {
    // Note: since there are no results, we do not need to read any data
    // Deleting the cursor also frees up memory used
    delete MySQLCur;   
    WriteSystemLog(MSG_INFO,"SQL query: Success");
  }
  else {
    WriteSystemLog(MSG_INFO,"SQL Server not connected, reconnecting...");
    ConnectSQL();
    if (MySQLCur->execute(chquery)) {
      WriteSystemLog(MSG_INFO,"SQL query: Success");
    }
    else {
      WriteSystemLog(MSG_WARNING,"No SQL connection");
    }
    // Note: since there are no results, we do not need to read any data
    // Deleting the cursor also frees up memory used
    delete MySQLCur;     
  }
}

