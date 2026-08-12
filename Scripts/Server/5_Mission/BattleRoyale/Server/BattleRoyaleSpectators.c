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
 *  There is NO client -> server RPC. Target selection is automatic, so there is no client request to
 *  authenticate, and therefore nothing to spoof, rate-limit or gate.
 */

//! One death. Written once, read for the rest of the match.
class BattleRoyaleDeathRecord
{
    string victim_uid;   //!< SteamID64. Map key.
    string victim_name;  //!< cached at death, so rendering a name never needs an identity
    string killer_uid;   //!< SteamID64 of the responsible PLAYER, or "" for every non-player cause
    int death_ms;        //!< GetGame().GetTime() at death
    string party_id;     //!< VigridPartyAPI.GetPartyId() snapshot, "" when solo or no party addon
    vector death_pos;    //!< where they fell. Used to pick the NEAREST living player as a fallback.

    void BattleRoyaleDeathRecord(string victim, string name, string killer, int time_ms, string party, vector position)
    {
        victim_uid = victim;
        victim_name = name;
        killer_uid = killer;
        death_ms = time_ms;
        party_id = party;
        death_pos = position;
    }
}

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
    protected bool m_Ended;

    //! Which tier the LAST ResolveTarget call returned from. Written at every one of its five exits
    //! and read by the caller on the very next line, so it is a return value in all but name - a
    //! second out-parameter would have to be threaded through three call sites for a field nothing
    //! branches on. Never read it anywhere but immediately after a ResolveTarget call.
    protected int m_LastResolveTier;

    void BattleRoyaleSpectators()
    {
        m_Deaths = new map<string, ref BattleRoyaleDeathRecord>();
        m_DeathOrder = new array<string>();
        m_Spectators = new map<string, ref BattleRoyaleSpectatorEntry>();
        m_NextPushMs = 0;
        m_NextCarryMs = 0;
        m_Ended = false;
        m_LastResolveTier = 0;
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

    //------------------------------------------------------------------------------------------
    //--- The ledger
    //------------------------------------------------------------------------------------------

    /**
     *  Resolve the PLAYER responsible for a death to a uid, or "" when nothing player-shaped is.
     *
     *  EEKilled's `source` is the WEAPON for every gun and melee kill, not the shooter - which is
     *  exactly the bug that left the WIP branch's chain resolving NULL on every real kill. The
     *  hierarchy-parent step is the fix, and it is the same idiom the webhook code already uses at
     *  0_BattleRoyaleState.c:545-550.
     */
    static string ResolveKillerUid(PlayerBase victim, Object source)
    {
        string victim_uid = "";
        string result = "";
        PlayerBase killer_player = NULL;
        EntityAI source_entity = NULL;

        if (!victim)
            return "";
        if (!victim.GetIdentity())
            return "";

        victim_uid = victim.GetIdentity().GetPlainId();

        if (!source)
            return "";
        //--- zone damage, starvation, dehydration, fall, drowning: the victim is their own source
        if (source == victim)
            return "";

        //--- explosives carry the activator's SteamID64 directly
        if (source.IsInherited(Grenade_Base) || source.IsInherited(LandMineTrap))
        {
            EnScript.GetClassVar(source, "m_ActivatorId", -1, result);
            if (result == victim_uid)
                result = "";

            return result;
        }

        //--- gun or melee: the weapon's hierarchy parent is the shooter
        source_entity = EntityAI.Cast(source);
        if (source_entity)
            killer_player = PlayerBase.Cast(source_entity.GetHierarchyParent());

        //--- fists, or a direct player source
        if (!killer_player)
            killer_player = PlayerBase.Cast(source);

        //--- anything else (infected, animal, vehicle, world object) is environmental to us
        if (!killer_player)
            return "";
        if (killer_player == victim)
            return "";
        if (!killer_player.GetIdentity())
            return "";

        result = killer_player.GetIdentity().GetPlainId();
        if (result == victim_uid)
            result = "";

        return result;
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
        m_Deaths.Set(uid, record);
        m_DeathOrder.Insert(uid);

        string killer_log = killer_uid;
        if (killer_log == "")
            killer_log = "<environment>";

        string party_log = party_id;
        if (party_log == "")
            party_log = "-";

        BattleRoyaleUtils.Info(string.Format("[Spectate] RecordDeath victim=%1 killer=%2 party=%3", uid, killer_log, party_log));
    }

    /**
     *  Record a death whose killer the caller has ALREADY resolved to a PlayerBase.
     *
     *  Used by the unconscious-disconnect path, which knows the real killer but then kills the
     *  player by zeroing health - losing that attribution by the time EEKilled fires.
     */
    void RecordDeathWithKiller(PlayerBase victim, PlayerBase killer)
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

        string killer_uid = "";
        if (killer && killer.GetIdentity())
            killer_uid = killer.GetIdentity().GetPlainId();
        if (killer_uid == uid)
            killer_uid = "";

        string party_id = PartyIdOf(victim);
        string victim_name = victim.player_name;
        if (victim_name == "")
            victim_name = victim.GetIdentity().GetName();

        BattleRoyaleDeathRecord record = new BattleRoyaleDeathRecord(uid, victim_name, killer_uid, GetGame().GetTime(), party_id, victim.GetPosition());
        m_Deaths.Set(uid, record);
        m_DeathOrder.Insert(uid);

        BattleRoyaleUtils.Info(string.Format("[Spectate] RecordDeath (disconnect) victim=%1 killer=%2", uid, killer_uid));
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

        m_Spectators.Remove(uid);
        BattleRoyaleUtils.Info("[Spectate] Removed " + uid);
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

            Retarget(entry);
        }
    }

    protected void Retarget(BattleRoyaleSpectatorEntry entry)
    {
        if (!entry)
            return;

        string previous = entry.target_uid;
        entry.target_uid = ResolveTarget(entry.uid);
        entry.resolved_tier = m_LastResolveTier;

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
        bool do_carry = false;
        if (BR_SPECTATE_CARRY_CORPSE && now >= m_NextCarryMs)
        {
            m_NextCarryMs = now + BR_SPECTATE_CARRY_INTERVAL_MS;
            do_carry = true;
        }

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
                m_Spectators.Remove(uid);
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
                Retarget(entry);
                continue;
            }

            //--- (d) keepalive. Re-delivers the target position and entity every second, so a client
            //--- that missed or could not resolve the first push simply latches on the next.
            if (do_push)
                Push(entry, identity);

            //--- (e) corpse carry. Keeps the replication bubble - which sits on the corpse, not on
            //--- the camera - within range of whoever is being watched.
            if (do_carry)
                CarryCorpse(entry);
        }
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

        //--- Drop the corpse from the connection's selection.
        //---
        //--- IT DOES NOT DROP THE NETWORK BUBBLE WITH IT, and do not assume otherwise from the shape
        //--- of this call. The bubble stays on the corpse: measured both directions 2026-08-10 with
        //--- the diag TP Target entry, the watched target is not replicated at 1200 m from where this
        //--- player died (sustained 90 s, no recovery walking back to 1122 m) and is replicated again
        //--- at 700 m, with the camera's UpdateSpectatorPosition running throughout - 507 pushes,
        //--- camera always within ~4 m of the pushed position. That call is documented as "position
        //--- of network bubble" and has no effect on player replication.
        //---
        //--- So this line is kept for what it actually does - taking the corpse out of the
        //--- connection's selection so the client stops treating it as its own player - and the ~1 km
        //--- limit is a known, documented limitation rather than something this line solves.
        //--- CLAUDE.md carries the full history, including why the carrier body is not the answer.
        GetGame().SelectPlayer(identity, NULL);

        GetGame().SelectSpectator(identity, BR_SPECTATE_CAM_CLASS, position);

        entry.pending_enter = false;
        entry.entered_ms = GetGame().GetTime();

        Push(entry, identity);
        Notify(entry, identity);

        BattleRoyaleUtils.Info(string.Format("[Spectate] BeginSpectate %1 class=%2 pos=%3 target=%4 (T%5)", entry.uid, BR_SPECTATE_CAM_CLASS, position.ToString(), TargetLog(entry.target_uid), entry.resolved_tier));
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

        m_Ended = true;

        int count = m_Spectators.Count();
        int i = 0;
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

        m_Spectators.Clear();
    }

    protected string TargetLog(string uid)
    {
        if (uid == "")
            return "none";

        return uid;
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
        int i = 0;

        BattleRoyaleUtils.Info(string.Format("[Spectate] --- table: %1 spectator(s), %2 death(s), ended=%3", spectators, deaths, m_Ended));

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

#endif
}
#endif
