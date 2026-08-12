#ifdef SERVER
/**
 *  KillFeed - the death hook, and the storage for a pushed cause hint.
 *
 *  A second `modded class PlayerBase` alongside the host mod's. Both chain through super, so
 *  application order does not matter - but the override here must keep calling super, or it
 *  silently cuts the other mod out of the chain.
 */
modded class PlayerBase
{
    //--- Set through KillFeedAPI.NoteEnvironmentalDamage. Scripted damage (a play-area tick, for
    //--- instance) reaches EEKilled with the victim as their own killer, indistinguishable from
    //--- starvation - this is how the host game says which it was. Consumed on death.
    int m_KillFeedHintCause = KillFeedCause.INVALID;
    int m_KillFeedHintTime = 0;

    override void EEKilled(Object killer)
    {
        super.EEKilled(killer);

        KillFeedDeath.OnPlayerKilled(this, killer);
    }
}
#endif
