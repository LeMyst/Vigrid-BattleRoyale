#ifdef SERVER
//--- Optional bridge to the dzhooks native hook library (lemyst.github.io/dayz-serverkit/composer).
//---
//--- Its `scriptlock` and `adminbypass` modules each run a side thread that polls a sentinel file
//--- every 0.5 s and reads only its FIRST character: '1' locks, '0' unlocks, anything else is
//--- ignored. This class writes that file, so the match locks the server natively at the moment
//--- the countdown starts - the same moment the Vigrid API and autolock webhooks lock it - with
//--- no RCon password leaving the box and no remote API in the loop.
//---
//--- Without the hook nothing reads the file and writing it costs nothing, which is why this
//--- needs no #ifdef, no companion mod and no requiredAddons entry. The operator opts in on the
//--- hook side, by pointing the module at this file:
//---     DZHOOK_LOCKFILE=<profiles>\Vigrid-BattleRoyale\server_lock.txt          (scriptlock)
//---     DZHOOK_ADMINBYPASS=1 DZHOOK_ADMINBYPASS_FILE=<same path>               (adminbypass)
//---
//--- Three properties the call sites rely on:
//---   - A truncate-then-write can be observed half-done, but an empty file reads as EOF, which the
//---     hook ignores rather than treating as an unlock. So there is no torn state to guard against.
//---   - The file OUTLIVES the process. A match ends in RequestExit with '1' still in it, and a crash
//---     mid-match leaves it there too, so the next boot MUST write '0' or that server stays locked
//---     forever. BattleRoyaleServer.Init does exactly that, and it is the first thing that runs
//---     once the mission is up, before anyone can have connected.
//---   - Nothing unlocks at the end of a match on purpose: the server is about to exit, and
//---     unlocking would let players connect into the restart.
class BattleRoyaleNativeLock
{
	static string GetPath()
	{
		return BATTLEROYALE_SETTINGS_FOLDER + "server_lock.txt";
	}

	static void Set(bool locked)
	{
		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if ( !server_data.write_lock_file )
			return;

		//--- BattleRoyaleConfig creates this folder when it loads, but that is an ordering
		//--- assumption worth one no-op call not to depend on.
		MakeDirectory(BATTLEROYALE_SETTINGS_FOLDER);

		//--- Both lines below go through Print, not BattleRoyaleUtils. A retail server runs at
		//--- BATTLEROYALE_LOG_LEVEL 0, which filters Warn and Info alike, and the only level it
		//--- keeps is Error, which is fatal (Error2 unwinds). So the hook's whole state change
		//--- would be invisible on exactly the servers it runs on - measured 2026-11-05: no
		//--- [NativeLock] line in a retail script_*.log while the hook logged every transition.
		//--- Two lines per match; the open failure is the one an operator most needs to see.
		FileHandle file = OpenFile(GetPath(), FileMode.WRITE);
		if ( file == 0 )
		{
			Print("[DayZ-BattleRoyale][NativeLock] cannot open " + GetPath() + " for writing");
			return;
		}

		string value = "0";
		if ( locked )
			value = "1";

		FPrint(file, value);
		CloseFile(file);

		Print("[DayZ-BattleRoyale][NativeLock] wrote " + value + " to " + GetPath());
	}
}
#endif
