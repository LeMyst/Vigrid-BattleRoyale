#ifndef SERVER
/**
 *  Vigrid Party - client lifecycle and keybind.
 *
 *  A second `modded class MissionGameplay` alongside the Battle Royale one. Both chain through
 *  super, so application order does not matter - but every override here must keep calling super,
 *  or it silently cuts the other mod out of the chain.
 */
modded class MissionGameplay
{
    protected ref VigridPartyClient m_VigridParty;

    override void OnInit()
    {
        VigridPartyLog.Debug("MissionGameplay::OnInit entering super");
        super.OnInit();
        VigridPartyLog.Debug("MissionGameplay::OnInit super returned");

        if (!m_VigridParty)
            m_VigridParty = new VigridPartyClient();

        VigridPartyLog.Debug("MissionGameplay::OnInit done");
    }

    override void OnMissionFinish()
    {
        m_VigridParty = NULL;

        super.OnMissionFinish();
    }

    VigridPartyClient GetVigridParty()
    {
        return m_VigridParty;
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!m_VigridParty)
            return;

        m_VigridParty.Update(timeslice);

        if (!GetUApi())
            return;
        if (m_UIManager.IsMenuOpen(MENU_CHAT_INPUT))
            return;

        //--- The map takes the whole screen and its own input; a bind firing underneath it would
        //--- act on a world the player is not currently looking at.
        if (m_UIManager.IsMenuOpen(MENU_MAP))
            return;

        //--- Pings are suppressed while the party menu is open, but the menu bind below is not -
        //--- pressing it again is how the menu closes.
        if (!m_UIManager.IsMenuOpen(MENU_VIGRID_PARTY))
            HandlePingInput();

        //--- Resolved by name so nothing here depends on the UAVigridPartyMenu constant, which is
        //--- generated from another PBO's Inputs.xml.
        UAInput party_input = GetUApi().GetInputByName(VIGRID_PARTY_INPUT_MENU);
        if (!party_input)
            return;
        if (!party_input.LocalPress())
            return;

        if (m_UIManager.IsMenuOpen(MENU_VIGRID_PARTY))
        {
            m_UIManager.CloseMenu(MENU_VIGRID_PARTY);
            return;
        }

        GetUIManager().EnterScriptedMenu(MENU_VIGRID_PARTY, GetUIManager().GetMenu());
    }

    /**
     *  Poll the two ping binds. LocalPress() is edge-triggered, so holding the key does nothing
     *  after the first frame and the cooldown never has to fight an auto-repeat.
     *
     *  Resolved by name for the same reason the menu bind is: the generated UAVigridPartyPing
     *  constant comes from another PBO's Inputs.xml and nothing here should depend on it.
     */
    private void HandlePingInput()
    {
        UAInput ping_input = GetUApi().GetInputByName(VIGRID_PARTY_INPUT_PING);
        if (ping_input && ping_input.LocalPress())
            m_VigridParty.PlacePing();

        UAInput clear_input = GetUApi().GetInputByName(VIGRID_PARTY_INPUT_PING_CLEAR);
        if (clear_input && clear_input.LocalPress())
            m_VigridParty.ClearPings();
    }
}
#endif
