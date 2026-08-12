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

        if (HandlePartyClose())
            return;

        //--- Nothing this addon binds may fire while any menu at all is open. A ping is an 8 km
        //--- camera raycast against a world the player is not currently looking at, and the menu
        //--- bind is already handled above for the one menu that is ours.
        //---
        //--- "Any menu", not a list of ids - the list this replaces named only MENU_MAP, which is
        //--- vanilla's map rather than the Vigrid one, so both binds still fired under the Vigrid
        //--- map, the inventory, the in-game Esc menu and the death screen. Enumerating ids drifts;
        //--- the map addon learned the same lesson at MapMissionGameplay.HandleMapOpen. Note this
        //--- deliberately stays id-agnostic: Party must not reference MENU_VIGRID_MAP or anything
        //--- else owned by Extra/Map.
        if (m_UIManager.GetMenu())
            return;

        HandlePingInput();
        HandlePartyOpen();
    }

    /**
     *  Close the party menu, on Esc or on a second press of the bind.
     *
     *  Esc has to be polled here rather than left to the engine. While ANY scripted menu is open,
     *  MissionGameplay.OnUpdate never reaches its `else if (UAUIMenu.LocalPress()) Pause()` branch
     *  (missiongameplay.c:691), so Esc over the party menu was a dead key rather than a competing
     *  one - nothing closed and nothing opened. Every vanilla menu answers this the same way, by
     *  reading UAUIBack itself; the vanilla map does it at mapmenu.c:296.
     *
     *  UAUIBack is read through GetInputByID, not by name: it is a vanilla input and its constant is
     *  always present, unlike this PBO's own binds.
     *
     *  @return true when the menu was closed, so the caller stops before the open path re-opens it.
     */
    protected bool HandlePartyClose()
    {
        if (!m_UIManager.IsMenuOpen(MENU_VIGRID_PARTY))
            return false;

        if (GetUApi().GetInputByID(UAUIBack).LocalPress())
        {
            m_UIManager.CloseMenu(MENU_VIGRID_PARTY);
            return true;
        }

        UAInput close_input = GetUApi().GetInputByName(VIGRID_PARTY_INPUT_MENU);
        if (!close_input)
            return false;
        if (!close_input.LocalPress())
            return false;

        m_UIManager.CloseMenu(MENU_VIGRID_PARTY);
        return true;
    }

    /**
     *  Open the party menu. The caller has already established that nothing else is open.
     *
     *  Resolved by name so nothing here depends on the UAVigridPartyMenu constant, which is
     *  generated from another PBO's Inputs.xml.
     */
    protected void HandlePartyOpen()
    {
        UAInput party_input = GetUApi().GetInputByName(VIGRID_PARTY_INPUT_MENU);
        if (!party_input)
            return;
        if (!party_input.LocalPress())
            return;

        //--- NULL parent, not GetMenu(): the gate in OnUpdate proves nothing is open, and adopting
        //--- whatever was there is what made the in-game Esc menu the party menu's parent, so
        //--- closing the party menu handed focus back to a screen the player had already left.
        GetUIManager().EnterScriptedMenu(MENU_VIGRID_PARTY, NULL);
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
