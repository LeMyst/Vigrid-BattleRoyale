/**
 *  KillFeed - the public API. THIS IS THE ENTIRE CONTRACT with the host game.
 *
 *  The addon needs none of it: it hooks vanilla PlayerBase.EEKilled and produces a feed on a bare
 *  DayZ server. These calls exist so a host game that knows more than the engine does can say so -
 *  when a match is running, and why a player is losing health right now.
 *
 *  Every method is safe to call at any time, including before anything is initialised, so a host
 *  game never has to null-check.
 *
 *  Usage from the host game (guard every call site, so removing the addon still builds):
 *
 *      #ifdef KILLFEED
 *          KillFeedAPI.SetActive(true);
 *      #endif
 *
 *  No file-level guard - each half is #ifdef-ed inside the class body, the shape VigridPartyAPI
 *  uses. The server half is the match-state contract; the client half is diag-only test scaffolding.
 */
class KillFeedAPI
{
#ifdef SERVER
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
#endif

#ifndef SERVER
#ifdef DIAG_DEVELOPER
    /**
     *  Queue one synthetic row. Diag builds only.
     *
     *  A kill feed is the one thing a single client can never produce for itself - it takes two
     *  players and a death - so every visual defect in a row otherwise has to survive a two-client
     *  test to be found. This lets a developer push a row and look at it.
     *
     *  Rows go in through the same queue the network handler fills, so a fake row is rendered by
     *  exactly the code that renders a real one, including the ECE_LOCAL preview entity and its
     *  attachments. Nothing here fakes the rendering.
     *
     *  Pass "" for weapon_type to exercise the icon-and-phrase middle cell instead of the model,
     *  and -1 for distance to hide that field - the conventions KillFeedEntry documents.
     */
    static void DebugPush(string killer_name, string victim_name, string weapon_type, string attachments, int distance, int cause)
    {
        KillFeedRPC rpc = KillFeedRPC.GetInstance();
        if (!rpc)
            return;

        rpc.pending.Insert(new KillFeedEntry(killer_name, victim_name, weapon_type, attachments, distance, cause));
        KillFeedLog.Debug("DebugPush " + killer_name + " -> " + victim_name + " cause=" + cause);
    }

    //! Rows the feed can show at once. Exposed so a caller can overflow it deliberately - eviction
    //! and the matching Release() are otherwise unreachable without four real deaths in a row.
    static int GetMaxRows()
    {
        return KILLFEED_MAX_ROWS;
    }

    //! The separator DebugPush expects inside `attachments`, so a caller never hardcodes it.
    static string GetAttachmentSeparator()
    {
        return KILLFEED_ATTACHMENT_SEPARATOR;
    }
#endif // DIAG_DEVELOPER
#endif // !SERVER
}
