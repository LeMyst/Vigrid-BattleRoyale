#ifdef SERVER
/**
 *  KillFeed - the public API. THIS IS THE ENTIRE CONTRACT with the host game.
 *
 *  The addon needs none of it: it hooks vanilla PlayerBase.EEKilled and produces a feed on a bare
 *  DayZ server. These two calls exist so a host game that knows more than the engine does can say
 *  so - when a match is running, and why a player is losing health right now.
 *
 *  Every method is safe to call at any time, including before anything is initialised, so a host
 *  game never has to null-check.
 *
 *  Usage from the host game (guard every call site, so removing the addon still builds):
 *
 *      #ifdef KILLFEED
 *          KillFeedAPI.SetActive(true);
 *      #endif
 */
class KillFeedAPI
{
    /**
     *  Turn the feed on or off at runtime, on top of the `enabled` setting. A host game with a
     *  lobby phase wants it off there: kills before the match are not part of the story, and a
     *  feed running in the lobby just leaks who is fighting whom.
     *
     *  Not persisted - a fresh server starts with the feed on.
     */
    static void SetActive(bool active)
    {
        KillFeedDeath.SetActive(active);
    }

    static bool IsActive()
    {
        return KillFeedDeath.IsActive();
    }

    /**
     *  Record why `victim` is currently taking damage, so that a death in the next few seconds is
     *  labelled with `cause` instead of the generic environmental fallback.
     *
     *  Needed because scripted damage - DecreaseHealthCoef and friends - surfaces in EEKilled with
     *  the victim as their own killer, which is indistinguishable from starving or falling. Call it
     *  wherever the damage is applied; it is cheap enough to call on every tick.
     *
     *  The hint expires after KILLFEED_HINT_TTL_MS and is consumed by the death that uses it, so a
     *  stale one can never mislabel a later, unrelated death.
     */
    static void NoteEnvironmentalDamage(PlayerBase victim, int cause)
    {
        if (!victim)
            return;

        victim.m_KillFeedHintCause = cause;
        victim.m_KillFeedHintTime = GetGame().GetTime();
    }
}
#endif
