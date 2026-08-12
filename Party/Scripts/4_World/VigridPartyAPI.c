#ifdef SERVER
/**
 *  Vigrid Party - the public API. THIS IS THE ENTIRE CONTRACT with the host game.
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
 *  Usage from the host game (guard every call site, so removing party.pbo still builds):
 *
 *      #ifdef VIGRID_PARTY
 *          int groups = VigridPartyAPI.GetGroupCount(GetPlayers());
 *      #endif
 */
class VigridPartyAPI
{
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
}
#endif
