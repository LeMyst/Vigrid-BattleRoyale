#ifdef SERVER
//--- The dzhooks `serverlock` module binds these two natives, and its `scriptdefine` module injects
//--- DZHOOK_SERVERLOCK into every script module when it does - so the guard is true exactly when the
//--- natives exist. An unbound `proto native` is a compile failure, which is why the declarations
//--- and the calls sit behind the same guard and nothing names them anywhere else. Without the hook
//--- (or with DZHOOK_SERVERLOCK unset on the hook side) both compile out.
#ifdef DZHOOK_SERVERLOCK
proto native void ServerLock();
proto native void ServerUnlock();
#endif

//--- Optional bridge to the dzhooks native hook library (lemyst.github.io/dayz-serverkit/composer).
//--- It drives the native lock at two moments: the countdown starts (lock) and the server boots
//--- (unlock). It picks ONE of two backends at compile time, and either is a no-op without the hook.
//---
//--- 1. THE `serverlock` NATIVES - preferred, used whenever the hook provides them (on by default
//---    since serverkit 8921f20, DZHOOK_SERVERLOCK=0 turns them off; older DLLs needed
//---    DZHOOK_SERVERLOCK=1). They set engine lock bit 0x04, the same one `scriptlock` sets, but
//---    synchronously, with no file and no path to configure, and the bit dies with the process - so
//---    a crash cannot leave the next boot locked. The file is then NOT written on lock, so
//---    `scriptlock` and `adminbypass` never see a '1'.
//---    ⚠️ Do NOT enable it together with `adminbypass`. That module locks through the whitelist and
//---    leaves the lock bit clear; ServerLock() sets the bit, which the engine tests BEFORE the
//---    whitelist, so the admins adminbypass exists to let in would be refused.
//---
//--- 2. THE SENTINEL FILE - the fallback, without DZHOOK_SERVERLOCK. The `scriptlock` and
//---    `adminbypass` modules each run a side thread that polls it every 0.5 s and reads only its
//---    FIRST character: '1' locks, '0' unlocks, anything else is ignored. Without the hook nothing
//---    reads the file and writing it costs nothing. The operator opts in by pointing one module at it:
//---        DZHOOK_LOCKFILE=<profiles>\Vigrid-BattleRoyale\server_lock.txt          (scriptlock)
//---        DZHOOK_ADMINBYPASS=1 DZHOOK_ADMINBYPASS_FILE=<same path>               (adminbypass)
//---    `write_lock_file` in server_settings.json turns these writes off.
//---
//--- Either way the match locks the server natively at the moment the countdown starts - the same
//--- moment the Vigrid API and autolock webhooks lock it - with no RCon password leaving the box and
//--- no remote API in the loop.
//---
//--- Four properties the call sites rely on:
//---   - A truncate-then-write can be observed half-done, but an empty file reads as EOF, which the
//---     hook ignores rather than treating as an unlock. So there is no torn state to guard against.
//---   - The file OUTLIVES the process. A match ends in RequestExit with '1' still in it, and a crash
//---     mid-match leaves it there too, so the next boot MUST write '0' or that server stays locked
//---     forever. BattleRoyaleServer.Init does exactly that, and it is the first thing that runs
//---     once the mission is up, before anyone can have connected.
//---   - That boot '0' is still written with the natives in use, when the file already exists. A
//---     server switched from the file to `serverlock` may keep DZHOOK_LOCKFILE set and a '1' left
//---     by its last file-driven match, and nothing else would ever clear it.
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
#ifdef DZHOOK_SERVERLOCK
		SetNative(locked);

		//--- Only to clear a stale '1' (see the header). Never lock through the file as well: the
		//--- natives are the backend, and a '1' would also be read by an `adminbypass` left watching it.
		if ( !locked && FileExist(GetPath()) )
			WriteLockFile(false);
#else
		WriteLockFile(locked);
#endif
	}

#ifdef DZHOOK_SERVERLOCK
	//--- `write_lock_file` deliberately does NOT gate this: that switch is about the file, and the
	//--- hook-side off switch for the natives is DZHOOK_SERVERLOCK=0.
	static void SetNative(bool locked)
	{
		if ( locked )
		{
			ServerLock();
			Print("[DayZ-BattleRoyale][NativeLock] ServerLock() called");
		}
		else
		{
			ServerUnlock();
			Print("[DayZ-BattleRoyale][NativeLock] ServerUnlock() called");
		}
	}
#endif

	static void WriteLockFile(bool locked)
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
