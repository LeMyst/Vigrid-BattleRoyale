#ifndef SERVER
/**
 *  Vigrid Map - client lifecycle and keybind.
 *
 *  A fourth `modded class MissionGameplay` alongside the Battle Royale, party and kill feed ones.
 *  They all chain through super, so application order does not matter - but every override here
 *  must keep calling super, or it silently cuts the others out of the chain.
 *
 *  Every minimap reference in this file is behind VIGRID_MAP_MINIMAP, declared in Extra/Map/config.cpp
 *  - comment that define out and the minimap is gone from the build entirely, keybind handler
 *  included. See VigridMapMinimap.c for what stays behind on purpose.
 */
modded class MissionGameplay
{
    protected ref VigridMapClient m_VigridMap;

#ifdef VIGRID_MAP_MINIMAP
    protected ref VigridMapMinimap m_VigridMinimap;
#endif

    override void OnInit()
    {
        super.OnInit();

        //--- The RPC singleton outlives a server change; anything still held belongs to the
        //--- previous session and would draw on this one's map. Reset BEFORE the controller is
        //--- built, because its constructor asks the server for a fresh set.
        VigridMapRPC.GetInstance().Reset();

        if (!m_VigridMap)
            m_VigridMap = new VigridMapClient();

#ifdef VIGRID_MAP_MINIMAP
        //--- Built here but drawn nothing until its first Update, which is where it creates its
        //--- widgets - see VigridMapMinimap.EnsureRoot for why that cannot happen yet.
        if (!m_VigridMinimap)
            m_VigridMinimap = new VigridMapMinimap();
#endif

        VigridMapLog.Debug("MissionGameplay::OnInit done");
    }

    override void OnMissionFinish()
    {
        m_VigridMap = NULL;

#ifdef VIGRID_MAP_MINIMAP
        m_VigridMinimap = NULL;
#endif

        super.OnMissionFinish();
    }

    VigridMapClient GetVigridMapClient()
    {
        return m_VigridMap;
    }


    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!m_VigridMap)
            return;

        m_VigridMap.Update(timeslice);

#ifdef VIGRID_MAP_MINIMAP
        if (m_VigridMinimap)
            m_VigridMinimap.Update(timeslice);
#endif

        if (!GetUApi())
            return;
        if (m_UIManager.IsMenuOpen(MENU_CHAT_INPUT))
            return;

#ifdef VIGRID_MAP_MINIMAP
        HandleMinimapToggle();
#endif

        if (HandleMapClose())
            return;

        HandleMapOpen();
    }

    /**
     *  Close the map, on Esc or on a second press of the bind.
     *
     *  Esc has to be polled here rather than left to the engine. While ANY scripted menu is open,
     *  MissionGameplay.OnUpdate never reaches its `else if (UAUIMenu.LocalPress()) Pause()` branch
     *  (missiongameplay.c:691), so Esc over the map was a dead key rather than a competing one -
     *  nothing closed and nothing opened. Every vanilla menu answers this the same way, by reading
     *  UAUIBack itself; the vanilla map does it at mapmenu.c:296.
     *
     *  UAUIBack is read through GetInputByID, not by name: it is a vanilla input and its constant is
     *  always present, unlike this PBO's own binds.
     *
     *  @return true when the map was closed, so the caller stops before the open path re-opens it.
     */
    protected bool HandleMapClose()
    {
        if (!m_UIManager.IsMenuOpen(MENU_VIGRID_MAP))
            return false;

        if (GetUApi().GetInputByID(UAUIBack).LocalPress())
        {
            m_UIManager.CloseMenu(MENU_VIGRID_MAP);
            return true;
        }

        UAInput close_input = GetUApi().GetInputByName(VIGRID_MAP_INPUT_TOGGLE);
        if (!close_input)
            return false;
        if (!close_input.LocalPress())
            return false;

        m_UIManager.CloseMenu(MENU_VIGRID_MAP);
        return true;
    }

    /**
     *  Open the map, if there is nothing in the way.
     *
     *  The guard is "any menu at all", not a list of ids. Enumerating them drifted: only MENU_MAP and
     *  MENU_CHAT_INPUT were ever named, so the bind still fired over the in-game Esc menu, the
     *  inventory, the death screen and the spawn-selection screen - and passing GetMenu() as the
     *  parent made whichever of those it was the map's parent menu, so closing the map handed focus
     *  back to a screen the player had already left.
     *
     *  Note the bind itself is resolved by name, so nothing here depends on the UAVigridMapToggle
     *  constant generated from this PBO's Inputs.xml. Vanilla's own UAMapToggle is also M by default
     *  and fires at missiongameplay.c:599 when CfgGameplayHandler.GetMapIgnoreMapOwnership() is on;
     *  that is pre-existing and out of scope here, but it is where to look if M ever does two things.
     */
    protected void HandleMapOpen()
    {
        UAInput open_input = GetUApi().GetInputByName(VIGRID_MAP_INPUT_TOGGLE);
        if (!open_input)
            return;
        if (!open_input.LocalPress())
            return;

        if (m_UIManager.GetMenu())
            return;

        //--- Matches VigridMapMinimap.ShouldShow: a corpse has no business reading a map, and the
        //--- death screen is a menu anyway, so this only catches the gap between the two.
        PlayerBase local_player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!local_player)
            return;
        if (!local_player.IsAlive())
            return;

        //--- NULL parent, not GetMenu(): nothing can be open at this point, and adopting whatever was
        //--- there is what caused the focus hand-back described above.
        GetUIManager().EnterScriptedMenu(MENU_VIGRID_MAP, NULL);
    }

#ifdef VIGRID_MAP_MINIMAP
    /**
     *  Show or hide the minimap, and remember the choice.
     *
     *  Silently does nothing when the server has switched the minimap off entirely - the player's
     *  preference is still stored, so it comes back if they later join a server that allows it, but
     *  toggling something invisible would just look broken.
     */
    protected void HandleMinimapToggle()
    {
        UAInput minimap_input = GetUApi().GetInputByName(VIGRID_MAP_INPUT_MINIMAP);
        if (!minimap_input)
            return;
        if (!minimap_input.LocalPress())
            return;
        if (!VigridMapRPC.GetInstance().minimap_allowed)
            return;

        bool enabled = VigridMapPrefs.ToggleMinimap();
        VigridMapLog.Debug("Minimap toggled to " + enabled);
    }
#endif
}
#endif
