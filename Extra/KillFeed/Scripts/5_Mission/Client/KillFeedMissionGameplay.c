#ifndef SERVER
/**
 *  KillFeed - client lifecycle.
 *
 *  A `modded class MissionGameplay` alongside the host mod's. Both chain through super, so
 *  application order does not matter - but every override here must keep calling super, or it
 *  silently cuts the other mod out of the chain.
 */
modded class MissionGameplay
{
    protected ref KillFeedUI m_KillFeed;

    override void OnInit()
    {
        super.OnInit();

        if (!m_KillFeed)
            m_KillFeed = new KillFeedUI();

        //--- The RPC singleton outlives a server change; anything still queued belongs to the
        //--- previous session and would pop up on this one's HUD.
        KillFeedRPC.GetInstance().Reset();

        KillFeedLog.Debug("MissionGameplay::OnInit done");
    }

    override void OnMissionFinish()
    {
        //--- Free the preview entities before the world goes away, rather than leaving it to
        //--- whenever the UI object happens to be collected.
        if (m_KillFeed)
            m_KillFeed.Clear();

        m_KillFeed = NULL;

        super.OnMissionFinish();
    }

    KillFeedUI GetKillFeed()
    {
        return m_KillFeed;
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!m_KillFeed)
            return;

        m_KillFeed.Update(timeslice);
    }
}
#endif
