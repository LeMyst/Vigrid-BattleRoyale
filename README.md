# DayZ Battle Royale
-------

### Hello Survivors,
-----
Today we are bringing you a new game mode for DayZ. DayZ Battle Royale. When most of you think of the Battle Royale experience, we hope that you think back to the ArmA 3 mod and the experience of a simulated last man standing game. This will be an unforgiving last man standing death match. Making one mistake could easily lead to a game over. 

Just to make it clear from the start, the game is still in its early alpha stage. For our Alpha 1.0 launch, we wanted to keep the play area small. This keeps the core of the code smaller, reduces the performance impact from our lack of experience with the new Modding systems, and allows us to tailor the experience to fit more easily. For these reasons, the play area will be within a 500m radius around cherno. 

DayZ Battle Royale will be just as harsh as DayZ is. Players will still need to eat and drink at the same rate, stamina will not be reduced, the Zone will do 10 damage every 2 seconds, and bullet damage will not be modified. Custom loot spawns have been implemented throughout the city. These can spawn piles of food, water, ammo, weapons, and gear. Deciding what to loot, when to loot, who and when to fight will all be key aspects of the game. At any point, a wrong decision could lead to your untimely death. 

To simplify the player spawning, players will spawn in a circle around a pile of backpacks. Their will only be a few of these bags, but they will contain some decent items to give you a head start in the first couple of minutes. A timer will count down and the round will begin. Game play does not stop until only one player remains. 

As stated, we are still in early alpha. Alpha 1.0 will be released when the last critical bugs are resolved. We aim to have a release sometime in the next week.

#### Thanks,

Kegan - Lead Dev
 
  
## Download & Setup for Pre-Alpha v 0.92 (Updated on Thursday, October 4th, 2018)
-----
### Download link for packed files
[Download from our website](http://lystic.net/DayZBR/dayzbr.rar)

### Server setup.
NOTE: I RECOMMEND YOU USE [DZSA LAUNCHER](https://www.dayzsalauncher.com/#/tools) WITH YOUR SERVER.
- Fresh install dayz 0.63 or greater server
- copy dayzbr folder into the server root
- move dayzbr/Keys/dayzbr.key to the DayZServer\Keys folder for your server
- Run server with *-mod=dayzbr*
- Lastly, Open serverDZ.cfg and add *equalModRequired = 1;* 

After you run the server for the first time, a config will be created. It is located in your server profile directory in a file called BRData.json
This contains information for battle royale. Minimum players, start delay, and more.


### Client setup - DZSA Launcher
- MAKE SURE YOU HAVE THE EXPERIMENTAL BUILD OF DAYZ

To play Battle Royale. I Strongly Recommend you use [DZSA Launcher](https://www.dayzsalauncher.com/#/home)

If you choose to install the files by hand, note that you will not be able to use DZSA Launcher. 

- copy dayzbr folder into  steamapps\common\DayZ
- Add *-mod=dayzbr* to your DayZ launch parameters
 
  
## Current Development Status
------

The current files are considered **UNSTABLE**. If you wish to test out this unstable build, you can download the files above or use the DZSA Launcher. When we reach Alpha 1.0, detailed hosting and client installation instructions will be provided.

If you are looking to host a server. I recommend using dzsa launcher and telling players to, not install the mod files, but just join through the launcher. It will handle downloading the mod files. You can read more from the links above.

Sorry, but we expect Alpha 1.0 to be done by the end of this first week. You can follow development from the Issues board.