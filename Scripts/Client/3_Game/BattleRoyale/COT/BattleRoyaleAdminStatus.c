/**
 *  What the COT admin panel shows: one snapshot of the match, assembled server-side and answered to
 *  one admin.
 *
 *  UNGUARDED on purpose - the server fills it and the client renders it, so both sides need the
 *  type. Under Scripts/Client/ with no #ifdef, which is this repo's idiom for shared code (see
 *  BattleRoyaleConstants.c, BattleRoyaleUtils.c).
 *
 *  ⚠️ IT GOES ON THE WIRE AS ONE JSON STRING, not as a sequence of ScriptRPC writes, and that is a
 *  deliberate trade rather than laziness. Sequential rpc.Write / ctx.Read pairs couple the two sides
 *  by ORDER: insert a field in the middle of the writer and every read after it silently returns the
 *  wrong value - no error, no log line, just plausible-looking wrong numbers, which is the worst
 *  failure texture this repo keeps running into. JSON removes the ordering contract entirely, and it
 *  buys forward and backward compatibility for free: a key the reader does not know is ignored, and
 *  a key the writer did not send leaves the field initialiser below in place. That is the same
 *  property BattleRoyaleDataBase.Upgrade() already relies on for scalars.
 *
 *  The cost is one small parse per push. It is sent at BR_ADMIN_STATUS_INTERVAL_MS to the handful of
 *  admins who have the panel open, so it is not worth optimising.
 *
 *  ⚠️ NOTHING IDENTIFYING BELONGS IN HERE. No SteamID64s, no per-player rows, no positions. The
 *  reply is per-identity rather than broadcast, but the rule that keeps that safe is that the
 *  payload stays aggregate. Per-player data is #301 and #304, and both carry their own reasoning
 *  about who may receive a uid.
 *
 *  Every field has an initialiser, so a default-constructed instance is a valid "nothing known yet"
 *  snapshot and the renderer never has to special-case a NULL.
 */
class BattleRoyaleAdminStatus
{
    //--- The state machine -----------------------------------------------------------------------
    //--- Indices are NOT fixed: spawn selection is conditional on enable_spawn_selection_menu and
    //--- the rounds are a loop over num_zones, so the panel shows position-in-list rather than
    //--- assuming a numbering.
    string state_name        = "";
    int    state_index       = -1;
    int    state_count       = 0;
    bool   state_paused      = false;

    //--- Population ------------------------------------------------------------------------------
    int    players_alive     = 0;   //!< the roster the current state holds
    int    connected         = 0;   //!< everyone on the server, including admins and spectators
    int    spectators        = 0;

    //--- ⚠️ BR_ADMIN_GROUPS_UNKNOWN when the party manager is not ready. #ifdef VIGRID_PARTY says
    //--- the addon is COMPILED IN, not that parties are in play - with the manager disabled
    //--- GetGroupCount() returns one group per player, i.e. a number always identical to
    //--- players_alive. That degradation is correct for the logic tests and wrong to DISPLAY, so the
    //--- filler asks VigridPartyAPI.IsReady() first and the renderer hides the field on the
    //--- sentinel. This is the #158 mechanism.
    int    groups_alive      = BR_ADMIN_GROUPS_UNKNOWN;

    //--- The circle ------------------------------------------------------------------------------
    int    countdown_ms      = BR_COUNTDOWN_NONE;
    bool   zone_locked       = false;
    float  current_radius    = 0;   //!< 0 = no damaging circle yet, which is normal until LockNewZone
    float  future_radius     = 0;   //!< 0 = no circle announced yet
    int    generation_seed   = 0;   //!< so an operator can replay a layout they did not like

    //--- The lobby start gate --------------------------------------------------------------------
    //--- Only meaningful while lobby_phase. This is the readout that answers "why hasn't it
    //--- started?", which until now existed only as BR_DiagLogGate behind DIAG_DEVELOPER - i.e.
    //--- unreachable on the live servers where the question actually gets asked.
    bool   lobby_phase       = false;
    bool   lobby_held        = false;  //!< an admin is holding it open - see BattleRoyaleDebug.b_ManualStart
    int    lobby_players     = 0;      //!< raw roster, which is what "full" is measured against
    int    lobby_loaded      = 0;      //!< what minimum_players is actually measured against
    int    lobby_not_loaded  = 0;
    int    lobby_minimum     = 0;
    int    lobby_uptime_s    = 0;
    int    lobby_min_wait_s  = 0;
    bool   lobby_full        = false;
    bool   lobby_notfull_ok  = false;
    int    lobby_notfull_left_s = 0;
    bool   lobby_vote_system = false;
    int    lobby_ready_count = 0;
    bool   lobby_load_hold   = false;  //!< a load hold is running, i.e. the start is being delayed

    //--- Would BR_AdminStartMatch() accept right now? False means the group-count floor would refuse
    //--- it: IsOneSideLeft() is the win condition, so a match force-started with a single group ends
    //--- on its first tick. The panel disables Start Now on this rather than letting the operator
    //--- press a button that produces an instantly-finished match.
    bool   lobby_can_start_now = false;
}
