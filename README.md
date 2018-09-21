# Level Measurement
Level measurement for rain water reservoir with Arduino Mega2560

<h1>Features</h1>
<ul><li>Measurement of level in rain water reservoir by distance measurement<br> 
        Distance from sensor to water level is measured multiple times once per hour.<br>
        The average is used for further calculations of remaining volume.<br>
        Additionally standard deviation and sensor diagnostic data are used to calculate signal health (0=bad, 100=good)<br>
        An indication LED is flashing in green if no error is detected. The on-time of the LED in comparison to the period (2sec) reflects the relative volume in reservoir.
  </li>
  <li>Control of refilling vent below dedicated level<br>
      For systems with a refilling possibility into the reservoir, are filling vent is actuated below a given water level.  
      The vent is actuated for a given volume which is converted to a actuation time. After refilling is completed, a new measurement is  started. Refilling can started by pressing button as well.    
  </li>
  <li>Calculation of used volume per day<br>
       Used amount of water per day is calculated only if no rainis detected from rain sensor. Rain sensor signal is read from SmartHome Sensor (HOMEMATIC). Therfore number of datapoint is entered into program.<br>An average over the last 10 days is calculated as well.</li>
  <li>Diagnosis of intake filter clogging with external rain sensor<br>During days with enough rain level change in reservoir is compared with calulated level change determined by rain quantity and roof surface. If level change is too low in comparison to calculated level change an error is set and indicated in Webserver and SmartHome System.</li>
  <li>Web server to enter settings<br>
      A web server is provided to monitor actual values and to change settings in system. Settings are stored in EEPROM permanently.</li>        
  <li>Provides values to Smart-Home Server (HOMEMATIC)<br>
      Actual volume in Liter, relative volume in Percent, Signal Health, Error state, Filter conditions are send to SmartHome system. A system variable with a specific name and unit has to be provided in Homematic.  </li>  
  <li>Statistic and log data storage on SDCard</li></ul>

<h1>Dedicated Hardware</h1>
<ul><li>Arduino Mega2560</li>
  <li>Ethernet Shield </li>
  <li>US-Sensor JSN-SRD4T-2.0 (5V, GND, TRIG, ECHO)</li>
  <li>Multicolor indication LED red/green</li>
  <li>OptoTriac</li>
  <li>Push Button 5V</li>
  <li>Power Supply 5V</li>
</ul>

<h1>Configuration</h1>
<h2>Ethernet Settings</h2>
<h2>SmartHome (Homematic)</h2>
<h1>Folders</h1>

<h1>Used libraries</h1>
NTPClient  https://github.com/arduino-libraries/NTPClient.git Commit 020aaf8<br>
TimeLib    https://github.com/PaulStoffregen/Time.git Version 1.5+Commit 01083f8
