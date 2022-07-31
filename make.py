3## Compiler menu for Arduino
import os
import subprocess
import shutil
import re
import serial.tools.list_ports
import serial.tools.miniterm
import serial
	  
print("--- Available configurations -------------------------------")
dirname = './config'
filenames = os.listdir(dirname)
# print path to all filenames.
idx=0
listConfigs=[]
for filename in filenames:
    idx=idx+1
    print(idx,": ",filename)            
    with open(os.path.join(dirname,filename)) as f:
        lines = f.readlines(1)
        print(lines[0].replace("//","").strip(),"\n") 				
        listConfigs.append(lines)
		  
choice = input("Enter your choice: ")
sel_file = os.path.join(dirname,filenames[int(choice)-1])
sel_cfg = listConfigs[int(choice)-1]
print(sel_cfg[0])

#Copy config file
print('Copy ',sel_file,' to config.h')
shutil.copy(sel_file,'config.h')
print('\n')

#Create header with git-version
print("Finding git version\n")
GITPATH = "C:/Users/Andreas/AppData/Local/Atlassian/SourceTree/git_local/mingw32/bin"
Version = subprocess.run(GITPATH+"/git describe --tags",stdout=subprocess.PIPE,universal_newlines=True)
VersInfo = subprocess.run(GITPATH+"/git log --pretty=full -n 1",stdout=subprocess.PIPE,universal_newlines=True)
Version = Version.stdout.strip()
VersionInfo = VersInfo.stdout.strip()

print("Git Version: ",Version,'\n')
print("Commit comment:\n",VersionInfo,'\n')

#Open file version_info.has_key
file = open("version_info.h","w")
file.write("//Version info for LevelMeasurement.ino\n")
file.write("#define PRG_VERS \""+Version+"\"\n")
file.write("#define PRG_CHANGE_DESC \""+ VersionInfo.replace("\n","<br>")+"\"\n")
file.write("#define PRG_CFG \""+sel_cfg[0].replace("// ","").strip()+"\"\n")
file.close()

#Searching for Arduino Ports
print("Searching for Arduino COM ports\n")
serList = serial.tools.list_ports.comports()
for device, desc, hwid in sorted(serList):
    if re.search("Arduino",desc):
        print("{}: {} [{}]".format(device, desc, hwid))
        portFound = device
		
#Compile and upload
if not portFound:
    print("No COM-port found - upload not possible")
else:	
    print("\nCompile and upload")
    result = subprocess.run("C:/Program Files (x86)/Arduino/arduino.exe LevelMeasurement.ino --upload --port "+portFound,stdout=subprocess.PIPE,universal_newlines=True)
    print(result.stdout)	
	
    #Start Terminal
    ser = serial.Serial(portFound, 115200, timeout=1)	
    while(True):
        line = ser.readline()
        #line=line.replace("b'","")
        #line=line.rstrip("\r\n'")
        try:
            line=line.decode()
        except:
            line=line
        if (len(line)>0):
            print(line)
