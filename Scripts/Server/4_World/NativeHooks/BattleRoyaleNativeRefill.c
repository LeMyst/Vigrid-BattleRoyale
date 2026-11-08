#ifdef SERVER
//--- The dzhooks `ce_refill` module (serverkit #29, PR #30, merged as 7c13ada) binds these two
//--- natives, and its `scriptdefine` sibling injects DZHOOK_CE_REFILL when it does - so the guard is
//--- true exactly when the natives exist. Same shape as DZHOOK_SERVERLOCK in BattleRoyaleNativeLock:
//--- the declarations and every call sit behind the guard, because calling an unbound `proto native`
//--- raises a VM exception ("not linked") rather than failing the build.
//---
//--- What they do: re-run the Central Economy's boot-time loot pass (WRSpawnerLoot::OnInitial) on
//--- demand. Each loot type is topped up to InitialSpawn % (CE_RefillLoot) or `percent` % of its
//--- nominal, counted from what is already on the map, so nothing is created on a full map and a
//--- repeat call is harmless. Both return the number of items created, or -1 if nothing ran (the
//--- `[ce_refill]` line in dzhooks_win.log says why).
//---
//--- Measured on ChernarusPlus (serverkit #29): ~19,300 items in 8.4 s on DayZDiag and ~14,600 in
//--- 4.3 s on retail, the server frozen meanwhile - hence the steps, one per frame. Nothing is placed
//--- within LootSpawnAvoidance (100 m stock) of a player, and nothing within CleanupAvoidance of a
//--- player is ever expired, which is why the refill only runs while everyone is in the lobby.
#ifdef DZHOOK_CE_REFILL
proto native int CE_RefillLoot();
proto native int CE_RefillLootTo(int percent);
#endif

class BattleRoyaleNativeRefill
{
	//! True when this build was compiled against a hook that binds the refill natives.
	static bool IsAvailable()
	{
#ifdef DZHOOK_CE_REFILL
		return true;
#else
		return false;
#endif
	}

	//! Top every loot type up to `percent` % of nominal. -1 when nothing ran or the hook is absent.
	static int RefillTo(int percent)
	{
#ifdef DZHOOK_CE_REFILL
		return CE_RefillLootTo( percent );
#else
		return -1;
#endif
	}

	//! Top every loot type up to InitialSpawn % of nominal. -1 when nothing ran or the hook is absent.
	static int Refill()
	{
#ifdef DZHOOK_CE_REFILL
		return CE_RefillLoot();
#else
		return -1;
#endif
	}

	static void LogMode()
	{
		//--- Print, not BattleRoyaleUtils: a retail server filters every level but the fatal one.
#ifdef DZHOOK_CE_REFILL
		Print("[DayZ-BattleRoyale][NativeRefill] DZHOOK_CE_REFILL set: the map loot can be refilled between matches");
#endif
	}
}
#endif
