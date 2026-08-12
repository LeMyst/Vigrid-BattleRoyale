/**
 *  Vigrid Party - the public API. THIS IS THE ENTIRE CONTRACT with the host game.
 *
 *  TWO-SIDED. The server half answers questions about party composition; the client half answers
 *  questions about what the local player can currently see of their party. They share a name and
 *  nothing else - there is no method that exists on both sides, deliberately (see IsClientReady).
 *  The file carries no top-level guard; each half is guarded inside the class body instead, so both
 *  are visible in one place and neither can be added to without the other being read.
 *
 *  The server block is written first, which is the opposite order from VigridMapAPI. That is not an
 *  oversight and should not be "fixed": putting it first is what let the guards be introduced around
 *  148 lines of shipped, working code without re-indenting or moving any of it.
 *
 *  --- server half -------------------------------------------------------------------------------
 *
 *  Consumers pass PlayerBase and get PlayerBase back; no caller ever handles a party key, which is
 *  what lets the identity scheme stay an implementation detail (it is PlayerIdentity.GetPlainId()
 *  throughout - never GetPlayerId(), a session index the engine reuses after a disconnect).
 *
 *  Every grouping query takes the population explicitly rather than reaching for the player list
 *  itself. The caller decides whether that means a match roster or every connected player, so
 *  Party never needs to know anything about match state, and a player who is dead, disconnected or
 *  simply not in the population is invisible to these functions.
 *
 *  Every method is safe to call before the manager exists - the addon degrades to "everyone is
 *  solo" rather than throwing, so a host game never has to null-check.
 *
 *  --- client half -------------------------------------------------------------------------------
 *
 *  Reads VigridPartyRPC, the bag the server pushes into, and hides three things every consumer would
 *  otherwise have to know: that state_* is only valid while state_version == roster_version, that a
 *  teammate inside the network bubble has a better position than the pushed one, and that a ping
 *  can be locally expired before the server's next sweep removes it.
 *
 *  Every method is total: an out-of-range index yields "", vector.Zero, the off-white no-slot colour,
 *  0 or -1 rather than throwing. Same promise as the server half - no caller ever null-checks.
 *
 *  Usage from the host game (guard every call site, so removing party.pbo still builds):
 *
 *      #ifdef VIGRID_PARTY
 *          int groups = VigridPartyAPI.GetGroupCount(GetPlayers());
 *      #endif
 */
class VigridPartyAPI
{
#ifdef SERVER
    static bool IsReady()
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager)
            return false;

        return manager.IsEnabled();
    }

    /**
     *  Number of distinct groups in `population`. Always equal to GetGroups(population).Count() -
     *  they share one implementation precisely so the two can never disagree, which is the bug the
     *  code this replaces had (its count indexed the match roster while its grouping indexed every
     *  connected player).
     */
    static int GetGroupCount(array<PlayerBase> population)
    {
        return GetGroups(population).Count();
    }

    /**
     *  Partition `population` into groups: every player appears in exactly one group, and a player
     *  with no party forms a group of one.
     */
    static array<ref array<PlayerBase>> GetGroups(array<PlayerBase> population)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return SoloGroups(population);

        return manager.BuildGroups(population);
    }

    /**
     *  Party members of `player` that are present in `population`, excluding `player`. Returns an
     *  empty array - never null - for a solo player, so callers can foreach unconditionally.
     */
    static array<PlayerBase> GetTeammates(PlayerBase player, array<PlayerBase> population)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return new array<PlayerBase>();

        return manager.GetTeammatesIn(player, population);
    }

    /**
     *  The leader of `player`'s party, if present in `population`. Null for a solo player, and null
     *  when the leader is absent from the population, so a caller that gathers a party around its
     *  leader can tell "no leader here" from "I am the leader" rather than being given a stand-in.
     */
    static PlayerBase GetLeader(PlayerBase player, array<PlayerBase> population)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return NULL;

        return manager.GetLeaderIn(player, population);
    }

    //! O(1). Two players in no party are not teammates, and a player is not their own teammate.
    static bool AreTeammates(PlayerBase a, PlayerBase b)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return false;

        return manager.AreTeammates(VigridPartyManager.UidOf(a), VigridPartyManager.UidOf(b));
    }

    //! "" when the player has no party.
    static string GetPartyId(PlayerBase player)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return "";

        return manager.GetPartyIdOf(VigridPartyManager.UidOf(player));
    }

    /**
     *  Stable 0-based slot inside the party, -1 when solo. Ordered by join time and never
     *  reshuffled, which makes it safe to key per-member presentation off - a spawn marker colour,
     *  for instance, stays the same for the whole match.
     */
    static int GetMemberIndex(PlayerBase player)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return -1;

        return manager.GetMemberIndexOf(VigridPartyManager.UidOf(player));
    }

    static int GetMaxPartySize()
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager)
            return VIGRID_PARTY_DEF_MAX_SIZE;

        return manager.GetMaxPartySize();
    }

    /**
     *  Freeze party composition. The host game calls this when a match starts, so that nobody can
     *  split off mid-round and change the group count the match state machine is counting down.
     */
    static void SetFormationLocked(bool locked)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager)
            return;

        manager.SetFormationLocked(locked);
    }

    static bool IsFormationLocked()
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager)
            return false;

        return manager.IsFormationLocked();
    }

    /**
     *  Tell Party that a member's display name changed, so the rosters go out again.
     *
     *  Party reads names off the player at broadcast time and only broadcasts on composition
     *  changes, so a name that changes on its own is invisible to it. The host mod knows when that
     *  happens; Party cannot.
     */
    static void RefreshRosterNames()
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager)
            return;

        manager.RefreshRosterNames();
    }

    //! Degenerate partition used when the addon is disabled or not up yet: one group per player.
    private static array<ref array<PlayerBase>> SoloGroups(array<PlayerBase> population)
    {
        array<ref array<PlayerBase>> groups = new array<ref array<PlayerBase>>();
        if (!population)
            return groups;

        int count = population.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase player = population.Get(i);
            if (!player)
                continue;

            ref array<PlayerBase> solo = new array<PlayerBase>();
            solo.Insert(player);
            groups.Insert(solo);
        }

        return groups;
    }
#endif

#ifndef SERVER

    //--- readiness ---------------------------------------------------------------------------

    /**
     *  Whether the party system is switched on for this server.
     *
     *  Named apart from the server block's IsReady() on purpose, even though the two guards are
     *  mutually exclusive and one name would compile. They do not mean the same thing: the server's
     *  answers "the manager exists and is enabled", while this one leans on a setting that defaults
     *  TRUE before any VP_Settings arrives - so during the first moments of a session the two would
     *  be wrong in opposite directions. Worse, an unguarded 4_World or 5_Mission caller would compile
     *  against both and silently mean something different per side, which no build would catch.
     */
    static bool IsClientReady()
    {
        return VigridPartyRPC.GetInstance().enabled;
    }

    //! The gate every consumer wants: switched on AND actually in a party. One call, not two.
    static bool HasParty()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (!rpc.enabled)
            return false;

        return rpc.HasParty();
    }

    /**
     *  Whether the pushed member state is old enough to be worth flagging. Consumers dim rather than
     *  hide - IsMemberVisible owns the harder cutoff at which a member disappears entirely.
     */
    static bool IsStateStale()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        return (GetGame().GetTime() - rpc.state_recv_ms) > (3 * VIGRID_PARTY_DEF_STATE_INTERVAL_MS);
    }

    //--- roster ------------------------------------------------------------------------------

    static int GetMemberCount()
    {
        return VigridPartyRPC.GetInstance().roster_uids.Count();
    }

    //! Your own slot, or -1. Note this is a roster index, not a player id.
    static int GetSelfIndex()
    {
        return VigridPartyRPC.GetInstance().self_index;
    }

    static string GetMemberUid(int index)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (index < 0)
            return "";
        if (index >= rpc.roster_uids.Count())
            return "";

        return rpc.roster_uids.Get(index);
    }

    /**
     *  Display name for roster slot `index`, ready to hand straight to a TextWidget.
     *
     *  The server sends VIGRID_PARTY_UNKNOWN_NAME_KEY when it has never seen a name for an offline
     *  member, so what arrives may be a stringtable key rather than a name. Resolving it HERE is
     *  why every renderer should call this instead of reading rpc.roster_names itself: a caller
     *  that concatenates first - a leader " *", an " (Offline)" suffix - hands SetText a string
     *  that no longer starts with '#' and the key renders raw.
     */
    static string GetMemberName(int index)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (index < 0)
            return "";
        if (index >= rpc.roster_names.Count())
            return "";

        string name = rpc.roster_names.Get(index);
        if (name.IndexOf("#") == 0)
            return Widget.TranslateString(name);

        return name;
    }

    /**
     *  Whether slot `index` is currently connected - and nothing else.
     *
     *  Separate from IsMemberVisible because that one folds together online, alive and freshness:
     *  a dead teammate must not get a world tag, but the party menu still wants to list them by
     *  name without calling them offline.
     */
    static bool IsMemberOnline(int index)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (index < 0)
            return false;
        if (index >= rpc.roster_uids.Count())
            return false;

        //--- No usable state yet: assume online rather than labelling everybody offline for the
        //--- frame or two between a roster arriving and the first VP_TeamState that matches it.
        if (rpc.state_version != rpc.roster_version)
            return true;
        if (index >= rpc.state_flags.Count())
            return true;

        int member_flags = rpc.state_flags.Get(index);
        return (member_flags & VIGRID_PARTY_FLAG_ONLINE) != 0;
    }

    /**
     *  Whether slot `index` has state fresh and complete enough to draw.
     *
     *  Three separate reasons to say no, and they are not interchangeable: the state arrays belong to
     *  a different roster than the one we hold (state_version != roster_version), the last push is
     *  old enough to be meaningless, or the member is flagged offline or dead.
     *
     *  Deliberately does NOT exclude the local player - a caller that wants to skip itself compares
     *  against GetSelfIndex(), and the map wants a self entry while the nametags do not.
     *
     *  A member with no flags entry yet counts as visible, matching the nametag renderer: the flags
     *  array can legitimately be shorter than the roster for a push or two after somebody joins.
     */
    static bool IsMemberVisible(int index)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (index < 0)
            return false;
        if (index >= rpc.roster_uids.Count())
            return false;
        if (rpc.state_version != rpc.roster_version)
            return false;
        if ((GetGame().GetTime() - rpc.state_recv_ms) > VIGRID_PARTY_STALE_HIDE_MS)
            return false;
        if (index >= rpc.state_flags.Count())
            return true;

        //--- Compared against 0 rather than negated: `!` on an int result is not something
        //--- EnfusionScript can be relied on to convert.
        int member_flags = rpc.state_flags.Get(index);
        if ((member_flags & VIGRID_PARTY_FLAG_ONLINE) == 0)
            return false;
        if ((member_flags & VIGRID_PARTY_FLAG_ALIVE) == 0)
            return false;

        return true;
    }

    /**
     *  Ground-level world position of roster slot `index`, or vector.Zero when there is no data.
     *
     *  This is the method the whole client block exists for: it owns the choice between a live entity
     *  and an interpolated push, so state_prev_positions stays an implementation detail and two
     *  consumers cannot disagree about where a teammate is.
     *
     *  CAVEAT for `index == GetSelfIndex()`: ClientData.m_PlayerBaseList never contains the local
     *  player, so your own slot always falls through to the interpolated push and lags by up to an
     *  interval. Draw yourself from GetGame().GetPlayer() instead - do not ask this.
     */
    static vector GetMemberPosition(int index)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (index < 0)
            return vector.Zero;
        if (index >= rpc.roster_uids.Count())
            return vector.Zero;

        PlayerBase entity = FindLocalPlayer(rpc.roster_uids.Get(index));
        return ResolveBodyPos(rpc, index, entity);
    }

    //! Slot colour for a member of the CURRENT roster, at opacity `alpha` (0..1).
    static int GetMemberColour(int index, float alpha)
    {
        if (index < 0)
            return VigridPartyPalette.ColourForSlot(-1, alpha);
        if (index >= VigridPartyRPC.GetInstance().roster_uids.Count())
            return VigridPartyPalette.ColourForSlot(-1, alpha);

        return VigridPartyPalette.ColourForSlot(index, alpha);
    }

    /**
     *  Raw palette lookup, NOT roster-indexed.
     *
     *  For a slot that was recorded elsewhere and at another time - a marker stores the placer's slot
     *  server-side at placement. Resolving that through GetMemberColour would turn the marker
     *  off-white the moment its owner disconnects, and could differ between two clients whose rosters
     *  arrived in a different order. This returns the same colour on every client, for ever.
     */
    static int GetColourForSlot(int slot, float alpha)
    {
        return VigridPartyPalette.ColourForSlot(slot, alpha);
    }

    /**
     *  Bumps when the party's COMPOSITION changes - not when anybody moves. A renderer can use it to
     *  drop cached per-member state, but it is useless as a repaint trigger for positions.
     */
    static int GetRosterSeq()
    {
        return VigridPartyRPC.GetInstance().roster_seq;
    }

    //--- pings, read-only ---------------------------------------------------------------------
    //
    //  `index` here is COMPACTED: it runs 0..GetPingCount()-1 over the live pings only, so an
    //  expired one never surfaces to a caller. That costs an O(n) walk per accessor, with n capped
    //  at a handful, and buys not having to invalidate a cache on both a new push and a clock
    //  crossing an expiry.

    static int GetPingCount()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        int raw = RawPingCount();
        int now_ms = GetGame().GetTime();

        int live = 0;
        for (int i = 0; i < raw; i++)
        {
            int expire_ms = rpc.ping_expire_ms.Get(i);
            if (expire_ms > 0 && now_ms >= expire_ms)
                continue;

            live = live + 1;
        }

        return live;
    }

    static vector GetPingPos(int index)
    {
        int raw_index = RawPingIndex(index);
        if (raw_index < 0)
            return vector.Zero;

        return VigridPartyRPC.GetInstance().ping_positions.Get(raw_index);
    }

    static string GetPingOwnerUid(int index)
    {
        int raw_index = RawPingIndex(index);
        if (raw_index < 0)
            return "";

        return VigridPartyRPC.GetInstance().ping_owner_uids.Get(raw_index);
    }

    static int GetPingColour(int index, float alpha)
    {
        int raw_index = RawPingIndex(index);
        if (raw_index < 0)
            return VigridPartyPalette.ColourForSlot(-1, alpha);

        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        //--- Find returns -1 for an owner not on our roster, which the palette reads as off-white.
        //--- That happens for a frame or two when a ping set arrives before the roster explaining it.
        int owner_slot = rpc.roster_uids.Find(rpc.ping_owner_uids.Get(raw_index));
        return VigridPartyPalette.ColourForSlot(owner_slot, alpha);
    }

    //--- position resolution, shared with VigridPartyNametags ----------------------------------

    //! Locate a teammate's entity in the local network bubble. Null is normal, not an error.
    static PlayerBase FindLocalPlayer(string uid)
    {
        if (uid == "")
            return null;
        if (!ClientData.m_PlayerBaseList)
            return null;

        int count = ClientData.m_PlayerBaseList.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = PlayerBase.Cast(ClientData.m_PlayerBaseList.Get(i));
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;
            if (candidate.GetIdentity().GetPlainId() != uid)
                continue;

            return candidate;
        }

        return null;
    }

    /**
     *  Ground-level world position for roster slot `index` - what distance is measured to.
     *
     *  Keeps the `rpc` parameter it had as a private method of the nametag renderer, rather than
     *  fetching the singleton itself. Tidier the other way, but an unchanged body is what makes the
     *  move to this class provably behaviour-neutral. Taking a VigridPartyRPC also pins this method
     *  inside the client block, since that class does not exist on a server build.
     */
    static vector ResolveBodyPos(VigridPartyRPC rpc, int index, PlayerBase entity)
    {
        if (entity)
            return entity.GetPosition();

        if (index >= rpc.state_positions.Count())
            return vector.Zero;

        vector current = rpc.state_positions.Get(index);

        //--- Interpolate between the last two pushes so a distant teammate glides instead of
        //--- stepping once per interval.
        if (index < rpc.state_prev_positions.Count())
        {
            float span = rpc.state_recv_ms - rpc.state_prev_recv_ms;
            if (span > 0)
            {
                float t = Math.Clamp((GetGame().GetTime() - rpc.state_recv_ms) / span, 0, 1);
                current = vector.Lerp(rpc.state_prev_positions.Get(index), current, t);
            }
        }

        return current;
    }

    //--- debug scaffolding --------------------------------------------------------------------

#ifdef DIAG_DEVELOPER
    /**
     *  Fabricate a party of `member_count` teammates around the local player. Diag builds only.
     *
     *  A party needs three real clients to exercise properly - two partied plus one solo, because a
     *  round never advances while everyone is in one group - so every renderer that reads a roster
     *  (the HUD panel, the world nametags, the map's team layer) is otherwise untestable alone.
     *  This writes the same fields the VP_Roster and VP_TeamState handlers write, so every consumer
     *  runs unmodified.
     *
     *  Slot 0 is always the local player, exactly as a real roster has it: consumers resolve their
     *  own position from GetGame().GetPlayer() via GetSelfIndex(), and ClientData excludes the local
     *  player, so a fabricated self position would be the one value that never updates.
     *
     *  Teammates are placed on a ring around the player so they land at different bearings and
     *  distances - a cluster at one point would not exercise the nametag edge-clamp at all.
     */
    static void DebugSetRoster(int member_count)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        Man local_player = GetGame().GetPlayer();
        vector origin = vector.Zero;
        if (local_player)
            origin = local_player.GetPosition();

        rpc.roster_uids.Clear();
        rpc.roster_names.Clear();
        rpc.state_positions.Clear();
        rpc.state_prev_positions.Clear();
        rpc.state_health_level.Clear();
        rpc.state_blood_level.Clear();
        rpc.state_flags.Clear();

        //--- Slot 0: you.
        rpc.roster_uids.Insert("debug-self");
        rpc.roster_names.Insert("You");
        rpc.state_positions.Insert(origin);
        rpc.state_prev_positions.Insert(origin);
        rpc.state_health_level.Insert(0);
        rpc.state_blood_level.Insert(0);
        rpc.state_flags.Insert(VIGRID_PARTY_FLAG_ONLINE | VIGRID_PARTY_FLAG_ALIVE);

        for (int i = 0; i < member_count; i++)
        {
            float bearing = (i * 360.0) / member_count;
            float distance = 25.0 + (i * 35.0);

            vector offset = vector.Zero;
            offset[0] = Math.Sin(bearing * Math.DEG2RAD) * distance;
            offset[2] = Math.Cos(bearing * Math.DEG2RAD) * distance;

            vector member_pos = origin + offset;
            member_pos[1] = GetGame().SurfaceY(member_pos[0], member_pos[2]);

            rpc.roster_uids.Insert("debug-member-" + i);
            rpc.roster_names.Insert("Fake " + (i + 1));
            rpc.state_positions.Insert(member_pos);
            rpc.state_prev_positions.Insert(member_pos);
            rpc.state_health_level.Insert(i % 5);
            rpc.state_blood_level.Insert(i % 5);
            rpc.state_flags.Insert(VIGRID_PARTY_FLAG_ONLINE | VIGRID_PARTY_FLAG_ALIVE);
        }

        rpc.party_id = "debug-party";
        rpc.self_index = 0;
        rpc.leader_index = 0;
        rpc.roster_version = rpc.roster_version + 1;
        rpc.roster_seq = rpc.roster_seq + 1;

        //--- state_version must match roster_version or every member reads as "no usable state".
        rpc.state_version = rpc.roster_version;

        //--- Freshness. Without these IsStateStale() is true from the first frame and consumers dim
        //--- or hide the whole fabricated party.
        rpc.state_recv_ms = GetGame().GetTime();
        rpc.state_prev_recv_ms = rpc.state_recv_ms;

        VigridPartyLog.Debug("DebugSetRoster " + member_count + " fake members around " + origin);
    }

    //! Drop the fabricated party. Same shape as VigridPartyRPC.Reset's roster half.
    static void DebugClearRoster()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.roster_uids.Clear();
        rpc.roster_names.Clear();
        rpc.state_positions.Clear();
        rpc.state_prev_positions.Clear();
        rpc.state_health_level.Clear();
        rpc.state_blood_level.Clear();
        rpc.state_flags.Clear();

        rpc.party_id = "";
        rpc.self_index = -1;
        rpc.leader_index = -1;
        rpc.roster_version = rpc.roster_version + 1;
        rpc.roster_seq = rpc.roster_seq + 1;
        rpc.state_version = rpc.roster_version;

        VigridPartyLog.Debug("DebugClearRoster");
    }

    /**
     *  Drop one local ping, owned by whoever sits in slot 0 of the current roster.
     *
     *  Local only - the server knows nothing about it, so it disappears on the next real VP_PingSet.
     *  That is the point: it exercises the renderers (world markers and the map's ping glyph) with
     *  no second client and no party.
     *
     *  ttl_seconds of 0 means permanent, matching the real ping_ttl_seconds semantics.
     */
    static void DebugAddPing(vector pos, int ttl_seconds)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        string owner = "debug-self";
        if (rpc.roster_uids.Count() > 0)
            owner = rpc.roster_uids.Get(0);

        int expire_ms = 0;
        if (ttl_seconds > 0)
            expire_ms = GetGame().GetTime() + (ttl_seconds * 1000);

        rpc.ping_owner_uids.Insert(owner);
        rpc.ping_positions.Insert(pos);
        rpc.ping_expire_ms.Insert(expire_ms);
        rpc.ping_recv_ms = GetGame().GetTime();

        VigridPartyLog.Debug("DebugAddPing at " + pos + " ttl=" + ttl_seconds);
    }
#endif // DIAG_DEVELOPER

    //--- internals ----------------------------------------------------------------------------

    /**
     *  How many raw ping entries are safe to index, live or not.
     *
     *  Shortest of the three parallel arrays, because a truncated RPC can leave them out of step, and
     *  capped at the same ceiling the world-marker renderer uses so the two agree on what exists.
     */
    private static int RawPingCount()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int count = rpc.ping_owner_uids.Count();
        if (rpc.ping_positions.Count() < count)
            count = rpc.ping_positions.Count();
        if (rpc.ping_expire_ms.Count() < count)
            count = rpc.ping_expire_ms.Count();
        if (count > VIGRID_PARTY_PING_MAX_RENDERED)
            count = VIGRID_PARTY_PING_MAX_RENDERED;

        return count;
    }

    /**
     *  Raw array index of the `visible_index`-th live ping, or -1.
     *
     *  Expiry is honoured locally, to the frame, rather than waiting for the server's once-a-second
     *  sweep - the same rule VigridPartyPings applies, so a ping leaves the map and the world at the
     *  same moment. ping_expire_ms is already on the local clock; 0 means never.
     */
    private static int RawPingIndex(int visible_index)
    {
        if (visible_index < 0)
            return -1;

        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        int raw = RawPingCount();
        int now_ms = GetGame().GetTime();

        int seen = 0;
        for (int i = 0; i < raw; i++)
        {
            int expire_ms = rpc.ping_expire_ms.Get(i);
            if (expire_ms > 0 && now_ms >= expire_ms)
                continue;
            if (seen == visible_index)
                return i;

            seen = seen + 1;
        }

        return -1;
    }
#endif
}
