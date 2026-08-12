#ifdef DIAG_DEVELOPER
/**
 *  Battle Royale - the diag menu's state bag.
 *
 *  A bag of plain static fields, exactly like BattleRoyaleRPC: the diag callbacks store, they never
 *  act. Consumers poll these where they live - BattleRoyaleClient.Update for anything 5_Mission,
 *  PlayerBase for the teleport trace, BattleRoyaleUtils for the log level.
 *
 *  WHY A BAG AT ALL. The modded PluginDiagMenu / PluginDiagMenuClient must stay in 4_World, because
 *  that is where vanilla declares them and the four stages compile as separate ScriptModules. So a
 *  diag callback cannot touch KillFeedUI or BattleRoyaleClient directly. 3_Game is the lowest stage
 *  every party can see: the 4_World callbacks that write these, the 4_World PlayerBase that reads
 *  the trace flags, the 5_Mission client that polls the rest, and Scripts/Server at any stage.
 *
 *  NOT guarded on SERVER. The server needs the same class for the trace flags, the log level and
 *  the chat mirror - those are set by the BRDiagAction RPC handler over there. Client-only members
 *  sit in an inner #ifndef SERVER block, the two-sided shape VigridPartyAPI already uses.
 *
 *  The whole file is DIAG_DEVELOPER, so none of it exists in a retail build.
 */
class BattleRoyaleDiag
{
    //=========================================================================================
    //--- Shared. Written on the client by a callback, or on the server by the RPC handler.
    //=========================================================================================

    //! -1 means "no override, resolve normally". Otherwise one of BattleRoyaleUtils.NONE..TRACE.
    static int log_level_override = -1;

    //! Mirror server log lines into in-game chat over the ChatLog RPC. That mirror is otherwise
    //! unconditional on a DIAG server, which at trace level is most of the chat window.
    static bool chat_mirror = true;

    //! Log command/juncture state around a teleport. See PlayerBase.BR_LogTeleportState.
    static bool trace_teleport = false;

    //! How many CommandHandler ticks to keep logging for after a teleport juncture arrives.
    static int trace_teleport_ticks = 20;

#ifndef SERVER
    //=========================================================================================
    //--- Client only.
    //=========================================================================================

    //--- HUD. While hud_force is on, BattleRoyaleClient.Update drives every HUD element from these
    //--- instead of from BattleRoyaleRPC, so the whole HUD can be dressed with no match running.
    static bool hud_force = false;
    static int hud_players = 60;
    static int hud_groups = 20;
    static int hud_kills = 3;
    static int hud_countdown = 60;

    //--- Zones. Same idea: synthetic circles centred on the player, which feed the HUD distance
    //--- arrow and - through the single VigridMapAPI.SetZones call - the map's rings.
    static bool zones_fake = false;
    static float zone_radius = 1500;
    static float zone_next_radius = 600;

    //--- Kill feed. Read by the "Push Fake Kill" / "Fill Feed" callbacks at the moment they fire,
    //--- rather than mirroring the entry ids, so no id has to leave the plugin class.
    static int kf_cause = 0;
    static bool kf_with_weapon = true;

    //--- Party roster size to fabricate.
    static int party_size = 3;

    //--- Match Flow's "Jump To State" target. Held client-side and sent only when "Jump: Go" is
    //--- pressed: a range callback plausibly fires on every step while scrubbing, and nothing here
    //--- should be able to fire ten RPCs on the way to state 10.
    static int goto_state = 0;

    //--- One-shot requests. Monotonic counters, not bools - the consumer diffs against its own
    //--- last-seen value, so nobody owns the clear and two presses in one frame are not merged.
    static int req_open_spawn_menu = 0;
    static int req_open_leaderboard = 0;

    //! Zero everything. The plugin is destroyed and recreated on every world change, so this runs
    //! from RegisterModdedDiags() as well as from BattleRoyaleClient.Init().
    static void Reset()
    {
        log_level_override = -1;
        chat_mirror = true;
        trace_teleport = false;
        trace_teleport_ticks = 20;

        hud_force = false;
        hud_players = 60;
        hud_groups = 20;
        hud_kills = 3;
        hud_countdown = 60;

        zones_fake = false;
        zone_radius = 1500;
        zone_next_radius = 600;

        kf_cause = 0;
        kf_with_weapon = true;
        party_size = 3;

        goto_state = 0;

        req_open_spawn_menu = 0;
        req_open_leaderboard = 0;
    }

    /**
     *  Fire one server-side debug action.
     *
     *  One RPC carrying an action id, rather than one named RPC per action: adding an action is one
     *  enum value plus one case in the handler, with no new registration and no wire change. The
     *  float is present from day one on purpose - widening the payload later is a silent read
     *  failure on a client that was not rebuilt.
     *
     *  No target. The server resolves the subject from the RPC sender identity, the same way
     *  PlayerReadyUp and PlayerUnstuck do.
     */
    static void SendServerAction(int action, int arg_i, float arg_f)
    {
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "BRDiagAction", new Param3<int, int, float>( action, arg_i, arg_f ), true );
    }
#endif
}
#endif // DIAG_DEVELOPER
