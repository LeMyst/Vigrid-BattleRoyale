#ifdef SERVER
/**
 *  Spectating after death, entered IN PLACE - the client never disconnects and never reconnects.
 *
 *  Two earlier attempts (the system removed in e6a0e1b, and the `test-vpp-spectate` branch) both
 *  entered spectate on RECONNECT. That cannot work here: 2_BattleRoyaleCountReached locks the
 *  server through the autolock webhook so a dead player cannot rejoin, and vanilla MissionServer
 *  calls InvokeOnConnect only from ClientNewEvent/ClientReadyEvent, never from ClientRespawnEvent,
 *  so OnPlayerConnected does not even run on a respawn.
 *
 *  THREE INVARIANTS HOLD THE WHOLE FEATURE UP:
 *
 *  1. NOTHING HERE HOLDS A REFERENCE TO SOMEBODY ELSE'S OBJECT. Not a PlayerBase, not a
 *     PlayerIdentity - only SteamID64 strings (PlayerIdentity.GetPlainId(), never GetPlayerId(),
 *     which is a session index the engine reuses after a disconnect). Entities are re-resolved from
 *     a uid at the moment of use, so "the PlayerBase was freed" stops being a failure mode. This is
 *     also why the killer is stored as a uid: a handle to a corpse or a dropped weapon dangles, a
 *     uid does not, and the chain has to survive the killer's own death and disconnect.
 *     There are NO exceptions: this class holds no object reference at all.
 *
 *  2. A SPECTATOR IS NEVER REVIVED AND NEVER RE-ADDED TO m_Players. BattleRoyaleServer.OnPlayerTick
 *     force-logs-out anyone who is outside the current state AND EPlayerStates.ALIVE, so a
 *     spectator must stay dead. Re-adding them to the roster would also inflate GetPlayers().Count()
 *     and VigridPartyAPI.GetGroupCount(), which stalls every IsComplete() and makes UpdateTopPosition
 *     rewrite every survivor's br_position against an inflated field.
 *
 *  3. THE CORPSE IS NEVER DELETED, AND NEVER MOVED. The corpse IS the PlayerBase, and vanilla
 *     HandleBody only deletes a body when the player was alive, so a dead player's gear is meant to
 *     persist and be lootable where they fell. The removed implementation called ObjectDelete(player)
 *     "for network bubble fix", which destroys the victim's loot.
 *
 *     Note what that costs, because it is a deliberate trade and not an oversight. The replication
 *     bubble stays centred on this corpse - UpdateSpectatorPosition does NOT move it, established
 *     both directions 2026-08-10 (dead at 1200 m for 90 s, alive again at 700 m, 507 pushes
 *     throughout). So a spectator cannot see anything beyond ~1 km of where they died. MOVING the
 *     corpse would fix that and needs no entity creation, which is what killed the carrier - but it
 *     drags the victim's gear across the map with it, so the loot can never be found. That is a
 *     gameplay decision, not a bug fix. See CLAUDE.md, "Architecture -> Spectating".
 *
 *  There is exactly ONE driver - Tick(), called from BattleRoyaleServer.Update()'s existing 10 Hz
 *  block. No Timer, no CallLaterByName, no coroutine: a driver that only touches strings cannot fire
 *  against a freed object.
 *
 *  CLIENT -> SERVER RPCs. For an ordinary spectator there is exactly one, RequestSpectate, and it
 *  carries no payload: the actor is the engine-supplied `sender` and the only thing it can do is
 *  start spectating for a uid the server itself already registered. The four admin RPCs
 *  (AdminSpectateToggle / Cycle / Mode / CamPos) all resolve their actor the same way and all open
 *  with AdminEligibility, so nothing a client sends chooses who it acts on.
 *
 *  ADMIN SPECTATE, in one rule: it requires a NON-PARTICIPANT - alive, holding a body, and absent
 *  from m_Players. An admin who is competing is refused, because a competitor who can freecam the
 *  map is indistinguishable from a cheat. Two ways to be a non-participant: connect mid-match (which
 *  BattleRoyaleServer.OnPlayerConnected already handles, placing them at the live circle and
 *  exempting them from the late-join kick), or die and take AdminRespawn, which is the bridge
 *  between the two halves of the lifecycle. AdminEligibility is that rule, and every admin RPC
 *  consults it rather than re-deriving it.
 *
 *  Note that AdminRespawn is the one thing here that creates an entity, which invariant 2 otherwise
 *  forbids. It does not violate it: the new body is never added to m_Players, so the roster count,
 *  IsComplete() and br_position are all untouched, and the admin's own placement, leaderboard entry
 *  and corpse survive their respawn exactly as they were.
 */

//! BattleRoyaleDeathRecord used to be declared here. It now lives in
//! Scripts/Server/4_World/MatchSummary/BattleRoyaleDeathRecord.c, because BattleRoyaleMatchStats
//! also reads it and that class has to be 4_World for PlayerBase.EEHitBy to reach it - a 4_World
//! method cannot name a 5_Mission type. This class still owns and writes every record.

//! One player currently spectating.
class BattleRoyaleSpectatorEntry
{
    string uid;         //!< the spectator. Map key.
    string name;
    string target_uid;  //!< "" means no target - the camera orbits the play area centre
    int entered_ms;
    int enter_due_ms;   //!< deadline for the deferred SelectSpectator
    bool pending_enter;

    //! Which ResolveTarget tier (1-5) produced target_uid. Diagnostics only - nothing branches on
    //! it. The chain is five tiers deep and, without this, which one fired is only inferable from
    //! who you end up watching, which is exactly the thing that is hard to reason about live.
    int resolved_tier;

    //! Has this spectator's corpse been carried at least once? Gates the one-time DropAllItems: the
    //! loot must land at the death position, so it has to happen before the FIRST move and never
    //! again. Also switches the trigger from "trigger distance, if nobody is nearby" to the plain
    //! step distance, since an already-emptied, already-moved body has nothing left to protect.
    bool corpse_carried;

    //! An ADMIN session rather than a death. Changes three things and nothing else: the target
    //! resolver (ResolveAdminTarget, because ResolveTarget keys off the spectator's own death record
    //! and an admin has none), the carry destination (the camera, not the target), and the fact that
    //! the body being carried is alive and its gear must not be dropped.
    bool is_admin;

    //! Camera mode. Server-authoritative and pushed to the client in SetSpectateTarget - the client
    //! never picks one, it only asks. Only an admin entry is ever set to BR_SPECTATE_MODE_FREE.
    int mode;

    //! Last camera position the client reported, for the free-camera body carry. "0 0 0" until the
    //! first report, which CarryAnchorBody treats as "nothing to do".
    vector cam_pos;

    //! Earliest time the anchor body may be re-placed. Per-entry rather than global, so two admins
    //! spectating at once do not share one clock and starve each other.
    int next_anchor_ms;

    //! Earliest time THIS entry's carry pass may run (#280). Per-entry for exactly the reason
    //! next_anchor_ms is, and for one more: with a single shared clock every spectator evaluated on
    //! the same tick, and since many of them watch the same few survivors their corpses end up at the
    //! IDENTICAL position, so they cross BR_SPECTATE_CARRY_STEP_M together too - a burst of S
    //! teleports in one frame. Seeded with a random phase by SeedCarryPhase.
    //!
    //! Zero while BR_SPECTATE_CARRY_STAGGER is off, in which case nothing reads it.
    int next_carry_ms;

    void BattleRoyaleSpectatorEntry(string spectator_uid, string spectator_name)
    {
        uid = spectator_uid;
        name = spectator_name;
        target_uid = "";
        entered_ms = 0;
        enter_due_ms = 0;
        pending_enter = true;
        resolved_tier = 0;
        corpse_carried = false;
        is_admin = false;
        mode = BR_SPECTATE_MODE_ORBIT;
        cam_pos = "0 0 0";
        next_anchor_ms = 0;
        next_carry_ms = 0;
    }

    /**
     *  Draw this entry's carry phase.
     *
     *  Called where the carry starts MATTERING - BeginSpectate and BeginAdminSpectate - and never
     *  from the constructor. An entry is built at death and then sits through the whole death screen
     *  (BR_SPECTATE_ENTRY_DELAY_MS, 20 s), so a phase drawn at construction has long expired by the
     *  time the first pass runs; worse, two players who died in the same fight would both start
     *  their first real pass on the same tick and re-correlate immediately.
     *
     *  Unwrapped Math.RandomInt, the idiom throughout this mod. It consumes from the global stream,
     *  which is fine: this runs at spectate time, long after BattleRoyaleZone.PrepareGeneration has
     *  committed the chain, so a zone_generation_seed replay is unaffected.
     */
    void SeedCarryPhase()
    {
        next_carry_ms = GetGame().GetTime() + Math.RandomInt(0, BR_SPECTATE_CARRY_PHASE_MS);
    }
}

class BattleRoyaleSpectators
{
    protected static ref BattleRoyaleSpectators m_Instance;

    protected ref map<string, ref BattleRoyaleDeathRecord> m_Deaths;
    protected ref array<string> m_DeathOrder;  //!< victim uids, chronological
    protected ref map<string, ref BattleRoyaleSpectatorEntry> m_Spectators;

    protected int m_NextPushMs;
    protected int m_NextCarryMs;
    protected int m_NextAdminListMs;
    protected bool m_Ended;

    //! Watched uid -> the audience count last PUSHED to that player (#285).
    //!
    //! It exists for one reason: PushAudienceCounts is a RECOMPUTE, so it can see who has an
    //! audience but not who has just STOPPED having one. A uid that falls out of the tally is a
    //! player who needs a zero, and without this map there is nothing left to notice that - they
    //! would sit on a stale number for the rest of the match.
    protected ref map<string, int> m_AudienceSent;
    protected int m_NextAudienceMs;
    protected int m_NextAudienceKeepaliveMs;

    //! Cadence and change-detection for the corpse nametag list (#278). The set is append-only, so
    //! m_LastDeadPushCount is enough to know whether anything is worth sending; the keepalive covers
    //! an admin who entered spectate during a lull and would otherwise wait for the next death.
    protected int m_NextDeadListMs;
    protected int m_NextDeadKeepaliveMs;
    protected int m_LastDeadPushCount;

    //! Party id -> the team colour index that party holds for this match, assigned first-seen.
    //!
    //! Never cleared mid-match, and that IS the feature: a colour must not move under an admin who
    //! is using it to read a fight. The process restarts between matches (9_BattleRoyaleRestart),
    //! so there is nothing to reset - the map dies with the object. Bounded by the number of parties,
    //! which is bounded by half the player cap.
    protected ref map<string, int> m_PartyColourIndex;

    //! Which tier the LAST ResolveTarget call returned from. Written at every one of its five exits
    //! and read by the caller on the very next line, so it is a return value in all but name - a
    //! second out-parameter would have to be threaded through three call sites for a field nothing
    //! branches on. Never read it anywhere but immediately after a ResolveTarget call.
    protected int m_LastResolveTier;

    //==========================================================================================
    //--- LOAD COUNTERS (#280). Unguarded, on the same footing as BattleRoyaleZone.s_SurfaceCalls:
    //--- an int increment is free, and a counter that only exists in a diag build cannot answer a
    //--- question about a live server. Only the REPORT is throttled, and only the two switches that
    //--- steer a measurement are diag-gated.
    //---
    //--- The identity and roster columns are not padding. Without something to be a percentage OF,
    //--- "the carry costs X" decides nothing - and what it has to be compared against is the 10 Hz
    //--- liveness sweep in the same Tick(), which does the same shape of work ten times as often.
    //==========================================================================================

    //! Walks = how many times the whole list was fetched. Scanned = the sum of those lists' LENGTHS,
    //! which is an UPPER BOUND on comparisons rather than the true count: every one of these walks
    //! early-exits on a match. The bound is the honest figure to quote for FindBodyByUid, where the
    //! miss case dominates, and it is pessimistic by at most a factor of two for the others.
    static int s_IdentityWalks;
    static int s_IdentityScanned;
    static int s_RosterWalks;
    static int s_RosterScanned;
    //! THE term this issue is about: O(S x N), and N is every Man in the world, which only grows -
    //! the mod never deletes a body.
    static int s_BodyWalks;
    static int s_BodyScanned;
    static int s_Teleports;
    static int s_CarryFirst;
    static int s_CarryStep;
    static int s_CarryEvals;
    //! Most MoveCorpse calls in ONE Tick pass since the last report. This is the correlation
    //! measurement: with the stagger off it should approach the spectator count, with it on roughly
    //! a tenth of that. Everything else here is a rate; this is the peak.
    static int s_BurstMax;

    //! Time spent inside Tick's per-entry loop, accumulated across the reporting window.
    //!
    //! GetTickTime() is float SECONDS and advances within a frame; GetGame().GetTime() is integer
    //! milliseconds and a single pass of this loop reads 0 from it, which is how "free" gets
    //! concluded from a measurement that never had the resolution to say so. Accumulated across the
    //! window rather than reported per pass for the same reason.
    protected float m_TickSeconds;
    protected int m_NextLoadMs;

    void BattleRoyaleSpectators()
    {
        m_Deaths = new map<string, ref BattleRoyaleDeathRecord>();
        m_DeathOrder = new array<string>();
        m_Spectators = new map<string, ref BattleRoyaleSpectatorEntry>();
        m_NextPushMs = 0;
        m_NextCarryMs = 0;
        m_NextAdminListMs = 0;
        m_Ended = false;
        m_AudienceSent = new map<string, int>();
        m_PartyColourIndex = new map<string, int>();
        m_NextDeadListMs = 0;
        m_NextDeadKeepaliveMs = 0;
        m_LastDeadPushCount = -1;
        m_NextAudienceMs = 0;
        m_NextAudienceKeepaliveMs = 0;
        m_LastResolveTier = 0;
        m_TickSeconds = 0;
        m_NextLoadMs = 0;
        ResetLoadCounters();
    }

    static BattleRoyaleSpectators GetInstance()
    {
        if (!m_Instance)
            m_Instance = new BattleRoyaleSpectators();

        return m_Instance;
    }

    //------------------------------------------------------------------------------------------
    //--- uid -> thing resolvers. The whole answer to "how does this survive a freed PlayerBase".
    //------------------------------------------------------------------------------------------

    /**
     *  The connected identity for a uid, or NULL.
     *
     *  THIS is the liveness test for a spectator, not MissionServer.PlayerDisconnected: it answers
     *  correctly for a client that controls no entity at all, which is precisely what a spectator is.
     *  Note the vanilla typo in GetPlayerIndentities.
     */
    static PlayerIdentity IdentityOfUid(string uid)
    {
        if (uid == "")
            return NULL;

        array<PlayerIdentity> identities = new array<PlayerIdentity>();
        GetGame().GetPlayerIndentities(identities);

        int count = identities.Count();
        s_IdentityWalks++;
        s_IdentityScanned = s_IdentityScanned + count;
        for (int i = 0; i < count; i++)
        {
            PlayerIdentity candidate = identities.Get(i);
            if (!candidate)
                continue;

            if (candidate.GetPlainId() == uid)
                return candidate;
        }

        return NULL;
    }

    /**
     *  A player still IN THE MATCH for a uid, or NULL.
     *
     *  The match roster is the authority for "alive and playing" - a dead-but-connected spectator is
     *  absent from it by construction, so this can never hand back a spectator as someone's target.
     */
    static PlayerBase LivingPlayerByUid(string uid)
    {
        if (uid == "")
            return NULL;

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return NULL;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return NULL;

        array<PlayerBase> roster = state.GetPlayers();
        if (!roster)
            return NULL;

        int count = roster.Count();
        s_RosterWalks++;
        s_RosterScanned = s_RosterScanned + count;

        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            if (candidate.GetIdentity().GetPlainId() == uid)
                return candidate;
        }

        return NULL;
    }

    //! Party id of a live player, "" without the addon. The single #ifdef VIGRID_PARTY in this file.
    static string PartyIdOf(PlayerBase player)
    {
        if (!player)
            return "";

#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetPartyId(player);
#else
        return "";
#endif
    }

    /**
     *  Is the corpse carry running at all?
     *
     *  Same two-line shape as PartyIdOf above, and for the same reason: the #ifdef lives here so
     *  Tick() and CarryCorpse() read as plain code. In a retail build this compiles to the constant.
     *
     *  This is the A/B that PRICES the carry. With it off, CarryCorpse returns on its first line and
     *  the ENTIRE evaluation is skipped - both list walks and both array allocations - so the
     *  difference in the load line's tickms between two windows is the carry's cost, allocations
     *  included. That is a better instrument than a nested timer around the carry branch, which
     *  would measure the code but not what the allocator does afterwards.
     */
    protected bool CarryEnabled()
    {
#ifdef DIAG_DEVELOPER
        if (BattleRoyaleDiag.carry_enabled_override >= 0)
            return (BattleRoyaleDiag.carry_enabled_override > 0);
#endif

        return BR_SPECTATE_CARRY_CORPSE;
    }

    //! Is the per-entry carry phase in effect, rather than the one global clock? See
    //! BR_SPECTATE_CARRY_STAGGER - and note the constant, not the override, is what ships.
    protected bool CarryStaggered()
    {
#ifdef DIAG_DEVELOPER
        if (BattleRoyaleDiag.carry_stagger_override >= 0)
            return (BattleRoyaleDiag.carry_stagger_override > 0);
#endif

        return BR_SPECTATE_CARRY_STAGGER;
    }

    //------------------------------------------------------------------------------------------
    //--- The ledger
    //------------------------------------------------------------------------------------------

    /**
     *  Resolve the PLAYER responsible for a death to a uid, or "" when nothing player-shaped is.
     *
     *  Kept as a method here because the killer chain's call sites read better for it, but the logic
     *  lives in BattleRoyaleKillAttribution - it is shared with the webhook, the kill credit and
     *  PlayerBase.EEHitBy, and those four used to disagree with each other about grenades and traps.
     */
    static string ResolveKillerUid(PlayerBase victim, Object source)
    {
        return BattleRoyaleKillAttribution.ResolveKillerUid(victim, source);
    }

    /**
     *  Record a death. FIRST WRITE WINS.
     *
     *  That rule is what makes two known double-fire paths harmless. RemovePlayer is documented to
     *  run twice per kill, and the unconscious-disconnect path in BattleRoyaleServer.OnPlayerDisconnect
     *  zeroes health after resolving the real killer - which fires a second EEKilled whose source is
     *  the victim themself. Without first-write-wins that second call would overwrite good
     *  attribution with "".
     *
     *  Must be called BEFORE RemovePlayer, while the victim still has an identity and a party.
     */
    void RecordDeath(PlayerBase victim, Object source)
    {
        if (!victim)
            return;
        if (!victim.GetIdentity())
            return;

        string uid = victim.GetIdentity().GetPlainId();
        if (uid == "")
            return;

        if (m_Deaths.Contains(uid))
            return;  //--- first write wins

        string killer_uid = ResolveKillerUid(victim, source);
        string party_id = PartyIdOf(victim);
        string victim_name = victim.player_name;
        if (victim_name == "")
            victim_name = victim.GetIdentity().GetName();

        BattleRoyaleDeathRecord record = new BattleRoyaleDeathRecord(uid, victim_name, killer_uid, GetGame().GetTime(), party_id, victim.GetPosition());

        //--- The recap, resolved while `source` - the weapon or the armed device - is still in hand.
        //--- This is the only moment it is: EEKilled's source is not retained anywhere.
        //---
        //--- DELIBERATELY inside the first-write-wins block above. Filling these in on a later call
        //--- would be the one way to break the feature: the unconscious-disconnect path fires a
        //--- second EEKilled whose source is the VICTIM, so a second pass would overwrite a good
        //--- attribution with ENVIRONMENT and an empty name.
        BattleRoyaleKillDetails details = new BattleRoyaleKillDetails();
        BattleRoyaleKillAttribution.ResolveKillDetails(victim, source, details);

        record.cause = details.cause;
        record.killer_name = details.killer_name;
        record.weapon_type = details.weapon_type;
        record.distance_m = details.distance_m;
        record.killer_health_pct = details.killer_health_pct;
        record.damage_to_killer = BattleRoyaleMatchStats.GetInstance().GetPairDamage(uid, killer_uid);

        m_Deaths.Set(uid, record);
        m_DeathOrder.Insert(uid);

        string killer_log = killer_uid;
        if (killer_log == "")
            killer_log = "<environment>";

        string party_log = party_id;
        if (party_log == "")
            party_log = "-";

        BattleRoyaleUtils.Info(string.Format("[Spectate] RecordDeath victim=%1 killer=%2 party=%3", uid, killer_log, party_log));

        //--- The resolved recap, logged separately so what the server decided can be cross-checked
        //--- against what the death screen actually renders. Without it the only way to tell a bad
        //--- resolve from a bad render is to stage another two-client kill.
        //--- Built in steps: six fields in one concatenation is the shape that trips
        //--- "Formula too complex", which is a hard compile error the packer does not catch.
        string recap_log = "[Recap] victim=" + uid;
        recap_log = recap_log + " cause=" + record.cause;
        recap_log = recap_log + " killer=" + record.killer_name;
        recap_log = recap_log + " weapon=" + record.weapon_type;
        recap_log = recap_log + " dist=" + record.distance_m;
        recap_log = recap_log + " killer_hp=" + record.killer_health_pct;
        recap_log = recap_log + " dealt=" + record.damage_to_killer;
        BattleRoyaleUtils.Info(recap_log);
    }

    /**
     *  Record a death whose killer the caller has ALREADY resolved to a PlayerBase.
     *
     *  Used by the unconscious-disconnect path, which knows the real killer but then kills the
     *  player by zeroing health - losing that attribution by the time EEKilled fires.
     */
    void RecordDeathWithKiller(PlayerBase victim, PlayerBase killer)
    {
        string resolved = "";
        if (killer && killer.GetIdentity())
            resolved = killer.GetIdentity().GetPlainId();

        RecordDeathWithKillerUid(victim, resolved);
    }

    /**
     *  Record a death whose killer the caller has already resolved to a UID.
     *
     *  The uid form is the general one: a killer downed by an explosive may have no PlayerBase left
     *  at all, since a device records its owner as a string precisely so the attribution can outlive
     *  them. RecordDeathWithKiller is the object-shaped convenience wrapper over this.
     */
    void RecordDeathWithKillerUid(PlayerBase victim, string killer_uid)
    {
        if (!victim)
            return;
        if (!victim.GetIdentity())
            return;

        string uid = victim.GetIdentity().GetPlainId();
        if (uid == "")
            return;

        if (m_Deaths.Contains(uid))
            return;  //--- first write wins

        if (killer_uid == uid)
            killer_uid = "";

        string party_id = PartyIdOf(victim);
        string victim_name = victim.player_name;
        if (victim_name == "")
            victim_name = victim.GetIdentity().GetName();

        BattleRoyaleDeathRecord record = new BattleRoyaleDeathRecord(uid, victim_name, killer_uid, GetGame().GetTime(), party_id, victim.GetPosition());

        //--- The DEGRADED recap. There is no `source` on this path - the hit that downed them was
        //--- minutes ago and the object is long gone - so the weapon and the range are genuinely
        //--- lost. They are left unknown rather than guessed: the killer may have moved a kilometre
        //--- since, and a plausible wrong distance is worse than none at all. The killer's name is
        //--- the one thing that is both knowable and worth showing, and the damage already dealt to
        //--- them is a running total rather than a snapshot, so it survives the delay intact.
        if (killer_uid != "")
        {
            record.cause = BattleRoyaleKillCause.UNKNOWN;
            record.killer_name = BattleRoyaleKillAttribution.NameOfPlayer(BattleRoyaleKillAttribution.FindPlayerByUid(killer_uid));
            record.damage_to_killer = BattleRoyaleMatchStats.GetInstance().GetPairDamage(uid, killer_uid);
        }

        m_Deaths.Set(uid, record);
        m_DeathOrder.Insert(uid);

        BattleRoyaleUtils.Info(string.Format("[Spectate] RecordDeath (disconnect) victim=%1 killer=%2", uid, killer_uid));
    }

    //! The death record for a uid, or NULL if they are still alive. Plain data - see the class
    //! header on BattleRoyaleDeathRecord for why handing out the reference is safe here.
    BattleRoyaleDeathRecord GetDeathRecord(string uid)
    {
        if (uid == "")
            return NULL;
        if (!m_Deaths.Contains(uid))
            return NULL;

        return m_Deaths.Get(uid);
    }

    //! Convenience form for callers holding a player rather than a uid. Reads player_steamid first,
    //! since a disconnecting player's identity is already NULL by the time RemovePlayer runs.
    BattleRoyaleDeathRecord GetDeathRecordFor(PlayerBase player)
    {
        string uid = "";

        if (!player)
            return NULL;

        uid = player.player_steamid;
        if (uid == "" && player.GetIdentity())
            uid = player.GetIdentity().GetPlainId();

        return GetDeathRecord(uid);
    }

    //------------------------------------------------------------------------------------------
    //--- Target resolution
    //------------------------------------------------------------------------------------------

    /**
     *  Who this spectator should watch, or "" for "nobody - orbit the zone".
     *
     *  T1  a living teammate
     *  T2  the killer chain, seeded at the last teammate to die (a solo player seeds at themselves,
     *      so hop 0 reads their own killer - which is "a solo player's chain starts at their own
     *      killer", for free). Walks through dead killers to THEIR killers.
     *  T3  the most recent kill whose killer is still alive
     *  T4  the living player nearest to where this spectator fell
     *  T5  nothing - orbit the final circle
     */
    string ResolveTarget(string spectator_uid)
    {
        BattleRoyaleDeathRecord record = m_Deaths.Get(spectator_uid);
        string party = "";
        string seed = spectator_uid;
        string cursor = "";
        string killer = "";
        string newest = "";
        string teammate_uid = "";
        int i = 0;
        int hop = 0;
        ref set<string> visited = new set<string>();

        if (record)
            party = record.party_id;

        //--- T1: a living teammate. Live query against the live roster - no snapshot, so it cannot
        //--- go stale. Roster order is the tie-break; any member is an equally valid answer.
        if (party != "")
        {
            BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
            if (server && server.GetCurrentState())
            {
                array<PlayerBase> roster = server.GetCurrentState().GetPlayers();
                if (roster)
                {
                    int roster_count = roster.Count();
                    for (i = 0; i < roster_count; i++)
                    {
                        PlayerBase candidate = roster.Get(i);
                        if (!candidate)
                            continue;
                        if (!candidate.GetIdentity())
                            continue;

                        teammate_uid = candidate.GetIdentity().GetPlainId();
                        if (teammate_uid == spectator_uid)
                            continue;
                        if (PartyIdOf(candidate) != party)
                            continue;

                        m_LastResolveTier = 1;
                        return teammate_uid;
                    }
                }
            }
        }

        //--- T2 seed: the last teammate to die. Falls back to the spectator themselves.
        if (party != "")
        {
            for (i = m_DeathOrder.Count() - 1; i >= 0; i--)
            {
                string dead_uid = m_DeathOrder.Get(i);
                if (dead_uid == spectator_uid)
                    continue;

                BattleRoyaleDeathRecord dead_record = m_Deaths.Get(dead_uid);
                if (!dead_record)
                    continue;
                if (dead_record.party_id != party)
                    continue;

                seed = dead_uid;
                break;
            }
        }

        //--- T2: walk the chain.
        visited.Insert(spectator_uid);
        if (visited.Find(seed) == -1)
            visited.Insert(seed);

        cursor = seed;

        for (hop = 0; hop < BR_SPECTATE_CHAIN_MAX_HOPS; hop++)
        {
            BattleRoyaleDeathRecord node = m_Deaths.Get(cursor);
            if (!node)
                break;

            killer = node.killer_uid;
            if (killer == "")
                break;  //--- environmental death, the chain ends here
            if (visited.Find(killer) != -1)
                break;  //--- cycle guard

            visited.Insert(killer);

            if (LivingPlayerByUid(killer))
            {
                m_LastResolveTier = 2;
                return killer;
            }

            if (!m_Deaths.Contains(killer))
                break;  //--- killer left the server without dying

            cursor = killer;  //--- killer is dead too, follow THEIR killer
        }

        //--- T3: the most recent kill whose killer is still alive. Only reachable once the
        //--- spectator's whole party is dead, so it cannot leak a teammate's position to anyone.
        for (i = m_DeathOrder.Count() - 1; i >= 0; i--)
        {
            BattleRoyaleDeathRecord recent = m_Deaths.Get(m_DeathOrder.Get(i));
            if (!recent)
                continue;
            if (recent.killer_uid == "")
                continue;
            if (!LivingPlayerByUid(recent.killer_uid))
                continue;

            newest = recent.killer_uid;
            break;
        }

        if (newest != "")
        {
            m_LastResolveTier = 3;
            return newest;
        }

        //--- T4: the living player NEAREST to where this spectator fell.
        //--- This is what a suicide or a pure zone death with no kills anywhere resolves to. Orbiting
        //--- an empty circle is technically honest but useless to watch, and "nearest" at least puts
        //--- the camera on whoever was closest to the action you just left. Deliberately NOT random:
        //--- a random stranger defeats the chained contract, and GetRandomElement() on an empty array
        //--- calls Get(-1).
        string nearest = NearestLivingUid(spectator_uid);
        if (nearest != "")
        {
            m_LastResolveTier = 4;
            return nearest;
        }

        //--- T5: genuinely nobody left to follow - orbit the final circle.
        m_LastResolveTier = 5;
        return "";
    }

    //! The living roster member closest to `spectator_uid`'s death position, or "" if none.
    protected string NearestLivingUid(string spectator_uid)
    {
        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return "";
        if (!server.GetCurrentState())
            return "";

        array<PlayerBase> roster = server.GetCurrentState().GetPlayers();
        if (!roster)
            return "";

        vector origin = PlayAreaCentre();
        BattleRoyaleDeathRecord record = m_Deaths.Get(spectator_uid);
        if (record)
            origin = record.death_pos;

        string best_uid = "";
        float best_distance = 0;
        int count = roster.Count();

        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            string candidate_uid = candidate.GetIdentity().GetPlainId();
            if (candidate_uid == spectator_uid)
                continue;

            //--- 2D: a player on a rooftop is not further away in any sense that matters here.
            vector delta = candidate.GetPosition() - origin;
            delta[1] = 0;
            float distance = delta.Length();

            if (best_uid != "" && distance >= best_distance)
                continue;

            best_uid = candidate_uid;
            best_distance = distance;
        }

        return best_uid;
    }

    //------------------------------------------------------------------------------------------
    //--- Registry
    //------------------------------------------------------------------------------------------

    bool IsSpectator(string uid)
    {
        return m_Spectators.Contains(uid);
    }

    int GetSpectatorCount()
    {
        return m_Spectators.Count();
    }


    /**
     *  A player died. Register them as a spectator if the feature is on and the state allows it.
     *
     *  Must be called AFTER RemovePlayer, or the victim is still on the roster and can resolve
     *  themselves as their own target.
     */
    void OnDeath(PlayerBase victim)
    {
        if (m_Ended)
            return;
        if (!victim)
            return;
        if (!victim.GetIdentity())
            return;

        //--- The single read point for the feature flag. Turning it off mid-match stops new
        //--- spectators and leaves existing ones alone; turning it on works from the next death.
        if (!BattleRoyaleConfig.GetConfig().GetGameData().spectate_enabled)
            return;

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return;
        if (!state.AllowsSpectate())
            return;

        string uid = victim.GetIdentity().GetPlainId();
        if (uid == "")
        {
            BattleRoyaleUtils.Warn("[Spectate] OnDeath: victim has a blank SteamID64, not spectating");
            return;
        }

        if (m_Spectators.Contains(uid))
        {
            BattleRoyaleUtils.Warn("[Spectate] OnDeath: " + uid + " is already a spectator");
            return;
        }

        string victim_name = victim.player_name;
        if (victim_name == "")
            victim_name = victim.GetIdentity().GetName();

        BattleRoyaleSpectatorEntry entry = new BattleRoyaleSpectatorEntry(uid, victim_name);
        entry.enter_due_ms = GetGame().GetTime() + BR_SPECTATE_ENTRY_DELAY_MS;
        //--- Resolved again at BeginSpectate, so the chain reflects who is alive when spectating
        //--- actually starts rather than who was alive at the instant of death. This one is only a
        //--- provisional value for logging.
        entry.target_uid = ResolveTarget(uid);
        entry.resolved_tier = m_LastResolveTier;
        m_Spectators.Set(uid, entry);

        //--- Offer the choice. The client shows Spectate / Quit; pressing Spectate skips the wait,
        //--- pressing nothing lets enter_due_ms do it anyway.
        GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetSpectateOffer", new Param1<bool>(true), true, victim.GetIdentity());

        //--- Vanilla EEKilled already calls EnableVoN(this, false), which blocks TRANSMISSION.
        //--- Hearing is not provably blocked by that, so deafen explicitly: a spectator parked on a
        //--- living enemy must not be able to relay what they see.
        GetGame().MuteAllPlayers(uid, true);

        BattleRoyaleUtils.Info(string.Format("[Spectate] Registered %1 (%2), first target=%3 (T%4)", uid, victim_name, TargetLog(entry.target_uid), entry.resolved_tier));
    }

    //! Drop a spectator, e.g. because they disconnected.
    void Remove(string uid)
    {
        if (!m_Spectators.Contains(uid))
            return;

        DropEntry(uid);
        BattleRoyaleUtils.Info("[Spectate] Removed " + uid);
    }

    /**
     *  THE ONLY PLACE A SPECTATOR ENTRY IS DROPPED.
     *
     *  It exists so that no exit path can forget to undo what a session took out on the rest of the
     *  mod. There are four of them - a clean F3 exit, a body that went away underneath one, the
     *  liveness sweep, and an admin respawn superseding an ordinary death session - and the party
     *  hide had to be released on every single one, because a member left hidden reads to their
     *  teammates as permanently offline for the rest of the match.
     */
    protected void DropEntry(string uid)
    {
        SetPartyHidden(uid, false);
        m_Spectators.Remove(uid);
    }

    /**
     *  Keep a spectating admin out of the state their own party receives.
     *
     *  ⚠️ THIS IS THE OTHER HALF OF A FIX THAT LOOKED DONE. Suppressing Party's HUD through
     *  VigridPartyClientAPI.SetHudSuppressed only ever fixed the ADMIN'S OWN screen; the leak is on
     *  their TEAMMATES' screens, where a respawned admin kept showing as a live member with a
     *  compass caret pointing at wherever the camera had carried their body. Nothing client-side can
     *  reach that, because it is the server's state push putting the position on the wire.
     *
     *  Party is told only "do not broadcast this member" - it has no idea what spectating is, and
     *  the call is behind #ifdef VIGRID_PARTY like every other, so the mod still builds without it.
     *  Safe to call for an ordinary spectator too, and it deliberately is not: a dead teammate
     *  showing in the roster as dead is wanted behaviour, and only an admin is somewhere they are
     *  not really standing.
     */
    protected void SetPartyHidden(string uid, bool hidden)
    {
#ifdef VIGRID_PARTY
        VigridPartyAPI.SetMemberHidden(uid, hidden);
#endif
    }

    /**
     *  A player left the match. Re-resolve every spectator who was watching them.
     *
     *  Also re-resolves spectators who currently have NO target: a fresh kill may have just made a
     *  T3 candidate available.
     */
    void RetargetAfterLoss(string lost_uid)
    {
        if (m_Ended)
            return;
        if (m_Spectators.Count() == 0)
            return;

        array<string> uids = new array<string>();
        int count = m_Spectators.Count();
        int i = 0;
        for (i = 0; i < count; i++)
        {
            uids.Insert(m_Spectators.GetKey(i));
        }

        for (i = 0; i < uids.Count(); i++)
        {
            BattleRoyaleSpectatorEntry entry = m_Spectators.Get(uids.Get(i));
            if (!entry)
                continue;
            if (entry.target_uid != lost_uid && entry.target_uid != "")
                continue;

            Retarget(entry, lost_uid);
        }
    }

    /**
     *  Pick a new target for this entry. `lost_uid` is who they were watching (or who just left),
     *  and is only consulted for an admin - it is what makes "the camera follows the kill" work.
     */
    protected void Retarget(BattleRoyaleSpectatorEntry entry, string lost_uid)
    {
        if (!entry)
            return;

        string previous = entry.target_uid;

        //--- Two different resolvers, and the split is not an optimisation. ResolveTarget opens with
        //--- m_Deaths.Get(spectator_uid) to find the spectator's own party and killer chain; an admin
        //--- has no death record (they are alive, and may never have died at all), so every one of
        //--- its five tiers degrades to the T5 orbit without saying so.
        if (entry.is_admin)
        {
            entry.target_uid = ResolveAdminTarget(entry.uid, lost_uid);
            entry.resolved_tier = 0;  //--- tiers are a ResolveTarget concept; 0 reads as "admin" in the log
        }
        else
        {
            entry.target_uid = ResolveTarget(entry.uid);
            entry.resolved_tier = m_LastResolveTier;
        }

        if (entry.pending_enter)
            return;  //--- not spectating yet; BeginSpectate will push the fresh target

        PlayerIdentity identity = IdentityOfUid(entry.uid);
        if (!identity)
            return;

        Push(entry, identity);
        Notify(entry, identity);

        BattleRoyaleUtils.Info(string.Format("[Spectate] Retarget %1 lost=%2 -> %3 (T%4)", entry.uid, TargetLog(previous), TargetLog(entry.target_uid), entry.resolved_tier));
    }

    //------------------------------------------------------------------------------------------
    //--- The one driver
    //------------------------------------------------------------------------------------------

    void Tick()
    {
        //--- (0) THE AUDIENCE COUNTER, and it sits ABOVE the two early returns below DELIBERATELY.
        //--- Each of them swallows exactly the transition that matters most:
        //---
        //---   m_Spectators.Count() == 0  is the LAST spectator leaving - the one moment the player
        //---                              being watched has to be told zero. Below the guard, the
        //---                              count could be raised and never lowered.
        //---   m_Ended                    EndAll() clears the table, so at match end everybody's
        //---                              audience really is zero. Below the guard, the winner holds
        //---                              "12 watching" over the win screen for the rest of the
        //---                              session.
        //---
        //--- Nothing else has to change for either case: EndAll() empties m_Spectators, so the tally
        //--- simply comes back empty and the drain half of the pass pushes the zeros.
        //---
        //--- Self-throttled, and once its sent-map has drained the entire cost per tick is one time
        //--- comparison and two Count() calls.
        PushAudienceCounts();

        if (m_Ended)
            return;
        if (m_Spectators.Count() == 0)
            return;

        int now = GetGame().GetTime();
        int i = 0;

        //--- Walk a copy of the keys: removal during iteration is otherwise unsafe.
        array<string> uids = new array<string>();
        int count = m_Spectators.Count();
        for (i = 0; i < count; i++)
        {
            uids.Insert(m_Spectators.GetKey(i));
        }

        bool do_push = false;
        if (now >= m_NextPushMs)
        {
            m_NextPushMs = now + BR_SPECTATE_PUSH_MS;
            do_push = true;
        }

        //--- Off the 10 Hz tick on purpose: the carry pass resolves bodies by walking every Man in
        //--- the world, which is not something to do ten times a second per spectator.
        //--- BR_SPECTATE_CARRY_CORPSE is checked inside CarryCorpse rather than here, because it
        //--- governs dragging a DEAD player's loot across the map - a gameplay trade that has nothing
        //--- to say about an admin carrying their own live body under their own camera.
        //---
        //--- This ONE clock is the correlated baseline (#280): it fires for every entry on the same
        //--- tick. It is superseded per entry when the stagger is on - see the loop below - and kept
        //--- rather than deleted precisely so both behaviours are reachable within one match.
        bool do_carry = false;
        if (now >= m_NextCarryMs)
        {
            m_NextCarryMs = now + BR_SPECTATE_CARRY_INTERVAL_MS;
            do_carry = true;
        }

        //--- The measured region (#280) is the per-entry loop and nothing else: the key copy above
        //--- and the admin list below are separately throttled and are not what the issue asks about.
        //--- GetTickTime() is float SECONDS and moves within a frame - GetGame().GetTime() is integer
        //--- ms and would read 0 for a whole pass, which is how "it is free" gets concluded from an
        //--- instrument that never had the resolution to say it.
        float tick_started = GetGame().GetTickTime();
        int teleports_before = s_Teleports;

        for (i = 0; i < uids.Count(); i++)
        {
            string uid = uids.Get(i);
            BattleRoyaleSpectatorEntry entry = m_Spectators.Get(uid);
            if (!entry)
                continue;

            //--- (a) liveness. This is the PRIMARY disconnect detector: PlayerDisconnected cannot be
            //--- relied on for a client that controls no entity.
            PlayerIdentity identity = IdentityOfUid(uid);
            if (!identity)
            {
                DropEntry(uid);
                BattleRoyaleUtils.Info("[Spectate] Removed " + uid + " (identity gone)");
                continue;
            }

            //--- (b) deferred entry
            if (entry.pending_enter)
            {
                if (now < entry.enter_due_ms)
                    continue;

                BeginSpectate(entry, identity);
                continue;
            }

            //--- (c) target liveness. The catch-all: every failure mode that ends with "the target
            //--- is gone" converges here within 100 ms, whether or not its specific hook fired.
            if (entry.target_uid != "" && !LivingPlayerByUid(entry.target_uid))
            {
                Retarget(entry, entry.target_uid);
                continue;
            }

            //--- (d) keepalive. Re-delivers the target position and entity every second, so a client
            //--- that missed or could not resolve the first push simply latches on the next.
            if (do_push)
                Push(entry, identity);

            //--- (e) body carry. Keeps the replication bubble - which sits on the connection's own
            //--- entity, not on the camera - within range of what is being watched. An admin's
            //--- anchor is their own live body and it follows the CAMERA; a dead player's is their
            //--- corpse and it follows the TARGET.
            //--- TWO CLOCKS, and only one of them runs. The global one is the correlated baseline;
            //--- the per-entry one is the stagger (#280). Both are kept so that ONE match can be
            //--- measured either way - spectator, target and body counts all differ between any two
            //--- matches, so a pre/post comparison across runs would not be a comparison at all.
            //---
            //--- The admin anchor is deliberately included. It is still CALLED once per second, only
            //--- phase-shifted, so its own 5 s clock and its BR_ADMIN_ANCHOR_URGENT_M bypass keep the
            //--- latency they had; and its FindBodyByUid - which runs BEFORE that urgent check, so it
            //--- is paid every second regardless - gets spread along with everything else.
            bool carry_now = do_carry;
            if (CarryStaggered())
            {
                carry_now = false;
                if (now >= entry.next_carry_ms)
                {
                    entry.next_carry_ms = now + BR_SPECTATE_CARRY_INTERVAL_MS;
                    carry_now = true;
                }
            }

            if (carry_now)
            {
                s_CarryEvals++;

                if (entry.is_admin)
                    CarryAnchorBody(entry);
                else
                    CarryCorpse(entry);
            }
        }

        m_TickSeconds = m_TickSeconds + (GetGame().GetTickTime() - tick_started);

        int burst = s_Teleports - teleports_before;
        if (burst > s_BurstMax)
            s_BurstMax = burst;

        //--- (f) the admin overlay list. Outside the per-entry loop: it builds the roster payload
        //--- once and fans it out to whichever admins are watching, rather than rebuilding it per
        //--- admin. PushAdminList returns immediately when there are none.
        if (now >= m_NextAdminListMs)
        {
            m_NextAdminListMs = now + BR_ADMIN_CAMPOS_PUSH_MS;
            PushAdminList();
        }

        //--- (5) THE CORPSE LIST, on its own much slower clock. Deliberately NOT folded into the
        //--- 500 ms admin push above: that one carries motion and has to keep up with it, while this
        //--- one describes bodies that by definition never move again. Sending 59 names and positions
        //--- twice a second to say nothing changed would be the largest recurring payload in the mod.
        if (now >= m_NextDeadListMs)
        {
            m_NextDeadListMs = now + BR_ADMIN_DEAD_PUSH_MS;
            PushAdminDeadList(now);
        }

        ReportCarryLoad(now, uids.Count());
    }

    /**
     *  One throttled block saying what the carry pass actually costs (#280).
     *
     *  Kept rather than deleted once it answered, for the reason BattleRoyaleLobbyTags.ReportFunnel
     *  is kept: the question "is the corpse carry expensive" has at least three plausible answers -
     *  the per-frame teleport burst, the total walk volume, or nothing at all - and only a funnel
     *  separates them. It costs one integer comparison per tick once the window is open, and the
     *  whole thing is dead the moment nobody is spectating, because Tick() returns above it.
     *
     *  THREE LINES, EACH BUILT IN STEPS. The first alone carries a dozen concatenation terms, which
     *  is exactly the shape that trips "Formula too complex" - a hard compile error that packing does
     *  not catch and that only surfaces when the game loads the module.
     *
     *  Debug rather than Info, matching the per-carry line it sits beside. On a DIAG server these
     *  mirror into chat, so turn Chat Mirror off before a measurement run or the load lines evict
     *  what you were reading.
     */
    protected void ReportCarryLoad(int now, int spectators)
    {
        if (now < m_NextLoadMs)
            return;

        m_NextLoadMs = now + BR_SPECTATE_LOAD_DIAG_MS;

        string line = "[Spectate] load specs=" + spectators;
        line = line + " evals=" + s_CarryEvals;
        line = line + " tp=" + s_Teleports;
        line = line + " (first=" + s_CarryFirst;
        line = line + " step=" + s_CarryStep + ")";
        line = line + " burstmax=" + s_BurstMax;
        BattleRoyaleUtils.Debug(line);

        //--- walks/scanned. Scanned is the sum of the list LENGTHS, i.e. an upper bound on compares.
        //--- The ident column is the control: it is the same shape of work at 10 Hz instead of 1 Hz,
        //--- so a body figure far under it says the carry is not what to optimise.
        string walks = "[Spectate] load   body=" + s_BodyWalks;
        walks = walks + "/" + s_BodyScanned;
        walks = walks + " roster=" + s_RosterWalks;
        walks = walks + "/" + s_RosterScanned;
        walks = walks + " ident=" + s_IdentityWalks;
        walks = walks + "/" + s_IdentityScanned;
        BattleRoyaleUtils.Debug(walks);

        //--- The two switch states are load-bearing, not decoration: a log spanning an A/B cannot be
        //--- attributed afterwards without them, and the whole point of the toggles is to run both
        //--- conditions inside one match.
        float tick_ms = m_TickSeconds * 1000.0;

        string cost = "[Spectate] load   tickms=" + tick_ms;
        cost = cost + " window=" + BR_SPECTATE_LOAD_DIAG_MS;
        cost = cost + " carry=" + CarryEnabled();
        cost = cost + " stagger=" + CarryStaggered();
        BattleRoyaleUtils.Debug(cost);

        ResetLoadCounters();
    }

    //! Zero the window. Called after every report, and once from the constructor - the counters are
    //! static, so a second BattleRoyaleSpectators in one process would otherwise inherit the first's.
    protected void ResetLoadCounters()
    {
        s_IdentityWalks = 0;
        s_IdentityScanned = 0;
        s_RosterWalks = 0;
        s_RosterScanned = 0;
        s_BodyWalks = 0;
        s_BodyScanned = 0;
        s_Teleports = 0;
        s_CarryFirst = 0;
        s_CarryStep = 0;
        s_CarryEvals = 0;
        s_BurstMax = 0;
        m_TickSeconds = 0;
    }

    //! Report NOW and restart the window, so a measurement run can be bracketed exactly rather than
    //! waiting out the throttle and catching part of the previous condition. Diag-driven.
    void ReportCarryLoadNow()
    {
        m_NextLoadMs = 0;
        ReportCarryLoad(GetGame().GetTime(), m_Spectators.Count());
    }

    /**
     *  Move this spectator's corpse to their target, so the network bubble follows the action.
     *
     *  WHY AT ALL. The bubble is centred on the connection's own entity, which for an in-place
     *  spectator is still the corpse; UpdateSpectatorPosition does not move it. Past DayZ's default
     *  1000 m networkRangePlayers the target stops being replicated and the spectator is left
     *  watching a nametag with no character. Moving the corpse moves the bubble, measured directly,
     *  and it needs NO entity creation - which is the whole reason this works where the carrier body
     *  did not, that call having crashed a dedicated server twice.
     *
     *  WHY IT WAITS. A carried corpse leaves the world visually: the server-side position moves but
     *  the replicated one does not, so the body is not rendered anywhere and nobody can loot it. So
     *  the first carry is deferred as long as it safely can be - until the target is far enough to
     *  make it worth doing AND nobody is standing at the body - and the gear is dropped where the
     *  player fell before the body ever moves.
     *
     *  BR_SPECTATE_CARRY_FORCED_M is the backstop: someone camping a corpse must not be able to hold
     *  a spectator's view hostage all the way out to the boundary.
     */
    protected void CarryCorpse(BattleRoyaleSpectatorEntry entry)
    {
        if (!CarryEnabled())
            return;
        if (!entry)
            return;
        if (entry.target_uid == "")
            return;  //--- orbiting the circle; there is nothing to keep up with

        PlayerBase target = LivingPlayerByUid(entry.target_uid);
        if (!target)
            return;

        PlayerBase corpse = FindBodyByUid(entry.uid);
        if (!corpse)
            return;  //--- already cleaned up by the engine; nothing to carry

        vector corpse_pos = corpse.GetPosition();
        vector target_pos = target.GetPosition();
        float separation = vector.Distance(corpse_pos, target_pos);

        if (entry.corpse_carried)
        {
            //--- Already emptied and already invisible, so there is nothing left to protect and no
            //--- reason to be clever - it only has to keep up.
            if (separation < BR_SPECTATE_CARRY_STEP_M)
                return;

            //--- Logged at Debug rather than Info: this fires every ~250 m of a match-long chase and
            //--- would drown the Info stream. It exists because the first carry used to be the only
            //--- thing recorded, which made an entity=0 window impossible to explain afterwards -
            //--- "where was the corpse at 21:17:48" had no answer anywhere in the log.
            BattleRoyaleUtils.Debug(string.Format("[Spectate] Carry %1: body %2 -> %3 (%4 m)", entry.uid, corpse_pos.ToString(), target_pos.ToString(), separation));

            s_CarryStep++;
            MoveCorpse(corpse, target_pos);
            return;
        }

        if (separation < BR_SPECTATE_CARRY_TRIGGER_M)
            return;

        //--- Below the forced bound, hold off while anybody is close enough to be coming for the
        //--- loot. The failure this avoids is a player walking up to a body that vanishes as they
        //--- reach it, which reads as a bug however well documented it is.
        if (separation < BR_SPECTATE_CARRY_FORCED_M && IsAnyoneNear(corpse_pos, entry.uid))
            return;

        //--- The gear drops HERE, before the first move, which is why no position arithmetic is
        //--- needed: the body is still where the player died. DropAllItems walks the inventory and
        //--- ServerDropEntity's each item, so nothing is created and nothing is destroyed - the
        //--- items already exist and only leave the corpse. It skips anything inheriting
        //--- SurvivorBase, i.e. the body itself, and does drop clothing, which is harmless given
        //--- the corpse is about to be invisible anyway.
        corpse.DropAllItems();
        entry.corpse_carried = true;

        //--- Verify it actually emptied, because THIS FAILS SILENTLY. A locked inventory refuses
        //--- inventory moves and returns nothing to say so - the same LOCK_FROM_SCRIPT trap that made
        //--- players spawn naked, and a player who died mid-fall or mid-ladder still holds one. If it
        //--- did not empty, the gear is about to leave the match inside a body nobody can reach, and
        //--- a log line is the only thing that will ever reveal it.
        int left = CountInventory(corpse);
        if (left > 0)
            BattleRoyaleUtils.Warn(string.Format("[Spectate] Carry %1: DropAllItems left %2 item(s) on the body - they are about to leave the match. Locked inventory is the usual cause.", entry.uid, left));

        BattleRoyaleUtils.Info(string.Format("[Spectate] Carry %1: dropped gear at %2 (%3 left), body -> target at %4 (%5 m)", entry.uid, corpse_pos.ToString(), left, target_pos.ToString(), separation));

        s_CarryFirst++;
        MoveCorpse(corpse, target_pos);
    }

    /**
     *  Find a player's own CORPSE by uid.
     *
     *  LivingPlayerByUid cannot do this: it walks the current state's roster, and a dead player was
     *  removed from it at death. So this walks every Man in the world instead and matches on the
     *  cached PlayerBase.player_steamid rather than on PlayerIdentity - the same reason
     *  IsLateJoinExempt does, namely that the identity may be gone while the body is still there.
     *
     *  Returns a reference rather than storing one, so invariant 1 stands: the result is used within
     *  the call that asked for it and never kept.
     *
     *  Deliberately does NOT filter on "is dead". A uid that somehow resolves to a living body is
     *  something the caller should be able to see rather than silently get NULL for.
     */
    PlayerBase FindBodyByUid(string uid)
    {
        if (uid == "")
            return NULL;

        array<Man> everyone = new array<Man>();
        GetGame().GetPlayers(everyone);

        int count = everyone.Count();
        s_BodyWalks++;
        s_BodyScanned = s_BodyScanned + count;
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = PlayerBase.Cast(everyone.Get(i));
            if (!candidate)
                continue;
            if (candidate.player_steamid != uid)
                continue;

            return candidate;
        }

        return NULL;
    }

    /**
     *  How many items are still inside this entity, counting everything nested.
     *
     *  Only used to catch a DropAllItems that silently did nothing. LEVELORDER matches what
     *  DropAllItems itself walks, so the two agree on what "still there" means, and the SurvivorBase
     *  skip matches its skip - otherwise the body would count itself and this would never read zero.
     */
    protected int CountInventory(EntityAI entity)
    {
        if (!entity)
            return 0;
        if (!entity.GetInventory())
            return 0;

        array<EntityAI> contents = new array<EntityAI>();
        entity.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, contents);

        int total = 0;
        int count = contents.Count();
        for (int i = 0; i < count; i++)
        {
            EntityAI item = contents.Get(i);
            if (!item)
                continue;
            if (item.IsInherited(SurvivorBase))
                continue;

            total++;
        }

        return total;
    }

    //! Is any LIVING player other than the spectator themselves close to this position?
    protected bool IsAnyoneNear(vector position, string ignore_uid)
    {
        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return false;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return false;

        array<PlayerBase> roster = state.GetPlayers();
        if (!roster)
            return false;

        int count = roster.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;
            if (candidate.player_steamid == ignore_uid)
                continue;
            if (vector.Distance(candidate.GetPosition(), position) > BR_SPECTATE_CARRY_BYSTANDER_M)
                continue;

            return true;
        }

        return false;
    }

    //! Reposition a body. Same sync juncture every other teleport in the mod uses, and the one the
    //! diag probe validated this behaviour with - deliberately not a bare SetPosition.
    protected void MoveCorpse(PlayerBase corpse, vector position)
    {
        if (!corpse)
            return;

        s_Teleports++;

        position[1] = GetGame().SurfaceY(position[0], position[2]);

        ScriptJunctureData pCtx = new ScriptJunctureData;
        pCtx.Write( position );
        pCtx.Write( corpse.GetDirection() );
        corpse.SendSyncJuncture( BR_SYNC_JUNCTURE_TELEPORT, pCtx );
        corpse.SetSynchDirty();
    }

    /**
     *  The player asked to start spectating now, instead of waiting out enter_due_ms.
     *
     *  Safe by construction: the actor is the engine-supplied `sender`, never anything the client
     *  put in the payload, and the only thing this can do is start spectating for a uid that is
     *  ALREADY a registered, still-pending spectator - i.e. someone the server itself decided was
     *  dead and eligible. A living player, or a spectator who is already watching, gets nothing.
     */
    void RequestSpectate(PlayerIdentity sender)
    {
        if (m_Ended)
            return;
        if (!sender)
            return;

        string uid = sender.GetPlainId();
        if (uid == "")
            return;

        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(uid);
        if (!entry)
        {
            BattleRoyaleUtils.Warn("[Spectate] RequestSpectate from " + uid + " who is not a spectator - ignored");
            return;
        }

        if (!entry.pending_enter)
            return;  //--- already spectating; nothing to do

        BattleRoyaleUtils.Info("[Spectate] RequestSpectate accepted for " + uid);
        BeginSpectate(entry, sender);
    }

    protected void BeginSpectate(BattleRoyaleSpectatorEntry entry, PlayerIdentity identity)
    {
        if (!entry || !identity)
            return;

        //--- Resolve the chain NOW, not at death. If the killer died while this player sat on the
        //--- death screen, this is what picks up their killer instead - which is the whole point of
        //--- the chain being a chain.
        entry.target_uid = ResolveTarget(entry.uid);
        entry.resolved_tier = m_LastResolveTier;

        vector position = TargetPositionOf(entry);

        AttachSpectatorCamera(identity, position);

        entry.pending_enter = false;
        entry.entered_ms = GetGame().GetTime();

        //--- Drawn HERE, not in the constructor: the entry was built at death and has been sitting
        //--- out the death screen ever since, so a construction-time phase has long expired - and two
        //--- players who died in the same fight would re-correlate on their very first pass.
        entry.SeedCarryPhase();

        Push(entry, identity);
        Notify(entry, identity);

        BattleRoyaleUtils.Info(string.Format("[Spectate] BeginSpectate %1 class=%2 pos=%3 target=%4 (T%5)", entry.uid, BR_SPECTATE_CAM_CLASS, position.ToString(), TargetLog(entry.target_uid), entry.resolved_tier));
    }

    /**
     *  Put this connection behind a spectator camera. The two lines every entry path shares.
     *
     *  SelectPlayer(identity, NULL) DOES NOT DROP THE NETWORK BUBBLE, and do not assume otherwise
     *  from the shape of the call. The bubble stays on the connection's own entity - the corpse for
     *  a dead spectator, the live body for an admin. Measured both directions 2026-08-10 with the
     *  diag TP Target entry: the watched target is not replicated at 1200 m from where the spectator
     *  died (sustained 90 s, no recovery walking back to 1122 m) and is replicated again at 700 m,
     *  with the camera's UpdateSpectatorPosition running throughout - 507 pushes, camera always
     *  within ~4 m of the pushed position. That call is documented as "position of network bubble"
     *  and has no effect on player replication.
     *
     *  So this line is kept for what it actually does - taking the entity out of the connection's
     *  selection so the client stops treating it as its own player - and the range limit is answered
     *  by CarryAnchorBody instead. CLAUDE.md carries the full history, including why the carrier
     *  body is not the answer.
     */
    protected void AttachSpectatorCamera(PlayerIdentity identity, vector position)
    {
        if (!identity)
            return;

        GetGame().SelectPlayer(identity, NULL);
        GetGame().SelectSpectator(identity, BR_SPECTATE_CAM_CLASS, position);
    }

    //------------------------------------------------------------------------------------------

    //! Where the camera should start, and what it orbits when there is no target.
    protected vector TargetPositionOf(BattleRoyaleSpectatorEntry entry)
    {
        if (entry && entry.target_uid != "")
        {
            PlayerBase target = LivingPlayerByUid(entry.target_uid);
            if (target)
                return target.GetPosition();
        }

        return PlayAreaCentre();
    }

    /**
     *  What the camera orbits when there is nobody to follow: the centre of the FINAL circle.
     *
     *  m_PlayAreas is generated smallest-first, so index 0 is the tight endgame circle - the one
     *  circle guaranteed to exist for the whole match and to sit inside every larger one. Taking it
     *  from the registry avoids casting the current state, which matters because BattleRoyaleRound
     *  and BattleRoyaleLastRound do not share a GetZone() (LastRound extends BattleRoyaleState
     *  directly and only exposes GetPreviousZone()).
     */
    protected vector PlayAreaCentre()
    {
        if (!BattleRoyaleZone.m_PlayAreas)
            return "0 0 0";
        if (BattleRoyaleZone.m_PlayAreas.Count() == 0)
            return "0 0 0";

        BattleRoyalePlayArea final_area = BattleRoyaleZone.m_PlayAreas.Get(0);
        if (!final_area)
            return "0 0 0";

        return final_area.GetCenter();
    }

    protected void Push(BattleRoyaleSpectatorEntry entry, PlayerIdentity identity)
    {
        if (!entry || !identity)
            return;

        int mode = BR_SPECTATE_MODE_ORBIT;
        string target_name = "";
        PlayerBase target = NULL;

        if (entry.target_uid != "")
        {
            target = LivingPlayerByUid(entry.target_uid);
            if (target)
            {
                mode = BR_SPECTATE_MODE_FOLLOW;
                target_name = target.player_name;
                if (target_name == "" && target.GetIdentity())
                    target_name = target.GetIdentity().GetName();
            }
        }

        //--- FREE overrides whatever the target situation is - the camera is flying and is not
        //--- anchored to anybody. The target is still resolved and still pushed, because the client
        //--- highlights it in the overlay and cycling has to keep working while free-flying.
        if (entry.is_admin && entry.mode == BR_SPECTATE_MODE_FREE)
            mode = BR_SPECTATE_MODE_FREE;
        else
            entry.mode = mode;  //--- keep the entry honest for anything that reads it later

        vector position = TargetPositionOf(entry);

        //--- `target` is the CF Object argument, marshalled by network id. At the first push the
        //--- spectator's bubble may still be at their corpse, so it can arrive NULL on the client -
        //--- which is why the position travels in the payload too and the client has a proximity
        //--- fallback. Total failure of Object marshalling degrades to a 1 Hz position follow.
        GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetSpectateTarget", new Param4<string, string, vector, int>(entry.target_uid, target_name, position, mode), true, identity, target);
    }

    protected void Notify(BattleRoyaleSpectatorEntry entry, PlayerIdentity identity)
    {
        if (!entry || !identity)
            return;

        //--- MessagePlayerUntranslated cannot be used: it needs a PlayerBase in m_Players, and a
        //--- spectator deliberately is not one. The bare key is localized by the client in
        //--- BattleRoyaleRPC.NotificationMessage.
        if (entry.target_uid == "")
        {
            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>("STR_BR_SPECTATE_NO_TARGET", DAYZBR_MSG_TIME, "", "", "", "", ""), true, identity);
            return;
        }

        string target_name = "";
        PlayerBase target = LivingPlayerByUid(entry.target_uid);
        if (target)
        {
            target_name = target.player_name;
            if (target_name == "" && target.GetIdentity())
                target_name = target.GetIdentity().GetName();
        }

        GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>("STR_BR_SPECTATE_FOLLOWING", DAYZBR_MSG_TIME, target_name, "", "", "", ""), true, identity);
    }

    /**
     *  The match is over. Tell every spectator, and stop doing anything further.
     *
     *  The camera is deliberately NOT deactivated: with no player entity to fall back to,
     *  SetActive(false) renders nothing. It stays where it is, orbiting its last anchor, and the
     *  ESC menu offers Continue and Exit.
     */
    void EndAll()
    {
        if (m_Ended)
            return;

        int count = m_Spectators.Count();
        int i = 0;

        //--- Hand admin bodies back. Without this an admin is left in a camera with no way out: the
        //--- clear below drops their session, AdminToggle is guarded on m_Ended and would refuse, and
        //--- they are not dead so there is no death screen to quit from either. Runs first and in two
        //--- passes rather than inside the notify loop, because EndAdminSpectate mutates the map.
        array<string> admin_uids = new array<string>();
        for (i = 0; i < count; i++)
        {
            BattleRoyaleSpectatorEntry admin_entry = m_Spectators.GetElement(i);
            if (admin_entry && admin_entry.is_admin)
                admin_uids.Insert(admin_entry.uid);
        }

        for (i = 0; i < admin_uids.Count(); i++)
        {
            PlayerIdentity admin_identity = IdentityOfUid(admin_uids.Get(i));
            if (admin_identity)
                EndAdminSpectate(admin_identity);
        }

        m_Ended = true;

        count = m_Spectators.Count();
        for (i = 0; i < count; i++)
        {
            BattleRoyaleSpectatorEntry entry = m_Spectators.GetElement(i);
            if (!entry)
                continue;

            PlayerIdentity identity = IdentityOfUid(entry.uid);
            if (!identity)
                continue;

            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "EndSpectate", NULL, true, identity);
            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>("STR_BR_SPECTATE_ENDED", DAYZBR_MSG_TIME, "", "", "", "", ""), true, identity);
        }

        BattleRoyaleUtils.Info(string.Format("[Spectate] EndAll (%1 spectators)", count));

        //--- Release any party hide still standing before the map goes, since Clear() is the one
        //--- removal that does not run through DropEntry. The loop above cannot be trusted to have
        //--- done it: EndAdminSpectate is skipped for an admin whose identity has already gone, and
        //--- that is precisely the admin who would come back invisible to their party.
        for (i = 0; i < count; i++)
        {
            BattleRoyaleSpectatorEntry stale = m_Spectators.GetElement(i);
            if (!stale)
                continue;

            SetPartyHidden(stale.uid, false);
        }

        m_Spectators.Clear();
    }

    protected string TargetLog(string uid)
    {
        if (uid == "")
            return "none";

        return uid;
    }

    //------------------------------------------------------------------------------------------
    //--- ADMIN SPECTATE
    //---
    //--- One rule, stated once in AdminEligibility and never re-derived: admin spectate requires a
    //--- NON-PARTICIPANT - alive, holding a body, absent from m_Players.
    //------------------------------------------------------------------------------------------

    /**
     *  What may this identity do right now? The whole lifecycle, in one place.
     *
     *  Every admin RPC opens with this rather than assembling its own checks, so there is exactly one
     *  definition of "may spectate" and no way for two entry points to disagree - which is the bug
     *  the late-join admin exemption already had once, when OnPlayerConnected and OnPlayerTick
     *  each decided for themselves.
     */
    int AdminEligibility(PlayerIdentity identity)
    {
        if (!BattleRoyaleServer.IsAdminIdentity(identity))
            return BR_ADMIN_REFUSED_NOT_ADMIN;

        if (!BattleRoyaleConfig.GetConfig().GetGameData().admin_spectate_enabled)
            return BR_ADMIN_REFUSED_NOT_ADMIN;

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return BR_ADMIN_REFUSED_NOT_ADMIN;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return BR_ADMIN_REFUSED_NOT_ADMIN;

        //--- A real admin in the wrong phase. Its own verdict rather than folding into NOT_ADMIN,
        //--- so it can be explained on screen: NOT_ADMIN is answered silently by design, which made
        //--- pressing F3 in the lobby indistinguishable from a broken key.
        if (!state.AllowsSpectate())
            return BR_ADMIN_REFUSED_PHASE;

        //--- COMPETING. The roster is the authority, and this is the check the whole feature turns
        //--- on: an admin who is playing the match stays playing it.
        if (state.GetPlayerFromIdentity(identity))
            return BR_ADMIN_REFUSED_COMPETING;

        string uid = identity.GetPlainId();
        if (uid == "")
            return BR_ADMIN_REFUSED_NOT_ADMIN;

        //--- Not competing and holding a live body: mid-match joiner, or already respawned.
        PlayerBase body = FindBodyByUid(uid);
        if (body && body.IsAlive())
            return BR_ADMIN_ALLOW_SPECTATE;

        return BR_ADMIN_OFFER_RESPAWN;
    }

    //! Is this uid currently in an ADMIN spectate session (as opposed to an ordinary death one)?
    bool IsAdminSpectator(string uid)
    {
        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(uid);
        if (!entry)
            return false;

        return entry.is_admin;
    }

    /**
     *  Hide or restore the admin's anchor body - the live character being carried under the camera.
     *
     *  WHY THIS IS NOT `SetInvisibleRecursive`. That was tried, shipped and measured on 2026-08-11:
     *  other players could still see the body, and the admin could see their own standing beside
     *  whoever they were following. `SetInvisible` looks to be a LOCAL RENDER FLAG rather than
     *  replicated state - COT only ever calls it client-side, on the local player's own model
     *  (JM/COT/.../Player/DayZPlayerImplement.c:208) - so setting it server-side hides the body from
     *  nobody at all, silently.
     *
     *  COT's `COTSetInvisibility` is the route that actually replicates: it writes
     *  `m_JMIsInvisibleRemoteSynch` and calls `SetSynchDirty()`, which is precisely the half that
     *  was missing. This is a plain API call into a hard dependency, not adapted code, so it carries
     *  no obligation under COT's CC BY-SA licence.
     *
     *  `Interactive` rather than `DisableSimulation`, deliberately. This body is teleported every
     *  time the camera drifts, and control is handed back to it on exit - and the crash already
     *  fixed in EndAdminSpectate came from vanilla's camera initialising against a body that had
     *  never been simulated. Turning simulation off would push it further in that direction for no
     *  gain: invisibility is all that is wanted here.
     *
     *  Behind #ifdef JM_COT because that define comes from COT itself. COT is a hard dependency of
     *  this mod, so in practice it is always present; a build without it falls back to a visible
     *  body rather than failing to compile.
     */
    protected void HideAnchorBody(PlayerBase body, bool hidden)
    {
        if (!body)
            return;

        //--- Vanilla, and independent of the above: a parked body nobody can see must not be
        //--- killable by whatever it is dropped on top of.
        body.SetAllowDamage(!hidden);

#ifdef JM_COT
        if (hidden)
            body.COTSetInvisibility(JMInvisibilityType.Interactive);
        else
            body.COTSetInvisibility(JMInvisibilityType.None);
#else
        //--- No COT: say so once rather than leaving "the admin is visible" as a silent mystery.
        if (hidden)
            BattleRoyaleUtils.Warn("[Spectate] No JM_COT - the admin's body will be VISIBLE to other players while spectating");
#endif
    }

    //! Is anybody at all in an admin session? Gates the overlay push out of the common path.
    protected bool HasAdminSpectator()
    {
        int count = m_Spectators.Count();
        for (int i = 0; i < count; i++)
        {
            BattleRoyaleSpectatorEntry entry = m_Spectators.GetElement(i);
            if (entry && entry.is_admin)
                return true;
        }

        return false;
    }

    /**
     *  The one admin entry point: F3, and the death screen's admin button.
     *
     *  Deliberately ONE action rather than three, because what the admin wants is always "get me to
     *  the camera" or "get me out of it", and which mechanical step that needs is the server's
     *  problem, not something to make them press keys in the right order for. So:
     *
     *    already spectating          -> leave, and hand the body back
     *    eligible, holding a body    -> enter
     *    eligible but dead           -> respawn AND enter, in one press
     *    competing                   -> refuse, and say why
     */
    void AdminToggle(PlayerIdentity sender)
    {
        if (m_Ended)
            return;
        if (!sender)
            return;

        string uid = sender.GetPlainId();
        if (uid == "")
            return;

        //--- Leaving does not need eligibility: an admin who is already flying must always be able to
        //--- get out, even if the state moved on or the setting was turned off underneath them.
        if (IsAdminSpectator(uid))
        {
            EndAdminSpectate(sender);
            return;
        }

        int verdict = AdminEligibility(sender);

        if (verdict == BR_ADMIN_REFUSED_NOT_ADMIN)
        {
            BattleRoyaleUtils.Warn("[Spectate] Rejected AdminToggle from " + BattleRoyaleServer.GetIdentityLogName(sender));
            return;
        }

        if (verdict == BR_ADMIN_REFUSED_PHASE)
        {
            BattleRoyaleUtils.Info("[Spectate] AdminToggle refused for " + uid + ": state does not allow spectating");
            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>("STR_BR_SPECTATE_ADMIN_PHASE", DAYZBR_MSG_TIME, "", "", "", "", ""), true, sender);
            return;
        }

        if (verdict == BR_ADMIN_REFUSED_COMPETING)
        {
            BattleRoyaleUtils.Info("[Spectate] AdminToggle refused for " + uid + ": still competing");
            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>("STR_BR_SPECTATE_ADMIN_COMPETING", DAYZBR_MSG_TIME, "", "", "", "", ""), true, sender);
            return;
        }

        if (verdict == BR_ADMIN_OFFER_RESPAWN)
        {
            if (!AdminRespawn(sender))
                return;

            //--- Fall through into the camera in the same press. AdminRespawn left them alive,
            //--- non-participant and holding a body, which is exactly BeginAdminSpectate's precondition.
        }

        BeginAdminSpectate(sender);
    }

    /**
     *  Give a dead admin a fresh body, so they become a non-participant and can then spectate.
     *
     *  THE THREE LINES ARE VANILLA'S OWN, from MissionServer.CreateCharacter
     *  (P:\scripts\5_mission\mission\missionserver.c:486-495). Called directly rather than through
     *  the mission, for two reasons: CreateCharacter writes the mission's `m_player` member as a side
     *  effect, and this mod overrides EquipCharacter to apply the LOBBY loadout, which is wrong here.
     *
     *  CLAUDE.md records two crashes around player creation and neither applies. CreateObjectEx
     *  faulted at 0x0, and CreatePlayer faulted at 0x9 with a NULL identity - which is out of
     *  contract, its doc being "assign player entity to client" and vanilla's only null call site
     *  being missionbenchmark.c:366, a mission with no clients. This one passes a real, connected
     *  identity. The second recorded objection - "the body would be ALIVE and outside the state, so
     *  OnPlayerTick force-logs-it-out" - was about the carrier body; here alive-and-outside is the
     *  goal, and BattleRoyaleServer.IsLateJoinExempt covers admins.
     *
     *  NOTHING ABOUT THE MATCH CHANGES. The new body is never added to m_Players, so the roster
     *  count, every IsComplete() and br_position are untouched; the admin's placement, leaderboard
     *  entry and death record all stand, and their corpse and its loot stay where they fell.
     */
    bool AdminRespawn(PlayerIdentity identity)
    {
        if (m_Ended)
            return false;
        if (!identity)
            return false;

        string uid = identity.GetPlainId();
        if (uid == "")
            return false;

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return false;

        //--- Same placement a mid-match admin connect gets: the circle actually in play, falling back
        //--- to the lobby centre when no circle is live yet. Shared helper rather than a second copy
        //--- of the fallback, so the two entry points cannot disagree about where an admin belongs.
        vector position = "0 0 0";
        server.GetAdminSpawnPosition(position);

        Entity player_ent = GetGame().CreatePlayer(identity, BR_ADMIN_RESPAWN_CHARACTER, position, 0, "NONE");
        PlayerBase body = PlayerBase.Cast(player_ent);
        if (!body)
        {
            BattleRoyaleUtils.Warn("[Spectate] AdminRespawn " + uid + ": CreatePlayer returned nothing for " + BR_ADMIN_RESPAWN_CHARACTER);
            return false;
        }

        GetGame().SelectPlayer(identity, body);

        //--- Set by hand because nothing else does outside OnPlayerConnected, and FindBodyByUid
        //--- matches on player_steamid - without this the body this method just made is invisible to
        //--- every uid->body lookup in the file, including AdminEligibility's.
        body.player_steamid = uid;
        body.player_name = BattleRoyaleNameService.ResolveIdentity(identity);

        //--- They hold no state, so OnPlayerTick's not-in-state branch would schedule a kick. The
        //--- exemption is the same list a mid-match admin connect uses. IsLateJoinExempt now also
        //--- consults admins_steamid64 directly, so this is belt-and-braces rather than the only
        //--- thing standing between this body and an immediate disconnect - but it keeps the list
        //--- honest for anything that reads it.
        server.ExemptFromLateJoinKick(uid);

        //--- They are no longer an ordinary death spectator. Their death RECORD stays - it is a true
        //--- record and the killer chain still needs it - but the session goes.
        if (m_Spectators.Contains(uid))
            DropEntry(uid);

        //--- Undo the death-time deafening. They are a moderator now, not a dead player who might
        //--- relay what they see.
        GetGame().MuteAllPlayers(uid, false);

        //--- Deliberately NO EndSpectate RPC here. The caller enters the camera on the very next
        //--- line, whose Push sets spectate_active and closes the death screen anyway - and racing
        //--- an EndSpectate against that Push risks the two landing in the wrong order and leaving
        //--- the client believing it is not spectating when it is. The death screen closes itself
        //--- for a living player regardless (DeathScreenMenu.Tick), which covers the case where
        //--- entering the camera fails after the body was already created.
        BattleRoyaleUtils.Info(string.Format("[Spectate] AdminRespawn %1 as %2 at %3", uid, BR_ADMIN_RESPAWN_CHARACTER, position.ToString()));
        return true;
    }

    /**
     *  Start an admin session. The caller has already established eligibility.
     *
     *  Unlike OnDeath there is no deferred entry and no offer: the admin asked for this, so it
     *  happens now.
     */
    bool BeginAdminSpectate(PlayerIdentity identity)
    {
        if (m_Ended)
            return false;
        if (!identity)
            return false;

        string uid = identity.GetPlainId();
        if (uid == "")
            return false;

        if (m_Spectators.Contains(uid))
        {
            BattleRoyaleUtils.Warn("[Spectate] BeginAdminSpectate: " + uid + " already has a session");
            return false;
        }

        PlayerBase body = FindBodyByUid(uid);
        if (!body)
        {
            BattleRoyaleUtils.Warn("[Spectate] BeginAdminSpectate: " + uid + " has no body");
            return false;
        }

        string admin_name = body.player_name;
        if (admin_name == "")
            admin_name = identity.GetName();

        HideAnchorBody(body, true);

        BattleRoyaleSpectatorEntry entry = new BattleRoyaleSpectatorEntry(uid, admin_name);
        entry.is_admin = true;
        entry.pending_enter = false;   //--- no death screen to wait out
        entry.entered_ms = GetGame().GetTime();
        entry.SeedCarryPhase();
        entry.cam_pos = body.GetPosition();
        entry.target_uid = ResolveAdminTarget(uid, "");
        entry.mode = BR_SPECTATE_MODE_FOLLOW;
        m_Spectators.Set(uid, entry);

        //--- Out of their party's state feed for the duration. The anchor body is about to start
        //--- following the camera, and every position it takes would otherwise be broadcast to their
        //--- teammates as a live member - a compass caret pointing straight at wherever the admin
        //--- chose to watch from. Released by DropEntry on every exit path.
        SetPartyHidden(uid, true);

        AttachSpectatorCamera(identity, TargetPositionOf(entry));

        Push(entry, identity);
        Notify(entry, identity);

        BattleRoyaleUtils.Info(string.Format("[Spectate] BeginAdminSpectate %1 (%2) target=%3", uid, admin_name, TargetLog(entry.target_uid)));
        return true;
    }

    /**
     *  End an admin session and hand the body back.
     *
     *  The body is moved to the camera first, so leaving the camera puts the admin where they were
     *  looking rather than back where they started - which is the whole point of being able to fly
     *  there. Uses the same juncture as every other teleport in the mod.
     */
    bool EndAdminSpectate(PlayerIdentity identity)
    {
        if (!identity)
            return false;

        string uid = identity.GetPlainId();
        if (uid == "")
            return false;

        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(uid);
        if (!entry || !entry.is_admin)
            return false;

        //--- IsAlive as well as non-null. The body is carried around under the camera and is not
        //--- replicated, so nobody can shoot it - but it can still drown, fall or be caught by a
        //--- scripted damage source, and handing a corpse back would drop the admin into a dead
        //--- body with no death screen and no way out. Treated as "gone" so the next toggle offers a
        //--- respawn, which is the path that does work.
        PlayerBase body = FindBodyByUid(uid);
        if (!body || !body.IsAlive())
        {
            DropEntry(uid);
            BattleRoyaleUtils.Warn("[Spectate] EndAdminSpectate " + uid + ": body is gone or dead, session dropped");
            return false;
        }

        //--- Copied out BEFORE the Remove below. m_Spectators holds the only strong reference to the
        //--- entry, so removing it frees the object and `entry` becomes a dangling weak local - the
        //--- log line at the bottom of this method used to read entry.cam_pos AFTER the Remove,
        //--- which is a use-after-free that happened to print correct values most of the time.
        vector exit_pos = entry.cam_pos;

        //--- Leaving the camera puts the admin where they were LOOKING, not back where they started -
        //--- which is the whole point of being able to fly there.
        if (exit_pos != "0 0 0")
            MoveCorpse(body, exit_pos);

        //--- BEFORE handing control back - an admin who reappeared as an invisible invincible player
        //--- would be a far worse bug than the visible body this fixes.
        HideAnchorBody(body, false);

        //--- POSITION FIRST, CONTROL A TICK LATER, and the gap is deliberate.
        //---
        //--- Handing the body back in the same frame as the juncture teleport crashed the CLIENT
        //--- outright: "Access violation ... at 0x74" in vanilla
        //--- DayZPlayerCamera1stPerson.UpdateUDAngleUnlocked (dayzplayercamera_base.c:132), 2026-08-11.
        //--- That body has never been simulated - it was created by CreatePlayer and dropped from
        //--- the connection's selection immediately - so vanilla's first-person camera initialises
        //--- against a player that is mid-juncture and has no command state yet.
        //---
        //--- The teleport is left where it is and only SelectPlayer moves, because the position has
        //--- to be authoritative before the client starts predicting from it. Same spirit as
        //--- BR_NotifyTeleported: never resync a player inside the frame that moved them.
        //---
        //--- DropEntry rather than a bare Remove: it also puts the admin back into their party's
        //--- state feed, which is the one side effect of a session that outlives the session itself.
        DropEntry(uid);

        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(FinishAdminExit, BR_ADMIN_EXIT_SELECT_DELAY_MS, false, identity, body);

        GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "EndSpectate", NULL, true, identity);

        BattleRoyaleUtils.Info(string.Format("[Spectate] EndAdminSpectate %1 at %2", uid, exit_pos.ToString()));
        return true;
    }

    //! The deferred half of EndAdminSpectate - see the comment there. Re-checks both arguments,
    //! because a disconnect inside the delay window frees either of them.
    void FinishAdminExit(PlayerIdentity identity, PlayerBase body)
    {
        if (!identity || !body)
        {
            BattleRoyaleUtils.Warn("[Spectate] FinishAdminExit: identity or body gone, cannot hand control back");
            return;
        }

        GetGame().SelectPlayer(identity, body);
        BattleRoyaleUtils.Info("[Spectate] FinishAdminExit: control returned to " + identity.GetPlainId());
        return;
    }

    /**
     *  Who should an admin watch?
     *
     *  Deliberately NOT a sixth tier on ResolveTarget, which opens with m_Deaths.Get(spectator_uid)
     *  to find the spectator's own party and killer chain. An admin has no death record - they are
     *  alive, and may never have died at all - so every one of those five tiers degrades to the T5
     *  orbit without saying so.
     *
     *  Order:
     *    1. THE KILLER of whoever just died, if they are still alive. This is the "follow the fight"
     *       behaviour: watch someone lose a gunfight and the camera stays on the winner.
     *    2. The next living player in cycle order, so a zone death or a disconnect still lands
     *       somewhere useful rather than on the empty circle.
     *    3. "" - orbit the final circle, when nobody is left to watch.
     */
    protected string ResolveAdminTarget(string uid, string lost_uid)
    {
        if (lost_uid != "")
        {
            BattleRoyaleDeathRecord record = m_Deaths.Get(lost_uid);
            if (record && record.killer_uid != "" && record.killer_uid != lost_uid)
            {
                if (LivingPlayerByUid(record.killer_uid))
                {
                    BattleRoyaleUtils.Debug(string.Format("[Spectate] Admin %1 follows the killer of %2 -> %3", uid, lost_uid, record.killer_uid));
                    return record.killer_uid;
                }
            }
        }

        array<string> living = new array<string>();
        BuildCycleList(living);
        if (living.Count() == 0)
            return "";

        return living.Get(0);
    }

    /**
     *  Every living player, in a STABLE order.
     *
     *  Sorted by uid rather than left in roster order, because the roster is compacted as people die
     *  and an unsorted list would silently renumber itself under the admin - pressing Next twice
     *  could land back where it started. Sorting a string array is enough: uids are fixed-width
     *  SteamID64s, so lexicographic and numeric order agree.
     */
    protected void BuildCycleList(out array<string> living)
    {
        living.Clear();

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return;

        array<PlayerBase> roster = state.GetPlayers();
        if (!roster)
            return;

        int count = roster.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            string candidate_uid = candidate.GetIdentity().GetPlainId();
            if (candidate_uid == "")
                continue;

            living.Insert(candidate_uid);
        }

        living.Sort();
    }

    /**
     *  Step an admin's target forwards or backwards through the living players.
     *
     *  Wraps. A target that is no longer in the list - they died between the keypress and here -
     *  restarts from the top rather than refusing, since refusing would leave the admin pressing a
     *  key that appears to do nothing.
     */
    void CycleTarget(PlayerIdentity identity, int direction)
    {
        if (m_Ended)
            return;
        if (!identity)
            return;

        string uid = identity.GetPlainId();
        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(uid);
        if (!entry || !entry.is_admin)
            return;

        array<string> living = new array<string>();
        BuildCycleList(living);

        int count = living.Count();
        if (count == 0)
        {
            entry.target_uid = "";
            Push(entry, identity);
            Notify(entry, identity);
            return;
        }

        int index = living.Find(entry.target_uid);
        if (index == -1)
        {
            index = 0;
        }
        else
        {
            index = index + direction;

            //--- Not a modulo: EnfusionScript's % on a negative left operand is not something to
            //--- rely on, and there are only ever two ways off the end of the list.
            if (index < 0)
                index = count - 1;
            if (index >= count)
                index = 0;
        }

        entry.target_uid = living.Get(index);
        entry.resolved_tier = 0;

        Push(entry, identity);
        Notify(entry, identity);

        BattleRoyaleUtils.Debug(string.Format("[Spectate] Admin %1 cycled %2 -> %3 (%4/%5)", uid, direction, TargetLog(entry.target_uid), index + 1, count));
    }

    //! Switch an admin between FOLLOW and FREE. Server-authoritative: the client asks, the camera
    //! changes only when the new mode comes back down in SetSpectateTarget.
    void SetAdminMode(PlayerIdentity identity, int mode)
    {
        if (m_Ended)
            return;
        if (!identity)
            return;
        if (mode != BR_SPECTATE_MODE_FOLLOW && mode != BR_SPECTATE_MODE_FREE)
            return;

        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(identity.GetPlainId());
        if (!entry || !entry.is_admin)
            return;

        entry.mode = mode;

        //--- Entering FREE seeds the camera position from wherever the follow camera was, so the
        //--- body carry has something sane before the client's first report arrives.
        if (mode == BR_SPECTATE_MODE_FREE && entry.cam_pos == "0 0 0")
            entry.cam_pos = TargetPositionOf(entry);

        //--- Leaving FREE snaps to whoever the camera is NEAREST, rather than resuming whoever was
        //--- being followed before. An admin flies somewhere specific to look at something specific,
        //--- so "follow what I flew to" is the useful reading of the key - resuming a target on the
        //--- far side of the map would throw away the whole reason they flew there.
        if (mode == BR_SPECTATE_MODE_FOLLOW && entry.cam_pos != "0 0 0")
        {
            string nearest = NearestLivingUidTo(entry.cam_pos);
            if (nearest != "")
                entry.target_uid = nearest;
        }

        Push(entry, identity);

        //--- Feedback, because F5 is otherwise a key that visibly does nothing when the camera
        //--- happens to be pointing somewhere similar in both modes.
        string mode_key = "STR_BR_SPECTATE_ADMIN_FOLLOW";
        if (mode == BR_SPECTATE_MODE_FREE)
            mode_key = "STR_BR_SPECTATE_ADMIN_FREE";

        GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>(mode_key, DAYZBR_MSG_TIME, "", "", "", "", ""), true, identity);

        BattleRoyaleUtils.Debug(string.Format("[Spectate] Admin %1 mode -> %2", entry.uid, mode));
    }

    //! The client reporting where its free camera is, so the body can follow. ~2 Hz.
    void SetAdminCamPos(PlayerIdentity identity, vector position)
    {
        if (m_Ended)
            return;
        if (!identity)
            return;

        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(identity.GetPlainId());
        if (!entry || !entry.is_admin)
            return;

        entry.cam_pos = position;
    }

    /**
     *  Keep an admin's own body under their camera, so the replication bubble follows the view.
     *
     *  Same mechanism as CarryCorpse and the same measured justification - the bubble sits on the
     *  connection's entity and UpdateSpectatorPosition does not move it - but three things differ,
     *  and all three are because this body is ALIVE and belongs to the person flying the camera:
     *
     *    - the destination is the CAMERA, not a target. A free camera has no target to chase.
     *    - the gear is NOT dropped. It is the admin's own kit and they get it back on exit.
     *    - there is no bystander check and no forced bound. Nobody is coming to loot a live admin.
     *
     *  DO NOT assume this body is invisible the way a carried corpse is. That measurement was taken
     *  on a CORPSE; a live body is still simulated, so its teleports replicate and other players
     *  watch it slide across the map. BeginAdminSpectate hides it explicitly instead.
     */
    protected void CarryAnchorBody(BattleRoyaleSpectatorEntry entry)
    {
        if (!entry)
            return;
        if (entry.cam_pos == "0 0 0")
            return;  //--- no camera report yet

        PlayerBase body = FindBodyByUid(entry.uid);
        if (!body)
            return;

        vector body_pos = body.GetPosition();
        int now = GetGame().GetTime();

        //--- TWO CLOCKS, and the second is what makes the lazy one safe. Ordinarily the body is
        //--- re-placed at most every BR_ADMIN_ANCHOR_INTERVAL_MS, because each move is a sync
        //--- juncture on a live entity and the best position changes slowly. But a free camera at
        //--- the top speed step covers over 200 m/s - far enough in one interval to leave its own
        //--- bubble behind - so drifting past BR_ADMIN_ANCHOR_URGENT_M overrides the wait.
        bool urgent = vector.Distance(body_pos, entry.cam_pos) >= BR_ADMIN_ANCHOR_URGENT_M;
        if (!urgent && now < entry.next_anchor_ms)
            return;

        entry.next_anchor_ms = now + BR_ADMIN_ANCHOR_INTERVAL_MS;

        int covered = 0;
        vector wanted = ChooseAnchorPosition(entry.cam_pos, covered);

        //--- Hysteresis. Without it the body chases every small change in the best answer, which is
        //--- a juncture per tick for no visible gain.
        float move_distance = vector.Distance(body_pos, wanted);
        if (move_distance < BR_ADMIN_ANCHOR_STEP_M)
            return;

        BattleRoyaleUtils.Debug(string.Format("[Spectate] Anchor admin %1: body %2 -> %3 (%4 m, covers %5 player(s), urgent=%6)", entry.uid, body_pos.ToString(), wanted.ToString(), move_distance, covered, urgent));

        MoveCorpse(body, wanted);
    }

    /**
     *  Where to park the admin's body so the bubble holds as many players as possible.
     *
     *  The naive answer - put it on the camera - wastes most of the bubble whenever the camera is
     *  at the edge of a group, and the admin then cannot see or HEAR players who are only a few
     *  hundred metres from the ones they are watching. Voice and replication are both keyed to this
     *  entity, not to the camera, so moving it off-camera towards the crowd buys both at once.
     *
     *  Constraints, in priority order:
     *    1. The CAMERA must stay comfortably inside the bubble. Optimising coverage of players the
     *       admin cannot actually see would be worse than useless, so every candidate is capped at
     *       BR_ADMIN_ANCHOR_MAX_OFFSET_M from the camera.
     *    2. Cover as many living players as possible within BR_ADMIN_ANCHOR_COVER_M.
     *    3. Stay at least BR_ADMIN_ANCHOR_MIN_PLAYER_M from any one of them.
     *
     *  Constraint 3 is a PREFERENCE, not a hard rule: in a shrinking final circle every point is
     *  near somebody, and refusing to place the body at all would cost the admin the whole view.
     *  The search therefore runs twice, and the second pass drops it.
     *
     *  Candidates are the camera itself plus four points along the line from the camera to the
     *  centroid of the players it can see. A line rather than a grid because the useful direction
     *  is always "towards the crowd", and a 5-point line costs 5 * N distance checks where a sweep
     *  would cost hundreds - this runs on the server with every player in the match in scope.
     */
    protected vector ChooseAnchorPosition(vector camera_pos, out int covered)
    {
        covered = 0;

        array<vector> positions = new array<vector>();
        CollectLivingPositions(positions);
        if (positions.Count() == 0)
            return camera_pos;

        //--- Centroid of the players already within reach of the camera. Everyone else is beyond
        //--- saving from here, and including them would drag the anchor towards the far side of the
        //--- map and lose the players the admin is actually watching.
        vector sum = "0 0 0";
        int near_count = 0;
        int i = 0;
        for (i = 0; i < positions.Count(); i++)
        {
            if (vector.Distance(positions.Get(i), camera_pos) > BR_ADMIN_ANCHOR_COVER_M)
                continue;

            sum = sum + positions.Get(i);
            near_count = near_count + 1;
        }

        if (near_count == 0)
            return camera_pos;

        //--- Componentwise, NOT `sum / near_count`: EnfusionScript has no vector/float divide
        //--- operator. Same trap the camera boom hit, which is why it uses Normalized().
        float inv = 1.0 / near_count;
        vector centroid = Vector(sum[0] * inv, sum[1] * inv, sum[2] * inv);

        //--- Clamp the pull towards the centroid, so the camera never leaves the bubble.
        vector offset = centroid - camera_pos;
        offset[1] = 0;
        float offset_len = offset.Length();
        if (offset_len > BR_ADMIN_ANCHOR_MAX_OFFSET_M)
            offset = offset.Normalized() * BR_ADMIN_ANCHOR_MAX_OFFSET_M;

        vector best = camera_pos;
        int best_score = -1;
        float best_clearance = 0;

        //--- Two passes: honour the minimum spacing, then give it up rather than give up entirely.
        int pass = 0;
        for (pass = 0; pass < 2; pass++)
        {
            bool enforce_spacing = (pass == 0);

            int step = 0;
            for (step = 0; step <= 4; step++)
            {
                vector candidate = camera_pos + (offset * (step * 0.25));
                candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);

                int score = 0;
                float clearance = 100000;

                int p = 0;
                for (p = 0; p < positions.Count(); p++)
                {
                    float distance = vector.Distance(positions.Get(p), candidate);
                    if (distance <= BR_ADMIN_ANCHOR_COVER_M)
                        score = score + 1;
                    if (distance < clearance)
                        clearance = distance;
                }

                if (enforce_spacing && clearance < BR_ADMIN_ANCHOR_MIN_PLAYER_M)
                    continue;

                //--- Coverage first; among equal coverage prefer the spot furthest from anybody, so
                //--- the admin drifts to the quiet edge of a group rather than its middle.
                if (score < best_score)
                    continue;
                if (score == best_score && clearance <= best_clearance)
                    continue;

                best_score = score;
                best_clearance = clearance;
                best = candidate;
            }

            if (best_score >= 0)
                break;  //--- pass 0 found something; no need to relax the spacing
        }

        if (best_score < 0)
            return camera_pos;

        covered = best_score;
        return best;
    }

    /**
     *  The living player closest to a point, or "" when the roster is empty.
     *
     *  Distinct from NearestLivingUid, which measures from a spectator's DEATH POSITION and is part
     *  of the five-tier chain. This one takes an arbitrary point, because what an admin wants is
     *  "nearest to where I am now", and an admin has no death position to measure from.
     */
    protected string NearestLivingUidTo(vector origin)
    {
        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return "";

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return "";

        array<PlayerBase> roster = state.GetPlayers();
        if (!roster)
            return "";

        string best_uid = "";
        float best_distance = 0;

        int count = roster.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            string candidate_uid = candidate.GetIdentity().GetPlainId();
            if (candidate_uid == "")
                continue;

            float distance = vector.Distance(candidate.GetPosition(), origin);
            if (best_uid != "" && distance >= best_distance)
                continue;

            best_uid = candidate_uid;
            best_distance = distance;
        }

        return best_uid;
    }

    //! Positions of every living roster member. Positions only - nothing here needs identity, and
    //! copying them out keeps the scoring loop off the roster array.
    protected void CollectLivingPositions(out array<vector> positions)
    {
        positions.Clear();

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return;

        array<PlayerBase> roster = state.GetPlayers();
        if (!roster)
            return;

        int count = roster.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;

            positions.Insert(candidate.GetPosition());
        }
    }

    /**
     *  Build the admin overlay payload and send it to every admin currently spectating.
     *
     *  PER-IDENTITY, NEVER BROADCAST, and that is not incidental. This carries SteamID64s, which
     *  SetLeaderboard deliberately refuses to put on the wire precisely because it is a broadcast -
     *  "shipping SteamID64s to every client would be pure liability". Sending them only to a
     *  connection the server has already established is an admin is what makes it acceptable here.
     *
     *  Bounded by BR_ADMIN_LIST_MAX by construction: this mod has no RPC chunking anywhere.
     */
    protected void PushAdminList()
    {
        //--- Cheap bail before any roster work. On the overwhelmingly common path - a match with no
        //--- admin watching - this is the whole cost of the feature.
        if (!HasAdminSpectator())
            return;

        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (!server)
            return;

        BattleRoyaleState state = server.GetCurrentState();
        if (!state)
            return;

        array<PlayerBase> roster = state.GetPlayers();
        if (!roster)
            return;

        array<string> uids = new array<string>();
        array<string> names = new array<string>();
        array<vector> positions = new array<vector>();
        array<float> healths = new array<float>();
        array<int> kills = new array<int>();
        array<int> parties = new array<int>();

        int count = roster.Count();
        int i = 0;
        for (i = 0; i < count; i++)
        {
            if (uids.Count() >= BR_ADMIN_LIST_MAX)
            {
                BattleRoyaleUtils.Warn(string.Format("[Spectate] Admin list truncated at %1 of %2 players", BR_ADMIN_LIST_MAX, count));
                break;
            }

            PlayerBase candidate = roster.Get(i);
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            string candidate_uid = candidate.GetIdentity().GetPlainId();
            if (candidate_uid == "")
                continue;

            string candidate_name = candidate.player_name;
            if (candidate_name == "")
                candidate_name = candidate.GetIdentity().GetName();

            uids.Insert(candidate_uid);
            names.Insert(candidate_name);
            positions.Insert(candidate.GetPosition());
            healths.Insert(candidate.GetHealth01("", "Health"));
            kills.Insert(candidate.br_kills);
            parties.Insert(AdminListPartyIndexOf(candidate));
        }

        int spectators = m_Spectators.Count();
        for (i = 0; i < spectators; i++)
        {
            BattleRoyaleSpectatorEntry entry = m_Spectators.GetElement(i);
            if (!entry)
                continue;
            if (!entry.is_admin)
                continue;

            PlayerIdentity identity = IdentityOfUid(entry.uid);
            if (!identity)
                continue;

            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetAdminPlayerList", new Param6<array<string>, array<string>, array<vector>, array<float>, array<int>, array<int>>(uids, names, positions, healths, kills, parties), true, identity);
        }
    }

    /**
     *  Build the corpse nametag payload and send it to every admin currently spectating (#278).
     *
     *  ⚠ POSITIONS ARE WHERE THEY FELL, NOT WHERE THE BODY IS NOW, and that is the correct answer
     *  rather than a shortcut. CarryCorpse moves a spectator's own corpse hundreds of metres to drag
     *  the network bubble along - but a carried body's REPLICATED position does not follow, so no
     *  client renders it anywhere at all (measured; see the Spectating notes in CLAUDE.md). Tagging
     *  the server-side position would therefore put a name in empty air, hundreds of metres from any
     *  visible body, and leave the place the fight actually happened unlabelled. death_pos is where a
     *  body appears to everyone, and it is also the thing the admin is asking about.
     *
     *  That is also what makes the set APPEND-ONLY: a death position never changes, so there is
     *  nothing to re-send except a longer list. Hence the count comparison rather than a diff.
     *
     *  NO UIDS ON THE WIRE, matching SetAdminPlayerList's own reasoning, and no party index either -
     *  a dead player's team is not something an admin can act on, and a second team-coloured name
     *  over a fight is exactly the stacking the overlay already had to fix once.
     */
    protected void PushAdminDeadList(int now)
    {
        //--- Same cheap bail as PushAdminList: on a match with nobody watching this is the whole
        //--- cost of the feature.
        if (!HasAdminSpectator())
            return;

        int total = m_DeathOrder.Count();

        bool keepalive_due = now >= m_NextDeadKeepaliveMs;

        //--- Nothing new and nothing owed. The keepalive exists because an admin who entered spectate
        //--- during a lull would otherwise see no corpses until somebody else died.
        if (total == m_LastDeadPushCount && !keepalive_due)
            return;

        if (keepalive_due)
            m_NextDeadKeepaliveMs = now + BR_ADMIN_DEAD_KEEPALIVE_MS;

        array<string> names = new array<string>();
        array<vector> positions = new array<vector>();

        int i = 0;
        for (i = 0; i < total; i++)
        {
            if (names.Count() >= BR_ADMIN_DEAD_LIST_MAX)
            {
                BattleRoyaleUtils.Warn(string.Format("[Spectate] Admin dead list truncated at %1 of %2 bodies", BR_ADMIN_DEAD_LIST_MAX, total));
                break;
            }

            //--- One array read per line, and the map read on its own line too, before any call
            //--- consumes either. The shape measured elsewhere in this codebase to hand back another
            //--- array's contents entirely.
            string victim_uid = m_DeathOrder.Get(i);
            if (victim_uid == "")
                continue;

            BattleRoyaleDeathRecord record = m_Deaths.Get(victim_uid);
            if (!record)
                continue;

            string victim_name = record.victim_name;
            if (victim_name == "")
                continue;

            names.Insert(victim_name);
            positions.Insert(record.death_pos);
        }

        m_LastDeadPushCount = total;

        int spectators = m_Spectators.Count();
        int sent = 0;
        for (i = 0; i < spectators; i++)
        {
            BattleRoyaleSpectatorEntry entry = m_Spectators.GetElement(i);
            if (!entry)
                continue;
            if (!entry.is_admin)
                continue;

            PlayerIdentity identity = IdentityOfUid(entry.uid);
            if (!identity)
                continue;

            GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetAdminDeadList", new Param2<array<string>, array<vector>>(names, positions), true, identity);
            sent++;
        }

        BattleRoyaleUtils.Debug(string.Format("[Spectate] Dead list: %1 bodies to %2 admin(s), keepalive=%3", names.Count(), sent, keepalive_due));
    }

    //------------------------------------------------------------------------------------------
    //--- Audience counter (#285)
    //------------------------------------------------------------------------------------------

    /**
     *  Tell each watched player how many people are currently watching them.
     *
     *  A RECOMPUTE, not an increment, and that is the whole design. The count is a many-to-one
     *  aggregate, so it moves on far more than add and remove: a Retarget shifts one spectator and
     *  changes TWO players' counts with no entry added or dropped, BeginSpectate clears
     *  pending_enter, and an admin session supersedes an ordinary one. Keeping five mutation sites
     *  in step with one derived number is how it would go wrong; rebuilding the tally is
     *  O(spectators) once a second.
     *
     *  Called from the TOP of Tick(), above its two early returns - see the comment there for why
     *  that position is load-bearing rather than tidy.
     */
    protected void PushAudienceCounts()
    {
        int now = GetGame().GetTime();
        if (now < m_NextAudienceMs)
            return;

        m_NextAudienceMs = now + BR_AUDIENCE_PUSH_MS;

        //--- Nothing to count and nothing outstanding to clear. This is the entire cost of the
        //--- feature on a match where nobody has died yet, on a server with spectating switched off,
        //--- and on every tick after the last count has been zeroed out.
        if (m_Spectators.Count() == 0 && m_AudienceSent.Count() == 0)
            return;

        //--- Re-assert every value this often even when nothing changed. An edge-only push is
        //--- invisibly wrong for a client that missed one, which is the lesson the play areas taught
        //--- when they were edge-only until a reconnect proved otherwise.
        bool force = false;
        if (now >= m_NextAudienceKeepaliveMs)
        {
            m_NextAudienceKeepaliveMs = now + BR_AUDIENCE_KEEPALIVE_MS;
            force = true;
        }

        //--- The admin switch gates the TALLY, not the pass. With it off the tally comes back empty
        //--- and the drain below pushes everyone a zero, so turning it off mid-match clears the row
        //--- instead of freezing it on whatever number was last shown.
        bool enabled = BattleRoyaleConfig.GetConfig().GetGameData().show_spectator_count;

        map<string, int> tally = new map<string, int>();
        int i = 0;
        int count = 0;

        if (enabled)
        {
            count = m_Spectators.Count();
            for (i = 0; i < count; i++)
            {
                BattleRoyaleSpectatorEntry entry = m_Spectators.GetElement(i);
                if (!CountsAsAudience(entry))
                    continue;

                //--- Read into a local before the call that consumes it. Same rule as PushTeamState
                //--- in the party manager and the aliasing measured in BattleRoyaleZone: a container
                //--- read nested inside a call has been seen to return another container's contents.
                string watched = entry.target_uid;

                int running = 0;
                if (tally.Contains(watched))
                    running = tally.Get(watched);

                tally.Set(watched, running + 1);
            }
        }

        //--- (a) everyone who has an audience right now.
        count = tally.Count();
        for (i = 0; i < count; i++)
        {
            string watched_uid = tally.GetKey(i);
            int watchers = tally.GetElement(i);

            if (!force && AlreadySentAudience(watched_uid, watchers))
                continue;

            //--- Deliberately NOT recorded when the identity failed to resolve, so a client that was
            //--- mid-drop is retried on the next pass rather than remembered as up to date.
            if (!SendAudience(watched_uid, watchers))
                continue;

            m_AudienceSent.Set(watched_uid, watchers);
        }

        //--- (b) ...and everyone who HAD one and no longer does. Collected first: removing from a map
        //--- while walking it is the same hazard Tick()'s key copy exists for.
        array<string> stale = new array<string>();
        count = m_AudienceSent.Count();
        for (i = 0; i < count; i++)
        {
            string sent_uid = m_AudienceSent.GetKey(i);
            if (tally.Contains(sent_uid))
                continue;

            stale.Insert(sent_uid);
        }

        count = stale.Count();
        for (i = 0; i < count; i++)
        {
            //--- Dropped whether or not the send landed, unlike (a): an identity that no longer
            //--- resolves is a player who has left, and there is nobody left to correct.
            string drop_uid = stale.Get(i);
            SendAudience(drop_uid, 0);
            m_AudienceSent.Remove(drop_uid);
        }
    }

    //! Is this exactly what this player was last told? Split out of the caller so the map read is
    //! not nested inside the condition consuming it.
    protected bool AlreadySentAudience(string watched_uid, int watchers)
    {
        if (!m_AudienceSent.Contains(watched_uid))
            return false;

        int previous = m_AudienceSent.Get(watched_uid);
        return previous == watchers;
    }

    /**
     *  Is this spectator part of somebody's audience?
     *
     *  Three exclusions, and the first is the point of the whole feature.
     */
    protected bool CountsAsAudience(BattleRoyaleSpectatorEntry entry)
    {
        if (!entry)
            return false;

        //--- An ADMIN SESSION is an operator action, not part of the game. Surfacing it would tell a
        //--- player the exact moment they are being observed, which is the one thing admin spectate
        //--- must never leak - the same reason the anchor body is carried invisibly and the party
        //--- hide exists.
        //---
        //--- THE TEST IS THE SESSION TYPE, NOT MEMBERSHIP OF admins_steamid64, and that distinction
        //--- is load-bearing. An admin who competed, died and took ORDINARY spectate is a real
        //--- audience member watching for the same reason as anybody else; keying on the operator
        //--- roster would silently uncount them, so the number shown would depend on who is on an
        //--- admin list rather than on what sessions exist. is_admin is written in exactly one place
        //--- (BeginAdminSpectate) and means precisely "this is an operator session".
        if (entry.is_admin)
            return false;

        //--- Still on the death screen: the camera is not attached, they may press Quit and never
        //--- watch anybody at all, and target_uid is the PROVISIONAL value OnDeath records for
        //--- logging, which BeginSpectate re-resolves. Counting it would show an audience of 1 for up
        //--- to BR_SPECTATE_ENTRY_DELAY_MS and then move it to a different player for no reason
        //--- visible on either screen.
        if (entry.pending_enter)
            return false;

        //--- T5: orbiting the final circle with nobody to follow. There is no one to charge it to,
        //--- and "" must never enter m_AudienceSent, where it would fail IdentityOfUid for ever.
        if (entry.target_uid == "")
            return false;

        return true;
    }

    /**
     *  Push one player their own audience count. Returns whether it went out.
     *
     *  Per-identity, never broadcast: this is a fact about ONE player and is only ever true for
     *  them. Only the count goes on the wire - no names and no uids, so a player learns that they
     *  are being watched without learning who is dead.
     */
    protected bool SendAudience(string watched_uid, int watchers)
    {
        PlayerIdentity identity = IdentityOfUid(watched_uid);
        if (!identity)
            return false;

        GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetAudienceCount", new Param1<int>(watchers), true, identity);
        return true;
    }

    /**
     *  Match-local index of this player's PARTY, for the overlay's team colour. -1 means solo.
     *
     *  ⚠ THIS USED TO BE GetMemberIndex, AND THAT WAS THE BUG (#276). GetMemberIndex is the player's
     *  slot INSIDE their own party - 0 for the leader, 1 for the second to join - so every party's
     *  first member came out the same colour and the overlay answered "which of my teammates is
     *  that" to somebody who is in none of the parties. The question an admin asks is "are those two
     *  on the same side", which needs an index over PARTIES, not over members.
     *
     *  KEYED ON THE PARTY ID, NOT ON A POSITION IN ANY LIST. The obvious alternative - walk
     *  VigridPartyAPI.GetGroups(roster) and use the group's position - re-partitions the living
     *  population every push, so a team's colour would change the moment somebody in an earlier
     *  group died. GetPartyId is a stable string for the life of the party, and the first-seen order
     *  recorded in m_PartyColourIndex is fixed for the whole match by construction.
     *
     *  A party reduced to ONE living member keeps its colour. They do have a team - they are the
     *  last of it - and dropping them to neutral mid-match would make a colour blink out at exactly
     *  the moment an admin is watching that fight.
     *
     *  IsReady() rather than #ifdef VIGRID_PARTY alone: the addon ships in this repo, so the #else
     *  arm is dead on every real server, but party_settings.json can still switch the manager off -
     *  in which case every player is correctly solo and the whole overlay goes neutral. This is the
     *  #158 rule; the guard LOOKS like it covers the case and does not.
     */
    protected int AdminListPartyIndexOf(PlayerBase player)
    {
#ifdef VIGRID_PARTY
        if (!VigridPartyAPI.IsReady())
            return -1;

        string party_id = VigridPartyAPI.GetPartyId(player);
        if (party_id == "")
            return -1;

        if (!m_PartyColourIndex)
            m_PartyColourIndex = new map<string, int>();

        if (m_PartyColourIndex.Contains(party_id))
            return m_PartyColourIndex.Get(party_id);

        int assigned = m_PartyColourIndex.Count();
        m_PartyColourIndex.Insert(party_id, assigned);

        BattleRoyaleUtils.Debug(string.Format("[Spectate] Party %1 takes team colour index %2", party_id, assigned));
        return assigned;
#else
        return -1;
#endif
    }

#ifdef DIAG_DEVELOPER
    /**
     *  Dump the whole spectator table to the log, for the diag menu's "Log Spectators".
     *
     *  Every line goes through BattleRoyaleUtils.Info rather than one giant string, so the DIAG chat
     *  mirror renders it as readable rows rather than one clipped message.
     *
     *  Read-only by construction - it resolves nothing and mutates nothing, which is what makes it
     *  safe to press at any moment including mid-Retarget.
     */
    void LogSpectators()
    {
        int spectators = m_Spectators.Count();
        int deaths = m_DeathOrder.Count();
        int audiences = m_AudienceSent.Count();
        int i = 0;

        BattleRoyaleUtils.Info(string.Format("[Spectate] --- table: %1 spectator(s), %2 death(s), ended=%3", spectators, deaths, m_Ended));

        //--- What each watched player was last TOLD, which is the only view of the audience counter
        //--- there is: the tally itself is rebuilt and discarded inside every pass. Cross-check it
        //--- against the number actually on each client's HUD.
        for (i = 0; i < audiences; i++)
        {
            //--- Both reads on their own lines rather than nested in the Format call - the container
            //--- aliasing shape this repo has measured returning another container's contents.
            string audience_uid = m_AudienceSent.GetKey(i);
            int audience_count = m_AudienceSent.GetElement(i);
            BattleRoyaleUtils.Info(string.Format("[Spectate]   audience %1 = %2", audience_uid, audience_count));
        }

        for (i = 0; i < spectators; i++)
        {
            BattleRoyaleSpectatorEntry entry = m_Spectators.GetElement(i);
            if (!entry)
                continue;

            string phase = "spectating";
            if (entry.pending_enter)
                phase = "pending";

            //--- Whether the identity still resolves is the single most useful field here: it is what
            //--- the liveness sweep keys on, and "registered but gone" is the shape of a leak.
            string live = "identity=no";
            if (IdentityOfUid(entry.uid))
                live = "identity=yes";

            BattleRoyaleUtils.Info(string.Format("[Spectate]   %1 (%2) %3 target=%4 T%5 %6", entry.uid, entry.name, phase, TargetLog(entry.target_uid), entry.resolved_tier, live));
        }

        for (i = 0; i < deaths; i++)
        {
            BattleRoyaleDeathRecord record = m_Deaths.Get(m_DeathOrder.Get(i));
            if (!record)
                continue;

            string killer = record.killer_uid;
            if (killer == "")
                killer = "none (environmental)";

            BattleRoyaleUtils.Info(string.Format("[Spectate]   death #%1 %2 (%3) killer=%4 party=%5", i, record.victim_uid, record.victim_name, killer, record.party_id));
        }
    }

    /**
     *  Is this uid registered as a spectator? Lets the diag "Kill Me" refuse a second press from
     *  somebody who is already watching - SimulateDeath on a corpse is not a meaningful test, and
     *  RecordDeath is first-write-wins so the second death would be silently discarded anyway.
     */
    bool IsRegistered(string uid)
    {
        return m_Spectators.Contains(uid);
    }

    /**
     *  For the diag range test: where this spectator fell, and who they are watching right now.
     *
     *  Both halves are needed together and both live behind this class's privacy, so they come back
     *  in one call rather than as two accessors a caller could use inconsistently. The PlayerBase is
     *  resolved here through the same LivingPlayerByUid every other path uses - it is handed out,
     *  never stored, so invariant 1 ("holds no object reference at all") is untouched.
     *
     *  False means there is nothing to teleport: not spectating, still on the death screen, or
     *  orbiting the circle with no target at all (T5).
     */
    bool GetRangeTestSubject(string spectator_uid, out vector death_pos, out PlayerBase target_player)
    {
        death_pos = vector.Zero;
        target_player = NULL;

        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(spectator_uid);
        if (!entry)
            return false;
        if (entry.pending_enter)
            return false;
        if (entry.target_uid == "")
            return false;

        BattleRoyaleDeathRecord record = m_Deaths.Get(spectator_uid);
        if (!record)
            return false;

        target_player = LivingPlayerByUid(entry.target_uid);
        if (!target_player)
            return false;

        death_pos = record.death_pos;
        return true;
    }

    /**
     *  Price the carry pass at a FULL LOBBY, from a three-client test (#280).
     *
     *  WHY IT EXISTS. LaunchLocalMP.bat tops out at three clients, so at most two ordinary
     *  spectators can ever exist in a real test - and the thing #280 suspects is what happens with
     *  sixty. The repo's usual answer to "the case that matters is unreachable with three clients"
     *  is a synthetic self test at boot (zone_selftest_runs, auto_group_selftest), but that does NOT
     *  transfer here: at boot there are zero Man entities, so the one term that scales - the world
     *  walk in FindBodyByUid - would have nothing to walk. It has to run mid-match, hence a one-shot.
     *
     *  WHY NOT JUST CREATE SIXTY BODIES. Because that is what crashed a dedicated server twice, in
     *  this very subsystem: CreateObjectEx faulted at 0x0 and CreatePlayer at 0x9, neither catchable
     *  in script. See CLAUDE.md -> Spectating. Phase 2 pads an ARRAY instead, which is faithful
     *  because the work being priced is a Cast plus a comparison against a CACHED string member -
     *  identical whichever body the element points at.
     *
     *  WHAT IT CANNOT PRICE, stated rather than hidden: the proto GetPlayers() fill itself at N=60.
     *  Phase 1 measures it at the live N and it is a linear engine-side array copy, so its N=60 cost
     *  is bounded by roughly (60 / live N) times a figure already on screen.
     */
    void BenchCarry(string requester_uid, int bodies)
    {
        if (m_Ended)
        {
            BattleRoyaleUtils.Warn("[Spectate][Bench] refused: the match has ended");
            return;
        }

        int want = bodies;
        if (want < 1)
            want = 1;
        if (want > BR_SPECTATE_BENCH_MAX)
            want = BR_SPECTATE_BENCH_MAX;

        array<PlayerIdentity> identities = new array<PlayerIdentity>();
        GetGame().GetPlayerIndentities(identities);

        array<Man> everyone = new array<Man>();
        GetGame().GetPlayers(everyone);

        int live_p = identities.Count();
        int live_n = everyone.Count();
        if (live_n < 1)
        {
            BattleRoyaleUtils.Warn("[Spectate][Bench] refused: no Man entities in the world");
            return;
        }

        int live_r = 0;
        BattleRoyaleServer server = BattleRoyaleServer.GetInstance();
        if (server)
        {
            BattleRoyaleState bench_state = server.GetCurrentState();
            if (bench_state)
            {
                array<PlayerBase> roster = bench_state.GetPlayers();
                if (roster)
                    live_r = roster.Count();
            }
        }

        int iterations = BR_SPECTATE_BENCH_ITERATIONS;
        //--- Divisor kept as an explicit float. Every per-call figure below is a float divided by
        //--- the iteration count, and an int divisor is exactly the shape that quietly truncates.
        float iters_f = iterations;
        float want_f = want;
        int b = 0;
        int w = 0;
        float started = 0;

        //--- PHASE 1. The three real lookups at the real N, timed as batches. All pure reads.
        started = GetGame().GetTickTime();
        for (b = 0; b < iterations; b++)
        {
            IdentityOfUid(requester_uid);
        }
        float ident_ms = (GetGame().GetTickTime() - started) * 1000.0;

        started = GetGame().GetTickTime();
        for (b = 0; b < iterations; b++)
        {
            LivingPlayerByUid(requester_uid);
        }
        float roster_ms = (GetGame().GetTickTime() - started) * 1000.0;

        started = GetGame().GetTickTime();
        for (b = 0; b < iterations; b++)
        {
            FindBodyByUid(requester_uid);
        }
        float body_ms = (GetGame().GetTickTime() - started) * 1000.0;

        string p1 = "[Spectate][Bench] phase1 live P=" + live_p;
        p1 = p1 + " R=" + live_r;
        p1 = p1 + " N=" + live_n;
        p1 = p1 + " iters=" + iterations;
        BattleRoyaleUtils.Info(p1);

        string p1b = "[Spectate][Bench] phase1 per-call us: ident=" + ((ident_ms * 1000.0) / iters_f);
        p1b = p1b + " roster=" + ((roster_ms * 1000.0) / iters_f);
        p1b = p1b + " body=" + ((body_ms * 1000.0) / iters_f);
        BattleRoyaleUtils.Info(p1b);

        //--- PHASE 2. The world walk at N = want, without creating a single entity. The uid searched
        //--- for is one that cannot exist, so every iteration is the WORST case: a full walk with no
        //--- early exit, which is exactly the miss that dominates FindBodyByUid in a real pass.
        array<Man> padded = new array<Man>();
        while (padded.Count() < want)
        {
            for (b = 0; b < live_n; b++)
            {
                if (padded.Count() >= want)
                    break;

                //--- Read into a local on its own line before the call. Same container-aliasing rule
                //--- BattleRoyaleZone documents, and this loop is exactly its shape.
                Man source = everyone.Get(b);
                padded.Insert(source);
            }
        }

        string absent = "bench-no-such-uid";
        int padded_count = padded.Count();
        int hits = 0;

        started = GetGame().GetTickTime();
        for (b = 0; b < want; b++)
        {
            for (w = 0; w < padded_count; w++)
            {
                PlayerBase candidate = PlayerBase.Cast(padded.Get(w));
                if (!candidate)
                    continue;
                if (candidate.player_steamid != absent)
                    continue;

                hits++;
            }
        }
        float pass_ms = (GetGame().GetTickTime() - started) * 1000.0;

        //--- PHASE 3a. The teleport's SCRIPT half, with nothing sent. SurfaceY is the call the zone
        //--- generator bothers to count, so it is the part most likely to dominate server-side.
        PlayerBase own_body = FindBodyByUid(requester_uid);
        float surface_ms = 0;
        if (own_body)
        {
            vector probe = own_body.GetPosition();
            started = GetGame().GetTickTime();
            for (b = 0; b < iterations; b++)
            {
                probe[1] = GetGame().SurfaceY(probe[0], probe[2]);

                ScriptJunctureData probe_ctx = new ScriptJunctureData;
                probe_ctx.Write( probe );
                probe_ctx.Write( own_body.GetDirection() );
            }
            surface_ms = (GetGame().GetTickTime() - started) * 1000.0;
        }

        string p2 = "[Spectate][Bench] phase2 match loop at N=" + padded_count;
        p2 = p2 + " x " + want + " passes = " + pass_ms;
        p2 = p2 + " ms (hits=" + hits + ")";
        BattleRoyaleUtils.Info(p2);

        string p3 = "[Spectate][Bench] phase3a juncture prep per-call us=" + ((surface_ms * 1000.0) / iters_f);
        BattleRoyaleUtils.Info(p3);

        //--- PHASE 3b. The only part with side effects, and the only one that is refused.
        //---
        //--- Allowed ONLY for an ordinary spectator whose corpse is already carried: that body has
        //--- already been emptied by DropAllItems and is already unreplicated, and the junctures land
        //--- on the presser's own connection. Refused outright on a LIVE body - the admin anchor case
        //--- - because that one is simulated and its teleports replicate to everybody.
        //---
        //--- Sent to the body's CURRENT position, so nothing actually moves.
        float move_ms = -1;
        BattleRoyaleSpectatorEntry entry = m_Spectators.Get(requester_uid);
        bool may_move = false;
        if (entry)
        {
            if (!entry.is_admin)
            {
                if (entry.corpse_carried)
                    may_move = true;
            }
        }

        if (may_move)
        {
            if (own_body)
            {
                vector hold = own_body.GetPosition();
                started = GetGame().GetTickTime();
                for (b = 0; b < want; b++)
                {
                    MoveCorpse(own_body, hold);
                }
                move_ms = (GetGame().GetTickTime() - started) * 1000.0;
            }
        }

        if (move_ms < 0)
            BattleRoyaleUtils.Info("[Spectate][Bench] phase3b skipped - needs an ordinary spectator whose corpse is already carried");
        else
            BattleRoyaleUtils.Info("[Spectate][Bench] phase3b " + want + " real MoveCorpse = " + move_ms + " ms");

        //--- THE PROJECTION. One carry pass at S spectators is S x (one roster walk + one world walk)
        //--- plus whatever teleports fire, and phase 2 measured exactly S passes over N bodies.
        float per_pass_ms = pass_ms + (want_f * ((roster_ms + body_ms) / iters_f));

        string proj = "[Spectate][Bench] projected " + want + " spectators x " + padded_count;
        proj = proj + " bodies: " + (want * padded_count) + " compares/pass";
        proj = proj + ", " + per_pass_ms + " ms/pass at 1 Hz";
        BattleRoyaleUtils.Info(proj);

        //--- The control. The same shape of work already runs at 10 Hz right beside the carry, so
        //--- this is what the number above has to be a percentage OF.
        float sweep_ms = (want_f * 10.0) * ((ident_ms + roster_ms) / iters_f);

        string cmp = "[Spectate][Bench] compare: the 10 Hz liveness sweep at S=" + want;
        cmp = cmp + " costs " + sweep_ms + " ms/s";
        BattleRoyaleUtils.Info(cmp);
    }

#endif
}
#endif
