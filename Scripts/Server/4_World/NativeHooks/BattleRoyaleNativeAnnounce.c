#ifdef SERVER
//--- The dzhooks `announce` module (serverkit #23, PR #26) binds these two natives, and its
//--- `scriptdefine` sibling injects DZHOOK_ANNOUNCE when it does - so the guard is true exactly when
//--- the natives exist. Same shape as DZHOOK_SERVERLOCK in BattleRoyaleNativeLock: an unbound
//--- `proto native` is a compile failure, so the declarations and every call sit behind the guard.
//---
//--- Unlike the serverlock natives these TAKE AN ARGUMENT. A `proto native` is called as a plain C
//--- function and a `string` arrives as a `const char*` (serverkit PROTO_NATIVE.md section 4).
#ifdef DZHOOK_ANNOUNCE
proto native void SetServerNamePrefix(string prefix);
proto native void SetServerNameSuffix(string suffix);
#endif

//--- Optional bridge: puts the match phase in the server-browser name, e.g.
//---     Vigrid BR #1 [LOBBY 6]   ->   [STARTING 0:30]   ->   [IN GAME 14 left]   ->   [ENDING]
//---
//--- What the hook changes, and what it does not: only Steam's copy of the name (A2S and the master
//--- list). The serverDZ.cfg hostname, the hive's name and the in-game name stay as configured. The
//--- hook cuts a long hostname on a UTF-8 boundary so the affixes stay whole; prefix and suffix
//--- share 48 bytes, and every suffix BattleRoyaleServer builds is well under 25.
//---
//--- The hook forces a Steam refresh on every change (at most once per second), so the caller must
//--- only hand over text that actually changed. SetSuffix dedups for it, which is what makes a 1 Hz
//--- caller cheap: a lobby that is not filling up costs one string compare per second.
//---
//--- Without the hook nothing is called and nothing is logged.
class BattleRoyaleNativeAnnounce
{
	protected static string s_Suffix;

	static void SetSuffix(string suffix)
	{
		if ( suffix == s_Suffix )
			return;

		s_Suffix = suffix;

#ifdef DZHOOK_ANNOUNCE
		SetServerNameSuffix( suffix );

		//--- Print, not BattleRoyaleUtils: a retail server filters every level but the fatal one
		//--- (see BattleRoyaleNativeLock). One line per phase change, a few per match.
		Print("[DayZ-BattleRoyale][NativeAnnounce] suffix '" + suffix + "'");
#endif
	}

	static void LogMode()
	{
#ifdef DZHOOK_ANNOUNCE
		Print("[DayZ-BattleRoyale][NativeAnnounce] DZHOOK_ANNOUNCE set: the match phase is shown in the server-browser name");
#endif
	}
}
#endif
