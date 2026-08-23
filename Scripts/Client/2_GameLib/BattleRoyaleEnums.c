/**
 *  Admin panel wire ids. Carried by ScriptRPC and dispatched by BRMasterControlsModule.OnRPC
 *  through the window its GetRPCMin() / GetRPCMax() declare.
 *
 *  Not guarded: an enum costs nothing in a retail build, and guarding it would mean guarding every
 *  mention of it. Same reasoning as BattleRoyaleDiagAction below.
 *
 *  ⚠️ APPEND ONLY - the value travels on the wire, so renumbering desyncs a client that was not
 *  rebuilt alongside the server.
 *
 *  The `Reserved*` slots are retired features whose senders and handlers have been deleted. They
 *  are kept rather than removed precisely BECAUSE of the append-only rule: dropping one renumbers
 *  every value after it. AddFakePlayer / AddFakeGroup moved to the diag menu (see PluginDiagMenu.c);
 *  SpawnHorde / SpawnChemicals never had a button or a server case at all, and were dead in three
 *  layers at once - enum value, module sender and form handler.
 */
enum BattleRoyaleCOTStateMachineRPC
{
    INVALID = 11000, //Do not move this

    Next,
    Pause,
    Resume,
    ReservedAddFakePlayer,   //!< RESERVED - do not reuse, do not remove
    ReservedAddFakeGroup,    //!< RESERVED - do not reuse, do not remove
    SpawnAirdrop,
    ReservedSpawnHorde,      //!< RESERVED - do not reuse, do not remove
    ReservedSpawnChemicals,  //!< RESERVED - do not reuse, do not remove

    //--- One id for every state-changing admin action; which action rides in the payload as a
    //--- BattleRoyaleAdminAction. Same shape as BRDiagAction and for the same reason: a new action
    //--- then costs an enum value and a switch case rather than a wire change.
    AdminAction,

    StatusRequest,  //!< client -> server, no payload. Answered per identity, never broadcast.
    StatusReply,    //!< server -> client, one serialized BattleRoyaleAdminStatus.

    //--- The per-player table. Its own request/reply pair rather than riding StatusReply, because it
    //--- is an order of magnitude larger and is only wanted while a panel is actually showing it -
    //--- a separate id is what lets the client not ask.
    RosterRequest,
    RosterReply,    //!< server -> client, one serialized BattleRoyaleAdminRoster. CARRIES UIDS.

    //--- The generated circle table. Static for the whole process once generation has run, so the
    //--- client asks once and caches rather than polling.
    ZoneRequest,
    ZoneReply,      //!< server -> client, one serialized BattleRoyaleAdminZoneTable.

    COUNT //Do not move this
}


/**
 *  What a BattleRoyaleCOTStateMachineRPC.AdminAction is asking the server to do.
 *
 *  Append only, for the same reason as the enum above - the value is written into the AdminAction
 *  payload and therefore travels on the wire.
 *
 *  Every one of these is authorized server-side by BRMasterControlsModule.AuthorizeAdminAction
 *  against a named COT permission plus the admins_steamid64 floor. The client-side permission check
 *  only decides whether the control is drawn.
 */
enum BattleRoyaleAdminAction
{
    INVALID = 13000, //Do not move this

    LOBBY_SET_HOLD,   //!< arg_i 0/1 - hold the lobby open, or release it
    LOBBY_START_NOW,  //!< start the match now, bypassing every start gate but the group-count floor

    //--- Player operations. arg_uid names the SUBJECT; the ACTOR is always resolved from `sender`,
    //--- never from the payload - a client may not name who it is acting as.
    PLAYER_READY,            //!< force one lobby player to ready up
    PLAYER_READY_ALL,        //!< every lobby player; ignores arg_uid
    PLAYER_REMOVE,           //!< drop them from the match roster WITHOUT disconnecting them
    PLAYER_UNSTUCK,          //!< ignoring the cooldown and AllowsUnstuck()
    PLAYER_TP_CIRCLE,        //!< to the circle currently in play, lobby centre before that
    PLAYER_EXEMPT_LATEJOIN,  //!< cancel a pending late-join kick and exempt them from further ones

    //--- Announcements. arg_text is the message, arg_f how long it stays on screen.
    ANNOUNCE_ALL,            //!< ignores arg_uid
    ANNOUNCE_PLAYER,         //!< whispered to arg_uid

    //--- Zone pacing.
    ZONE_LOCK_NOW,           //!< lock the incoming circle immediately, ending the travel window early
    ZONE_SELFTEST,           //!< arg_i = iterations; runs the generator acceptance harness and logs it

    //--- Appended rather than filed next to PLAYER_REMOVE, which is where it belongs by meaning.
    //--- These values travel on the wire, so inserting one renumbers every value after it - and the
    //--- append-only rule is not worth bending for tidiness.
    PLAYER_ADD,              //!< put a removed player back on the roster - the undo for PLAYER_REMOVE

    COUNT //Do not move this
}


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

    SET_CARRY_ENABLED,   //!< arg_i 0/1 - run the corpse carry at all. The A/B that PRICES it: off,
                         //!< CarryCorpse returns on its first line and the whole evaluation - both
                         //!< walks and both allocations - is skipped, so the difference in the load
                         //!< line's tickms between two windows IS the carry's cost.
    SET_CARRY_STAGGER,   //!< arg_i 0/1 - per-entry carry phase vs the one global clock (#280)
    LOG_CARRY_LOAD,      //!< report the carry-load counters NOW and reset the window, so a
                         //!< measurement run can be bracketed exactly rather than waiting out the
                         //!< 10 s throttle and catching part of the previous condition
    SPECTATE_BENCH_CARRY,//!< arg_i = bodies to project to; price the carry pass at a full lobby

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