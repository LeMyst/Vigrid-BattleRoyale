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
}
#endif
