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

    //! Log each player instance's base aiming angles. The instrument that found the remote-ADS-
    //! pitch desync - note it measured that REMOTE instances run no script CommandHandler, so only
    //! the owner and the server ever produce lines. See PlayerBase.BR_LogAimState.
    static bool trace_aim = false;

    //! Seconds between "[Spectate] cam=..." samples. Vanilla-ish 5 s is right for a background
    //! sanity check and far too coarse for a deliberate range test, where the interesting window is
    //! the 20-30 s after the target is flung out - 5 samples is not enough to tell a sustained
    //! entity=0 from the transient one every teleport produces. Client-side, read by the camera.
    static float spectate_trace_interval = 5.0;

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
    //--- How many people are spectating this player. The offline rig is the ONLY way to reach this
    //--- element at all: SERVER is undefined offline, so no spectator can exist and the server half
    //--- that produces the number is compiled out. Its range reaches 0 on purpose - 0 is the hidden
    //--- state, and a fixture that cannot reach the branch under test cannot test it.
    static int hud_audience = 2;
    static int hud_countdown = 60;

    //--- Zones. Same idea: synthetic circles centred on the player, which feed the HUD distance
    //--- arrow and - through the single VigridMapAPI.SetZones call - the map's rings.
    static bool zones_fake = false;
    static float zone_radius = 1500;
    static float zone_next_radius = 600;

    //--- Suppress the CURRENT circle while still publishing the next one - the shape the client is in
    //--- for the warm-up and the first 80% of round one, when the server has advertised where the
    //--- first circle is but nothing is lethal yet. Without this the fake rig always sets BOTH areas
    //--- and so can only ever exercise the branch that already worked: a fixture that cannot violate
    //--- the property under test cannot test it.
    static bool zones_fake_no_current = false;

    //--- ADMIN SPECTATE OVERLAY. Fabricates the whole SetAdminPlayerList / SetAdminDeadList payload
    //--- client-side, which is the only way any of #276-#279 can be seen without a three-client MP
    //--- session plus an admin steamid: every existing Spectate diag entry is SERVER-side, and SERVER
    //--- is undefined offline, so the half that produces this data is compiled out.
    //---
    //--- Drives four things at once - the team colours on the tags, the corpse tags, the tag
    //--- suppression while the map is open, and the map's own player layer - because all four read
    //--- the same two payloads.
    static bool admin_fake = false;
    static int admin_fake_players = 12;
    static int admin_fake_dead = 6;

    //--- Metres from the player that the fabricated ring is drawn at. Small enough that the tags are
    //--- legible on screen, large enough that the map glyphs do not stack into one blob.
    static float admin_fake_spread = 120.0;

    //--- Kill feed. Read by the "Push Fake Kill" / "Fill Feed" callbacks at the moment they fire,
    //--- rather than mirroring the entry ids, so no id has to leave the plugin class.
    static int kf_cause = 0;
    static bool kf_with_weapon = true;

    //--- Party roster size to fabricate.
    static int party_size = 3;

    //--- How many connected players to fabricate for the party menu's left column. Separate from
    //--- party_size: the roster and the online list are two independent fabrications, and the point
    //--- of the second is having somebody to invite INTO the first.
    //---
    //--- MUST match the default declared on the "Fake Online Players" range in PluginDiagMenu. A
    //--- range callback only fires when the value is CHANGED, so a mismatch means pressing Apply
    //--- without touching the slider silently uses this number rather than the one on screen.
    static int party_online_count = 20;

    //--- Spectate range test: how far from the spectator's corpse to fling the watched target.
    //--- Held client-side and sent only when "TP Target: Go" is pressed, for the same reason
    //--- goto_state is - a range callback plausibly fires on every step while scrubbing.
    static int tp_target_distance = 1200;

    //--- Match Flow's "Jump To State" target. Held client-side and sent only when "Jump: Go" is
    //--- pressed: a range callback plausibly fires on every step while scrubbing, and nothing here
    //--- should be able to fire ten RPCs on the way to state 10.
    static int goto_state = 0;

    //--- One-shot requests. Monotonic counters, not bools - the consumer diffs against its own
    //--- last-seen value, so nobody owns the clear and two presses in one frame are not merged.
    static int req_open_spawn_menu = 0;
    static int req_open_leaderboard = 0;
    static int req_open_death_screen = 0;

    //--- Raise a burst of test toasts. The notification stack is otherwise unreachable offline:
    //--- every real toast is a server push, and the one client-raised toast (NotifyLocal, on the F6
    //--- skeleton toggle) needs an admin spectate session to get at.
    static int req_push_toasts = 0;

    //--- Which BattleRoyaleKillCause the fake recap uses. Cycled so every recap branch and every
    //--- STR_BR_CAUSE_* key can be eyeballed without dying nine different ways on a live server.
    static int lastmatch_cause = BattleRoyaleKillCause.FIREARM;

    //--- Fake the "did not play the last match" case, which is the COMMON one in a real lobby and so
    //--- the one most likely to be left untested.
    static bool lastmatch_not_played = false;

    /**
     *  Let the death screen stay open while the local player is alive.
     *
     *  DeathScreenMenu.Tick() closes itself the moment GetPlayer().IsAlive() - an invariant that
     *  exists for the admin respawn. Offline the player is ALWAYS alive, so without this the screen
     *  shuts on its first tick and reads as a broken layout rather than as a working guard.
     */
    static bool suppress_alive_close = false;

    //! Zero everything. The plugin is destroyed and recreated on every world change, so this runs
    //! from RegisterModdedDiags() as well as from BattleRoyaleClient.Init().
    static void Reset()
    {
        log_level_override = -1;
        chat_mirror = true;
        trace_teleport = false;
        trace_teleport_ticks = 20;
        trace_aim = false;
        spectate_trace_interval = 5.0;

        hud_force = false;
        hud_players = 60;
        hud_groups = 20;
        hud_kills = 3;
        hud_audience = 2;
        hud_countdown = 60;

        zones_fake = false;
        zone_radius = 1500;
        zone_next_radius = 600;
        zones_fake_no_current = false;

        admin_fake = false;
        admin_fake_players = 12;
        admin_fake_dead = 6;
        admin_fake_spread = 120.0;

        kf_cause = 0;
        kf_with_weapon = true;
        party_size = 3;
        party_online_count = 20;
        tp_target_distance = 1200;

        goto_state = 0;

        req_open_spawn_menu = 0;
        req_open_leaderboard = 0;
        req_open_death_screen = 0;
        req_push_toasts = 0;
        lastmatch_cause = BattleRoyaleKillCause.FIREARM;
        lastmatch_not_played = false;
        suppress_alive_close = false;
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
