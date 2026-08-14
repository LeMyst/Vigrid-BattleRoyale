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

    /**
     *  Hide or show one connected member in the state their party receives.
     *
     *  A hidden member is presented to their teammates exactly as an offline one - no position, no
     *  world nametag, no compass caret, no marker on the map - while remaining a full member: still
     *  on the roster, still listed by name, still counted by GetGroupCount, and unaffected in every
     *  grouping query. Nothing else about the party changes.
     *
     *  WHY THIS EXISTS, stated as a general contract rather than the host's use case: a host mod can
     *  put a player somewhere that is not where they are playing from. Party cannot detect that and
     *  must not try - it has no concept of a match, a camera or a spectator - so the host asserts it.
     *
     *  Idempotent, session-scoped, and never persisted: parties outlive the process and this must
     *  not, or a member could come back invisible with nothing left to clear the flag.
     *
     *  ⚠️ The caller owns the clear. Set it false on every path out - including the ones that are
     *  not a clean exit, since a member left hidden reads to their party as permanently offline.
     */
    static void SetMemberHidden(string uid, bool hidden)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return;

        manager.SetMemberHidden(uid, hidden);
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

    /**
     *  Fill `population` into parties of at least `min_size`, creating parties as needed.
     *
     *  Existing parties are topped up before any new one is started, and one that already meets
     *  `min_size` is never touched and never split. Each new party's members - and therefore its
     *  leader, which is whoever went in first - are drawn in random order.
     *
     *  ⚠️ `min_groups` is a FLOOR ON THE RESULTING GROUP COUNT AND IT OUTRANKS `min_size`. A host
     *  that counts groups down to a winner needs at least two of them to count with, and merging
     *  the whole population into one party would end the match on its first tick. When the floor
     *  and the minimum size cannot both be honoured the pass stops early and leaves players
     *  short-handed; it never returns fewer than `min_groups` groups.
     *
     *  `remainder` is one of VIGRID_PARTY_REMAINDER_* and decides what happens to the players left
     *  over once no full party can be made from them - fewer than `min_size` of them, by definition,
     *  so some rule has to bend. ABSORB may exceed max_party_size; the other two never do.
     *
     *  Members added here are session-scoped: they are subtracted before parties.json is written, so
     *  an assignment made for one match cannot follow anybody into the next. Everything else about
     *  the party is normal - the roster, the HUD, the nametags, the pings and every grouping query.
     *
     *  Safe either side of SetFormationLocked (the lock only gates player requests), but call it
     *  BEFORE locking, or players are told composition is frozen and then watch it change.
     *
     *  Returns the resulting group count, or -1 when nothing was done - the addon is disabled,
     *  `min_size` is 1 or less, or the population is empty.
     */
    static int AutoGroup(array<PlayerBase> population, int min_size, int min_groups, int remainder)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return -1;

        return manager.AutoGroupPopulation(population, min_size, min_groups, remainder);
    }

    /**
     *  Plan `cases` synthetic populations with the settings AutoGroup would use and log the result.
     *  Changes nothing and touches no player - it is arithmetic against the same planner.
     *
     *  Worth running once after changing min_size: the cases that can strand a player or collapse
     *  the group count are combinatorial, and a live test rig cannot produce them.
     */
    static void AutoGroupSelfTest(int cases, int min_size, int min_groups, int remainder)
    {
        VigridPartyManager manager = VigridPartyManager.GetInstance();
        if (!manager || !manager.IsEnabled())
            return;

        VigridPartyAutoGroup.SelfTest(cases, min_size, manager.GetMaxPartySize(), min_groups, remainder);
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

    //--- Per-frame uid -> entity index over ClientData.m_PlayerBaseList. See FindLocalPlayer.
    private static ref map<string, PlayerBase> s_LocalByUid;
    private static int s_LocalByUidMs = -1;
    private static int s_LocalByUidCount = -1;

    /**
     *  Locate a teammate's entity in the local network bubble. Null is normal, not an error.
     *
     *  MEMOIZED PER FRAME, because the callers are per-frame renderers and there are several of
     *  them. This was a linear scan of the whole population per call, and every call site asks it
     *  once PER PARTY MEMBER: VigridPartyNametags and VigridMapCompass.CollectCarets both run every
     *  frame, VigridMapMinimap and VigridMapMenu at 10 Hz. So a four-player party cost six-odd full
     *  walks per frame, and each walk materialises a fresh string from GetPlainId() for every
     *  candidate it rejects. That is O(members x population) string allocations per frame, and it is
     *  worst exactly where the population is densest - the lobby, where everyone stands in one
     *  clearing and so everyone is inside everyone else's bubble.
     *
     *  The index makes it one walk per frame plus a hash lookup per call. Built lazily, so a solo
     *  player with no roster to draw never pays for it at all.
     *
     *  Invalidated on the millisecond AND on the population count. The clock alone is enough at any
     *  real frame rate - 60 fps is 16 ms apart - but an entity entering or leaving the bubble within
     *  the same millisecond would otherwise be missed for that frame, and the count is one integer
     *  compare. Entries are still null-checked on the way out: an entity deleted after the index was
     *  built reads as null through its reference, exactly as it did mid-walk before.
     */
    static PlayerBase FindLocalPlayer(string uid)
    {
        if (uid == "")
            return null;
        if (!ClientData.m_PlayerBaseList)
            return null;

        int count = ClientData.m_PlayerBaseList.Count();
        int now = GetGame().GetTime();

        if (now != s_LocalByUidMs || count != s_LocalByUidCount || !s_LocalByUid)
            RebuildLocalIndex(count, now);

        PlayerBase found = s_LocalByUid.Get(uid);
        if (!found)
            return null;

        return found;
    }

    //! One walk of the population, keyed by uid. The only place the scan still happens.
    private static void RebuildLocalIndex(int count, int now)
    {
        if (!s_LocalByUid)
            s_LocalByUid = new map<string, PlayerBase>();
        else
            s_LocalByUid.Clear();

        s_LocalByUidMs = now;
        s_LocalByUidCount = count;

        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = PlayerBase.Cast(ClientData.m_PlayerBaseList.Get(i));
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;

            //--- Read out before the call that consumes it, per the container-aliasing rule.
            string candidate_uid = candidate.GetIdentity().GetPlainId();

            //--- FIRST WINS, because the linear scan this replaces returned on its first match.
            //--- Two entities sharing a uid is not something we expect - but a corpse is never
            //--- deleted by this mod and stays in the population list, so it is not obviously
            //--- impossible either, and "same answer as before" is worth one Contains() per entry.
            if (s_LocalByUid.Contains(candidate_uid))
                continue;

            s_LocalByUid.Set(candidate_uid, candidate);
        }
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
     *  A LOCAL SANDBOX FOR THE PARTY UI. Diag builds only.
     *
     *  A party needs three real clients to exercise properly - two partied plus one solo, because a
     *  round never advances while everyone is in one group - so nothing that reads a roster (the HUD
     *  panel, the world nametags, the map's team layer, the menu itself) is testable alone.
     *
     *  Two halves, and the second is what makes the menu interactive. DebugSetRoster fabricates the
     *  ROSTER, which is the menu's right column. DebugSetPlayerList fabricates the ONLINE LIST,
     *  which is its left column and the thing every outgoing action needs a target from. Everything
     *  below writes the same fields the VP_* handlers write, so every consumer runs unmodified.
     *
     *  THE LATCH. Setting debug_fake_session makes VigridPartyRPC discard every incoming server push
     *  that would overwrite fabricated state - see VigridPartyRPC.DebugSuppressesPush. Without it a
     *  fake list survives at most one poll on a live server, because VigridPartyMenu asks for the
     *  real one every three seconds. VigridPartyClient's send methods read the same latch and apply
     *  each command here instead of putting it on the wire, so Invite / Kick / Promote / Leave /
     *  Disband / Accept / Decline all act on the fabrication. DebugClearFakes drops it again.
     */

    //--- The fabricated identities. Uids only have to be unique and recognisable in a log: nothing
    //--- resolves them against a real player, and FindLocalPlayer simply never matches one.
    private static const string DEBUG_PARTY_ID = "debug-party";
    private static const string DEBUG_SELF_UID = "debug-self";
    private static const string DEBUG_SELF_NAME = "You";
    private static const string DEBUG_MEMBER_PREFIX = "debug-member-";
    private static const string DEBUG_ONLINE_PREFIX = "debug-online-";
    private static const string DEBUG_INVITE_ID = "debug-invite";
    private static const string DEBUG_INVITER_UID = "debug-inviter";
    private static const string DEBUG_INVITER_NAME = "Fake Inviter";

    //! Whether the diag menu is currently driving a fabricated party. The one thing 5_Mission asks.
    static bool IsDebugFakeSession()
    {
        return VigridPartyRPC.GetInstance().debug_fake_session;
    }

    /**
     *  Where the `ring_slot`-th teammate stands.
     *
     *  Teammates go on a ring around the player so they land at different bearings and distances - a
     *  cluster at one point would not exercise the nametag edge-clamp at all.
     */
    private static vector DebugRingPos(vector origin, int ring_slot, int ring_total)
    {
        if (ring_total < 1)
            ring_total = 1;

        float bearing = (ring_slot * 360.0) / ring_total;
        float distance = 25.0 + (ring_slot * 35.0);

        vector offset = vector.Zero;
        offset[0] = Math.Sin(bearing * Math.DEG2RAD) * distance;
        offset[2] = Math.Cos(bearing * Math.DEG2RAD) * distance;

        vector result = origin + offset;
        result[1] = GetGame().SurfaceY(result[0], result[2]);
        return result;
    }

    /**
     *  Refill state_* from whatever the fabricated roster currently holds, and raise the repaint
     *  edges. EVERY mutation below ends here.
     *
     *  Three invariants live in this one place, and each of them has to hold or the party silently
     *  disappears rather than failing:
     *
     *  - the state_* arrays must be EXACTLY as long as roster_uids, since consumers index them with
     *    one counter;
     *  - state_version must equal roster_version, or IsMemberOnline and IsMemberVisible both read
     *    "no usable state";
     *  - state_recv_ms / state_prev_recv_ms must be current, or IsStateStale() is true from the
     *    first frame and every renderer dims or hides the whole party.
     *
     *  roster_seq is bumped here too. It is the menu's ONLY repaint trigger for the member column
     *  (VigridPartyMenu.Update), and it also forces the online column to repaint, so a mutation that
     *  did not bump it would not be visible until something else changed.
     *
     *  Flags are reset to online+alive for every slot, so a member toggled offline comes back online
     *  on the next mutation. That is a deliberate simplification: carrying flags across an insert or
     *  a remove means tracking them by uid, and re-pressing Toggle Member Offline is one keystroke.
     */
    private static void DebugRebuildState()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        Man local_player = GetGame().GetPlayer();
        vector origin = vector.Zero;
        if (local_player)
            origin = local_player.GetPosition();

        rpc.state_positions.Clear();
        rpc.state_prev_positions.Clear();
        rpc.state_health_level.Clear();
        rpc.state_blood_level.Clear();
        rpc.state_flags.Clear();

        int count = rpc.roster_uids.Count();

        //--- Your own slot takes no ring position, so the ring is one shorter than the roster.
        int ring_total = count;
        if (rpc.self_index >= 0 && rpc.self_index < count)
            ring_total = count - 1;

        int ring_slot = 0;

        for (int i = 0; i < count; i++)
        {
            //--- Your own slot keeps the real position. ClientData excludes the local player, so a
            //--- fabricated self position would be the one value that never updates - which is also
            //--- why GetMemberPosition must not be asked for it.
            vector member_pos = origin;
            if (i != rpc.self_index)
            {
                member_pos = DebugRingPos(origin, ring_slot, ring_total);
                ring_slot = ring_slot + 1;
            }

            rpc.state_positions.Insert(member_pos);
            rpc.state_prev_positions.Insert(member_pos);
            rpc.state_health_level.Insert(i % 5);
            rpc.state_blood_level.Insert(i % 5);
            rpc.state_flags.Insert(VIGRID_PARTY_FLAG_ONLINE | VIGRID_PARTY_FLAG_ALIVE);
        }

        rpc.roster_version = rpc.roster_version + 1;
        rpc.roster_seq = rpc.roster_seq + 1;
        rpc.state_version = rpc.roster_version;
        rpc.state_recv_ms = GetGame().GetTime();
        rpc.state_prev_recv_ms = rpc.state_recv_ms;
    }

    //! Index of `uid` in the fabricated online list, or -1.
    private static int DebugFindListIndex(string uid)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int count = rpc.list_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if (rpc.list_uids.Get(i) == uid)
                return i;
        }

        return -1;
    }

    //! Index of `uid` in the fabricated roster, or -1.
    private static int DebugFindRosterIndex(string uid)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int count = rpc.roster_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if (rpc.roster_uids.Get(i) == uid)
                return i;
        }

        return -1;
    }

    //! Put one fabricated player onto the online list. Never flagged as already partied: a member
    //! that just left one is by definition available again.
    private static void DebugAppendToList(string uid, string name)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.list_uids.Insert(uid);
        rpc.list_names.Insert(name);
        rpc.list_flags.Insert(0);
        rpc.list_seq = rpc.list_seq + 1;
    }

    //! Hand every teammate back to the online list, so leaving or disbanding is a loop rather than
    //! a one-way trip that empties the left column too.
    private static void DebugReturnMembersToList()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int count = rpc.roster_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if (i == rpc.self_index)
                continue;

            DebugAppendToList(rpc.roster_uids.Get(i), rpc.roster_names.Get(i));
        }
    }

    /**
     *  Fabricate a party of `member_count` teammates around the local player.
     *
     *  Slot 0 is always you, exactly as a real roster has it, and you are the leader - which is the
     *  branch that shows Promote and Kick on every other row.
     */
    static void DebugSetRoster(int member_count)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.roster_uids.Clear();
        rpc.roster_names.Clear();

        //--- Slot 0: you.
        rpc.roster_uids.Insert(DEBUG_SELF_UID);
        rpc.roster_names.Insert(DEBUG_SELF_NAME);

        for (int i = 0; i < member_count; i++)
        {
            rpc.roster_uids.Insert(DEBUG_MEMBER_PREFIX + i);
            rpc.roster_names.Insert("Fake " + (i + 1));
        }

        rpc.party_id = DEBUG_PARTY_ID;
        rpc.self_index = 0;
        rpc.leader_index = 0;
        rpc.debug_fake_session = true;

        DebugRebuildState();

        VigridPartyLog.Debug("DebugSetRoster " + member_count + " fake members");
    }

    //! Drop the fabricated party, keeping the online list and the latch. DebugClearFakes is the one
    //! that hands the session back to the real server.
    static void DebugClearRoster()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.roster_uids.Clear();
        rpc.roster_names.Clear();

        rpc.party_id = "";
        rpc.self_index = -1;
        rpc.leader_index = -1;

        DebugRebuildState();

        VigridPartyLog.Debug("DebugClearRoster");
    }

    /**
     *  Fabricate `count` connected players for the menu's left column.
     *
     *  This is the half DebugSetRoster never covered. The left column is VigridPartyRPC.list_*,
     *  filled only by a real VP_PlayerList reply, so solo there is nobody to invite and every
     *  outgoing action in the menu was unreachable.
     *
     *  Names are plain strings on purpose: VigridPartyMenu.RefreshOnline sets the row text straight
     *  from list_names, without the leading-'#' translation GetMemberName does for the roster, so a
     *  stringtable key here would render as the key.
     *
     *  Every third entry is flagged as already being in a party - bit0, the same bit the server
     *  sets - which is the only way to see the row whose Invite button is hidden.
     */
    static void DebugSetPlayerList(int count)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.list_uids.Clear();
        rpc.list_names.Clear();
        rpc.list_flags.Clear();

        for (int i = 0; i < count; i++)
        {
            rpc.list_uids.Insert(DEBUG_ONLINE_PREFIX + i);
            rpc.list_names.Insert("Fake Player " + (i + 1));

            int entry_flags = 0;
            if ((i % 3) == 2)
                entry_flags = 1;

            rpc.list_flags.Insert(entry_flags);
        }

        rpc.debug_fake_session = true;
        rpc.list_seq = rpc.list_seq + 1;

        VigridPartyLog.Debug("DebugSetPlayerList " + count + " fake players");
    }

    //! Start a party of one, as the Create button does.
    static void DebugCreateParty()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.roster_uids.Clear();
        rpc.roster_names.Clear();
        rpc.roster_uids.Insert(DEBUG_SELF_UID);
        rpc.roster_names.Insert(DEBUG_SELF_NAME);

        rpc.party_id = DEBUG_PARTY_ID;
        rpc.self_index = 0;
        rpc.leader_index = 0;
        rpc.debug_fake_session = true;

        DebugRebuildState();

        VigridPartyLog.Debug("DebugCreateParty");
    }

    /**
     *  Move a fabricated player from the online list into the party.
     *
     *  A real invite is a round trip the invitee has to accept; there is nobody to accept here, so
     *  this collapses to the outcome. Inviting with no party creates one first, matching the
     *  server's own VP_Invite.
     */
    static void DebugInvite(string uid)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int list_index = DebugFindListIndex(uid);
        if (list_index < 0)
        {
            VigridPartyLog.Debug("DebugInvite: " + uid + " is not in the fake player list");
            return;
        }

        if (rpc.roster_uids.Count() == 0)
            DebugCreateParty();

        string invited_name = rpc.list_names.Get(list_index);

        //--- RemoveOrdered, never Remove: vanilla's Remove() fills the hole with the LAST element,
        //--- which would reorder the column under the player's cursor between repaints.
        rpc.list_uids.RemoveOrdered(list_index);
        rpc.list_names.RemoveOrdered(list_index);
        rpc.list_flags.RemoveOrdered(list_index);
        rpc.list_seq = rpc.list_seq + 1;

        rpc.roster_uids.Insert(uid);
        rpc.roster_names.Insert(invited_name);

        DebugRebuildState();

        VigridPartyLog.Debug("DebugInvite " + uid + " joined the fake party");
    }

    /**
     *  Fabricate an incoming invitation, so the banner, the chat prompt and the two buttons under it
     *  are reachable without a second client.
     */
    static void DebugReceiveInvite()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.invite_id = DEBUG_INVITE_ID;
        rpc.invite_inviter_uid = DEBUG_INVITER_UID;
        rpc.invite_inviter_name = DEBUG_INVITER_NAME;
        rpc.invite_expires_ms = GetGame().GetTime() + (rpc.invite_ttl_seconds * 1000);
        rpc.invite_seq = rpc.invite_seq + 1;
        rpc.debug_fake_session = true;

        VigridPartyLog.Debug("DebugReceiveInvite from " + DEBUG_INVITER_NAME);
    }

    /**
     *  Answer the fabricated invitation.
     *
     *  Accepting joins SOMEBODY ELSE'S party - the inviter takes slot 0 and you take slot 1. That is
     *  the only way to reach the non-leader branches: RefreshOnline hides every Invite button and
     *  RefreshMembers hides Promote and Kick whenever self_index != leader_index.
     */
    static void DebugRespondToInvite(bool accept)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.ClearInvite();
        rpc.invite_seq = rpc.invite_seq + 1;

        if (!accept)
        {
            VigridPartyLog.Debug("DebugRespondToInvite declined");
            return;
        }

        rpc.roster_uids.Clear();
        rpc.roster_names.Clear();
        rpc.roster_uids.Insert(DEBUG_INVITER_UID);
        rpc.roster_names.Insert(DEBUG_INVITER_NAME);
        rpc.roster_uids.Insert(DEBUG_SELF_UID);
        rpc.roster_names.Insert(DEBUG_SELF_NAME);

        rpc.party_id = DEBUG_PARTY_ID;
        rpc.self_index = 1;
        rpc.leader_index = 0;
        rpc.debug_fake_session = true;

        DebugRebuildState();

        VigridPartyLog.Debug("DebugRespondToInvite accepted, you are not the leader");
    }

    //! Remove a teammate and hand them back to the online list. Dissolves the party below two
    //! members, exactly as the server's RemoveMemberInternal does.
    static void DebugKick(string uid)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int roster_index = DebugFindRosterIndex(uid);
        if (roster_index < 0)
            return;

        string kicked_name = rpc.roster_names.Get(roster_index);

        rpc.roster_uids.RemoveOrdered(roster_index);
        rpc.roster_names.RemoveOrdered(roster_index);

        DebugAppendToList(uid, kicked_name);

        //--- Removing a slot shifts every later slot down by one.
        if (rpc.self_index > roster_index)
            rpc.self_index = rpc.self_index - 1;
        if (rpc.leader_index > roster_index)
            rpc.leader_index = rpc.leader_index - 1;

        VigridPartyLog.Debug("DebugKick " + uid);

        if (rpc.roster_uids.Count() < 2)
        {
            DebugClearRoster();
            return;
        }

        DebugRebuildState();
    }

    //! Hand leadership to a teammate.
    static void DebugPromote(string uid)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int roster_index = DebugFindRosterIndex(uid);
        if (roster_index < 0)
            return;

        rpc.leader_index = roster_index;

        //--- Nothing about the members themselves changed, but roster_seq is the menu's only repaint
        //--- trigger for that column, so the leader marker would not move until something else did.
        DebugRebuildState();

        VigridPartyLog.Debug("DebugPromote " + uid);
    }

    //! Leave the fabricated party. Teammates go back onto the online list so the loop can be run
    //! again without re-applying the fake list.
    static void DebugLeaveParty()
    {
        DebugReturnMembersToList();
        DebugClearRoster();

        VigridPartyLog.Debug("DebugLeaveParty");
    }

    //! Disband it. Locally indistinguishable from leaving - the difference is who else it reaches,
    //! and here there is nobody else.
    static void DebugDisbandParty()
    {
        DebugReturnMembersToList();
        DebugClearRoster();

        VigridPartyLog.Debug("DebugDisbandParty");
    }

    /**
     *  Flip the last teammate between online and offline, so the grey "(Offline)" row and the HUD's
     *  inactive styling are reachable.
     *
     *  Never your own slot: the renderers read your position from GetGame().GetPlayer() regardless,
     *  so marking yourself offline would show nothing and confuse the reading of everything else.
     */
    static void DebugToggleMemberOffline()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        int target = -1;
        int count = rpc.roster_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if (i == rpc.self_index)
                continue;

            target = i;
        }

        if (target < 0)
            return;
        if (target >= rpc.state_flags.Count())
            return;

        //--- Added and subtracted rather than masked with a complement: the bit's state is already
        //--- known here, and EnfusionScript's support for '~' is not worth depending on.
        int flags = rpc.state_flags.Get(target);
        bool online = (flags & VIGRID_PARTY_FLAG_ONLINE) != 0;

        if (online)
            flags = flags - VIGRID_PARTY_FLAG_ONLINE;
        else
            flags = flags + VIGRID_PARTY_FLAG_ONLINE;

        rpc.state_flags.Set(target, flags);

        //--- roster_seq rather than a state push: RefreshMembers only repaints on a roster change,
        //--- so the grey row would otherwise not appear until the composition changed. This is the
        //--- one mutation that must NOT go through DebugRebuildState, which would reset the flag.
        rpc.roster_seq = rpc.roster_seq + 1;

        VigridPartyLog.Debug("DebugToggleMemberOffline slot " + target + " online=" + !online);
    }

    /**
     *  Drop everything fabricated AND the latch, handing the session back to the real server.
     *
     *  The latch is the whole point of this method. While it is set every server push is discarded,
     *  so leaving it on would silently freeze the client's party state for the rest of the session -
     *  and present as a networking bug rather than as a debug switch left down.
     */
    static void DebugClearFakes()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        DebugClearRoster();

        rpc.list_uids.Clear();
        rpc.list_names.Clear();
        rpc.list_flags.Clear();
        rpc.list_seq = rpc.list_seq + 1;

        rpc.ClearInvite();
        rpc.invite_seq = rpc.invite_seq + 1;

        rpc.ping_owner_uids.Clear();
        rpc.ping_positions.Clear();
        rpc.ping_expire_ms.Clear();
        rpc.ping_recv_ms = GetGame().GetTime();

        rpc.debug_fake_session = false;

        VigridPartyLog.Debug("DebugClearFakes - real server state resumes");
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

        string owner = DEBUG_SELF_UID;
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

    //! Drop every local marker. The fabricated counterpart of VP_PingClear, which cannot round-trip
    //! while the latch is discarding the VP_PingSet that would answer it.
    static void DebugClearPings()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        rpc.ping_owner_uids.Clear();
        rpc.ping_positions.Clear();
        rpc.ping_expire_ms.Clear();
        rpc.ping_recv_ms = GetGame().GetTime();

        VigridPartyLog.Debug("DebugClearPings");
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
