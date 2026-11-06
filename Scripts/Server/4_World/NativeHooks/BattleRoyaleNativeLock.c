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
//--- It drives the native lock two independent ways at the same two moments: the countdown starts
//--- (lock) and the server boots (unlock). Either is a no-op without the hook.
//---
//--- 1. THE SENTINEL FILE. The `scriptlock` and `adminbypass` modules each run a side thread that
//---    polls it every 0.5 s and reads only its FIRST character: '1' locks, '0' unlocks, anything
//---    else is ignored. Without the hook nothing reads the file and writing it costs nothing, so it
//---    needs no #ifdef. The operator opts in by pointing one module at it:
//---        DZHOOK_LOCKFILE=<profiles>\Vigrid-BattleRoyale\server_lock.txt          (scriptlock)
//---        DZHOOK_ADMINBYPASS=1 DZHOOK_ADMINBYPASS_FILE=<same path>               (adminbypass)
//---    `write_lock_file` in server_settings.json turns these writes off.
//---
//--- 2. THE `serverlock` NATIVES, when the operator sets DZHOOK_SERVERLOCK=1. They set the same
//---    engine lock bit `scriptlock` does, synchronously, with no file and no path to configure, and
//---    the bit dies with the process - so a crash cannot leave the next boot locked.
//---    ⚠️ Do NOT enable it together with `adminbypass`. That module locks through the whitelist and
//---    leaves the lock bit clear; ServerLock() sets the bit, which the engine tests BEFORE the
//---    whitelist, so the admins adminbypass exists to let in would be refused.
//---
//--- Either way the match locks the server natively at the moment the countdown starts - the same
//--- moment the Vigrid API and autolock webhooks lock it - with no RCon password leaving the box and
//--- no remote API in the loop.
//---
//--- Three properties the call sites rely on:
//---   - A truncate-then-write can be observed half-done, but an empty file reads as EOF, which the
//---     hook ignores rather than treating as an unlock. So there is no torn state to guard against.
//---   - The file OUTLIVES the process. A match ends in RequestExit with '1' still in it, and a crash
//---     mid-match leaves it there too, so the next boot MUST write '0' or that server stays locked
//---     forever. BattleRoyaleServer.Init does exactly that, and it is the first thing that runs
//---     once the mission is up, before anyone can have connected. (The native bit needs no such
//---     care, but the boot unlock is sent to it too, so both paths stay symmetric.)
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
		SetNative(locked);
		WriteLockFile(locked);
	}

	//--- `write_lock_file` deliberately does NOT gate this: that switch is about the file, and the
	//--- natives only exist when the operator opted in with DZHOOK_SERVERLOCK=1.
	static void SetNative(bool locked)
	{
#ifdef DZHOOK_SERVERLOCK
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
#endif
	}

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
