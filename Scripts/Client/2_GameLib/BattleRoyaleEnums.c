//#ifdef JM_COT
enum BattleRoyaleCOTStateMachineRPC
{
    INVALID = 11000, //Do not move this

    Next,
    Pause,
    Resume,
    AddFakePlayer,
    AddFakeGroup,
    SpawnAirdrop,
    SpawnHorde,
    SpawnChemicals,

    COUNT //Do not move this
}
//#endif


/**
 *  Server-side debug actions, carried as param1 of the single "BRDiagAction" RPC.
 *
 *  Not guarded: an enum costs nothing in a retail build, and guarding it would mean guarding every
 *  mention of it. The RPC registration, its handler and every sender ARE guarded on DIAG_DEVELOPER.
 *
 *  Append only - the value travels on the wire, so renumbering desyncs a client that was not
 *  rebuilt alongside the server.
 */
enum BattleRoyaleDiagAction
{
    INVALID = 12000, //Do not move this

    SKIP_STATE,        //!< advance one state now
    SET_PAUSED,        //!< arg_i 0/1 - Pause()/Resume() the current state
    GOTO_STATE,        //!< arg_i = target index; fast-forwards, running every transition on the way
    FORCE_READY_ALL,   //!< ready-up every lobby player, so the real countdown path runs
    LOG_STATE,         //!< dump index / name / player counts to the log and to chat

    TP_ZONE_CENTER,    //!< teleport the sender to the live circle's centre
    TP_NEXT_ZONE,      //!< ... to the incoming circle's centre
    TP_LOBBY,          //!< ... to the lobby centre
    FORCE_UNSTUCK,     //!< unstuck ignoring AllowsUnstuck() and the cooldown

    LOG_ZONE_TABLE,    //!< dump every generated play area, in generation order
    CLEAR_MAP_MARKERS, //!< VigridMapAPI.ClearAllMarkers()

    SET_LOG_LEVEL,     //!< arg_i = BattleRoyaleUtils level, or -1 to stop overriding
    SET_CHAT_MIRROR,   //!< arg_i 0/1 - gate the ChatLog RPC
    SET_TRACE_TP,      //!< arg_i 0/1 = on/off, arg_f = tick budget

    KILL_SELF,         //!< kill the sender outright, to reach the death -> spectate path
    LOG_SPECTATORS,    //!< dump the spectator table, with the resolved chain tier per entry
    SET_SPECTATE,      //!< arg_i 0/1 - flip spectate_enabled for this process only
    SPECTATE_TP_TARGET,//!< arg_i = metres; fling the sender's watched target that far from their corpse
    SPECTATE_TP_CORPSE,//!< move the sender's OWN corpse to their watched target - the bubble probe
    SET_ADMIN_SPECTATE,//!< arg_i 0/1 - flip admin_spectate_enabled for this process only

    SET_FAKE_UNLOADED, //!< arg_i 0/1 - mark every lobby player un/loaded, to hold the #8 load gate open
    LOG_LOBBY_GATE,    //!< dump every term of both lobby start gates, i.e. why it is or is not starting

    SET_TRACE_AIM,     //!< arg_i 0/1 - server-side aim-angle trace. See PlayerBase.BR_LogAimState.

    COUNT //Do not move this
}


/**
 *  What killed a player, as recorded on their death record and rendered by the death recap.
 *
 *  Not guarded, and deliberately NOT tied to KillFeedCause. Extra/KillFeed/ may not be named without
 *  #ifdef KILLFEED and can be deleted from the build entirely, so pinning the two enums together
 *  would make the recap depend on an optional addon. The values are numerically independent on
 *  purpose - do not "align" them.
 *
 *  Append only - the value travels on the wire and is written into last_match.json, so renumbering
 *  mislabels every death in a file written by an older build.
 */
enum BattleRoyaleKillCause
{
    UNKNOWN = 0,   //!< the degraded disconnect path: a killer is known, the how is not
    NONE,          //!< never eliminated. The WINNER - renders "You were never eliminated."
    FIREARM,
    MELEE,
    BAREHANDS,
    EXPLOSIVE,     //!< grenade, mine, claymore, IED - attributed to whoever armed it
    ZONE,          //!< the play area. Known only through the hint set at the two damage sites.
    INFECTED,
    ANIMAL,
    ENVIRONMENT    //!< fall, drowning, exposure, a building - nobody is responsible
}


enum BattleRoyaleMatchMakingState
{
	INVALID = -1, //Do not move this

	None,
	Searching,
	Connecting,
	Failed,
	Success
}