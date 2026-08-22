#ifndef SERVER
/**
 *  EarPlugs - client lifecycle and keybind.
 *
 *  A fifth `modded class MissionGameplay` alongside the Battle Royale, party, kill feed and map
 *  ones. They all chain through super, so application order does not matter - but every override
 *  here must keep calling super, or it silently cuts the other four out of the chain.
 */
modded class MissionGameplay
{
    protected ref VigridEarPlugsController m_EarPlugs;
    protected ref VigridEarPlugsHud m_EarPlugsHud;

    //! Latched so a missing bind is one line, not one line per frame. See HandleEarPlugsToggle.
    protected bool m_EarPlugsBindMissing;

    override void OnInit()
    {
        super.OnInit();

        if (!m_EarPlugs)
            m_EarPlugs = new VigridEarPlugsController();

        //--- Built here but draws nothing until its first Update, which is where it creates its
        //--- widgets - see VigridEarPlugsHud.EnsureRoot for why that cannot happen yet.
        if (!m_EarPlugsHud)
            m_EarPlugsHud = new VigridEarPlugsHud();

        VigridEarPlugsLog.Debug("MissionGameplay::OnInit done");
    }

    override void OnMissionFinish()
    {
        //--- Give the buses and the attenuation slot back BEFORE the refs go, and before super
        //--- tears the mission down. A master attenuation left set outlives this session: it would
        //--- follow the player to the main menu and into the next server they join, with no UI
        //--- anywhere to take it off again.
        if (m_EarPlugs)
            m_EarPlugs.Shutdown();

        m_EarPlugs = NULL;
        m_EarPlugsHud = NULL;

        super.OnMissionFinish();
    }

    VigridEarPlugsController GetEarPlugs()
    {
        return m_EarPlugs;
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!m_EarPlugs)
            return;

        m_EarPlugs.Update(timeslice);

        if (m_EarPlugsHud)
            m_EarPlugsHud.Update(m_EarPlugs.GetLevel());

        if (!GetUApi())
            return;

        //--- Chat only, deliberately - not "any menu". Plugging your ears while the inventory or the
        //--- map is open is a perfectly reasonable thing to want, and unlike the map's own bind this
        //--- one opens nothing and steals no focus, so there is nothing for an open menu to collide
        //--- with. Chat is the exception because J is a letter somebody is trying to type.
        if (m_UIManager.IsMenuOpen(MENU_CHAT_INPUT))
            return;

        HandleEarPlugsToggle();
    }

    /**
     *  Cycle the level on the bind.
     *
     *  The bind is resolved BY NAME, so nothing here depends on the UAVigridEarPlugsToggle constant
     *  generated from this PBO's Inputs.xml - which may not exist at compile time.
     *
     *  ⚠️ An unknown action name makes LocalPress return false forever, with no error anywhere. That
     *  is exactly how the reference implementation died silently when its inputs.xml stopped being
     *  declared, so the null check below logs once rather than failing quietly.
     */
    protected void HandleEarPlugsToggle()
    {
        UAInput toggle = GetUApi().GetInputByName(VIGRID_EARPLUGS_INPUT_TOGGLE);
        if (!toggle)
        {
            if (!m_EarPlugsBindMissing)
            {
                m_EarPlugsBindMissing = true;
                VigridEarPlugsLog.Error(VIGRID_EARPLUGS_INPUT_TOGGLE + " is not a registered input - check inputs= in Extra/EarPlugs/config.cpp");
            }
            return;
        }

        if (!toggle.LocalPress())
            return;

        m_EarPlugs.Cycle();

        if (m_EarPlugsHud)
            m_EarPlugsHud.Flash(m_EarPlugs.GetLevel());
    }
}
#endif
