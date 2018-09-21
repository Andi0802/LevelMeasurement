# Level Measurement
Level measurement for rain water reservoir with Arduino Mega2560

<h1>Features</h1>
<ul><li>Measurement of level in rain water reservoir by distance measurement<br> 
        Distance from sensor to water level is measured multiple times once per hour.<br>
        The average is used for further calculations of remaining volume.<br>
        Additionally standard deviation and sensor diagnostic data are used to calculate signal health (0=bad, 100=good)<br>        
  </li>
  <li>Control of refilling vent below dedicated level<br>
      For systems with a refilling possibility into the reservoir, are filling vent is actuated below a given water level.  
      The vent is actuated for a given volume which is converted to a actuation time. After refilling is completed, a new measurement is  started. Refilling can started by pressing button as well.    
  </li>
  <li>Calculation of used volume per day</li>
  <li>Diagnosis of intake filter clogging with external rain sensor</li>
  <li>Web server to enter settings</li>
  <li>Provides values to Smart-Home Server (HOMEMATIC)</li>  
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

<h1>Used libraries</h1>
NTPClient  https://github.com/arduino-libraries/NTPClient.git Commit 020aaf8
TimeLib    https://github.com/PaulStoffregen/Time.git Version 1.5+Commit 01083f8
