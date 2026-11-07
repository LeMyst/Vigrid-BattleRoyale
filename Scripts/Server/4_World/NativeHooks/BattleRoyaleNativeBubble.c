#ifdef SERVER
//--- Optional bridge to the dzhooks `spectatorbubble` module (serverkit issue #22, PR #24 - not merged
//--- yet). Tested working on the diag rig 2026-11-07: a target stayed replicated out to 2590 m from the
//--- corpse. Docs/native-hooks.md has the run.
//---
//--- WHY A SPECTATOR'S BUBBLE STAYS ON THEIR CORPSE without it (decoded on the 1.29 and 1.30 Linux
//--- servers, serverkit #22). The server keeps one record per connection, whose bubble centre is
//--- what replication is computed around. The camera's UpdateSpectatorPosition push DOES reach the
//--- server and IS stored there, once SelectSpectator has flagged the connection as a spectator. But
//--- every tick, just before the bubble is recomputed, the server overwrites that centre with the
//--- position of the connection's person entity - and SelectPlayer(identity, NULL) never detaches
//--- that entity (with NULL it makes no network call at all). A dead PlayerBase is still a Person,
//--- so the corpse always wins. That is the whole ~1 km limit in Docs/spectating.md.
//---
//--- The module skips that overwrite while the connection is spectating, so the bubble follows the
//--- camera, and registers DZHOOK_SPECTATORBUBBLE through its `scriptdefine` sibling when it installs.
//--- The guard is therefore true exactly when the bubble follows the camera, and it is the only
//--- thing this class tests. No native is bound, so there is nothing to keep in step with the hook.
//---
//--- What the mod changes when it is set:
//---   - CarryCorpse does nothing. The corpse stays where the player fell, with its gear on it, and
//---     stays lootable like any other body - which is the whole point.
//---   - CarryAnchorBody is KEPT. An admin's live body is still their person entity, and the hook no
//---     longer re-centres the bubble on it. Carrying it under the camera keeps it at the bubble's
//---     centre, so the server never tries to drop the admin's own controlled entity out of their
//---     bubble. The admin path is not tested with the hook yet; keeping the carry is the
//---     conservative half.
//---
//--- Without the hook the define is absent and both carries run exactly as before.
class BattleRoyaleNativeBubble
{
	//! True when the dzhooks module makes the bubble follow the spectator camera.
	static bool FollowsCamera()
	{
#ifdef DZHOOK_SPECTATORBUBBLE
		return true;
#else
		return false;
#endif
	}

	//! One line at boot naming the mode, through Print so it survives a retail server's log level
	//! (see BattleRoyaleNativeLock.WriteLockFile for why BattleRoyaleUtils would hide it).
	static void LogMode()
	{
		if ( FollowsCamera() )
			Print("[DayZ-BattleRoyale][NativeBubble] DZHOOK_SPECTATORBUBBLE set: the bubble follows the spectator camera, corpses are not carried");
		else
			Print("[DayZ-BattleRoyale][NativeBubble] no spectatorbubble hook: corpses are carried to keep the bubble in range");
	}
}
#endif
