#ifndef SERVER
#ifdef DIAG_DEVELOPER

/**
 *  The whole hook. Arm the dumper once the mission is up, tick it, and close the file if the mission
 *  ends before the walk finishes.
 *
 *  The Battle Royale mod carries its own `modded class MissionGameplay`. Two modded declarations over
 *  one vanilla class chain safely as long as both call super, which is the same arrangement
 *  Extra/SpawnWithBattery and Extra/SpawnWithAmmoAndMagazine already have over ItemBase.EEOnCECreate.
 */
modded class MissionGameplay
{
    private ref DumpItemHeights m_DumpItemHeights;

    override void OnMissionStart()
    {
        super.OnMissionStart();

        m_DumpItemHeights = new DumpItemHeights();
        m_DumpItemHeights.Arm();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (m_DumpItemHeights)
            m_DumpItemHeights.Update(timeslice);
    }

    override void OnMissionFinish()
    {
        if (m_DumpItemHeights)
            m_DumpItemHeights.Close();

        super.OnMissionFinish();
    }
}

#endif // DIAG_DEVELOPER
#endif // !SERVER
