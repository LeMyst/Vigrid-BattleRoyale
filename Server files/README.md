# Server Files

## Install

### Key words

SERVER ROOT | This is the root directory for your dayz server. This is where DayZServer_x64.exe is located


### Mods

To install the files you are going to need the following mods.
1. DayZBR
2. RPCFramework

### Instructions

This is for the use WITHOUT DayzSALauncher

1. Make sure your server is running 0.63 (Experimental) build
2. All of the mod folders should go into your SERVER ROOT. 
3. Inside each mod folder, you will find a Keys folder. In there you will see a .BIKEY file. Copy this file into the SERVER ROOT\Keys directory. 
4. Copy the dayzbr mission file to your server's MPMissions folder (found in SERVER ROOT)
5. Edit your serverDZ.cfg
	- Add  equalModRequired = 1;
	- Modify the mission template with template="dayzbr.chernarusplus";    
6. The BRZones folder must go inside your server's profile directory
7. To start your server choose your preferred batch file either Simple_Start_Server.bat or Start_Server.bat, make sure the batch files is located in the SERVER ROOT.
8. Simply Start the server by running your batch file.

This is for the use WITH DayzSALauncher

Coming Soon.