#ifndef SERVER
/**
 *  Vigrid Map - client lifecycle and keybind.
 *
 *  A fourth `modded class MissionGameplay` alongside the Battle Royale, party and kill feed ones.
 *  They all chain through super, so application order does not matter - but every override here
 *  must keep calling super, or it silently cuts the others out of the chain.
 *
 *  Every minimap reference in this file is behind VIGRID_MAP_MINIMAP and every compass reference
 *  behind VIGRID_MAP_COMPASS, both declared in Extra/Map/config.cpp - comment either define out and
 *  that feature is gone from the build entirely, keybind handler included. See VigridMapMinimap.c
 *  and VigridMapCompass.c for what stays behind on purpose.
 */
modded class MissionGameplay
{
    protected ref VigridMapClient m_VigridMap;

    //! True while this frame's aim override is ours to release. See UpdateAimSuppression.
    protected bool m_VigridMapAimOverridden;

#ifdef VIGRID_MAP_MINIMAP
    protected ref VigridMapMinimap m_VigridMinimap;
#endif

#ifdef VIGRID_MAP_COMPASS
    protected ref VigridMapCompass m_VigridCompass;
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

#ifdef VIGRID_MAP_COMPASS
        //--- Same deferral as the minimap: nothing is built until its first Update.
        if (!m_VigridCompass)
            m_VigridCompass = new VigridMapCompass();
#endif

        VigridMapLog.Debug("MissionGameplay::OnInit done");
    }

    override void OnMissionFinish()
    {
        m_VigridMap = NULL;

#ifdef VIGRID_MAP_MINIMAP
        m_VigridMinimap = NULL;
#endif

#ifdef VIGRID_MAP_COMPASS
        m_VigridCompass = NULL;
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

        //--- Above the m_VigridMap guard on purpose: this owns a release edge, and a release that
        //--- only runs while the controller happens to exist is not a release.
        UpdateAimSuppression();

        if (!m_VigridMap)
            return;

        m_VigridMap.Update(timeslice);

#ifdef VIGRID_MAP_MINIMAP
        if (m_VigridMinimap)
            m_VigridMinimap.Update(timeslice);
#endif

#ifdef VIGRID_MAP_COMPASS
        //--- Every frame, with no timer in front of it, unlike the minimap. The strip slides under a
        //--- fixed cursor, which is exactly where a 10 Hz update reads as stutter.
        if (m_VigridCompass)
            m_VigridCompass.Update(timeslice);
#endif

        if (!GetUApi())
            return;
        if (m_UIManager.IsMenuOpen(MENU_CHAT_INPUT))
            return;

#ifdef VIGRID_MAP_MINIMAP
        HandleMinimapToggle();
#endif

#ifdef VIGRID_MAP_COMPASS
        HandleCompassToggle();
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

    /**
     *  Stop the mouse turning the camera while the map is open, without touching the keyboard.
     *
     *  The menu leaves the keyboard focus with the game so the player keeps running, and pays for it
     *  by leaving every non-keyboard input live. VigridMapMenu.SuppressGameplayInputs covers the
     *  buttons; the four aim axes are the half it cannot reach. Supress() is a PRESS concept and the
     *  aim axes are analog, and vanilla's own answer for this menu class - Input.DisableKey on mouse
     *  axes 0-5, missiongameplay.c:616 - is the low-level device, while both player cameras read the
     *  aim engine-side off the input controller (GetAimChange / GetAimDelta, dayzplayercamera_base.c).
     *
     *  "aiming" (bin/specific.xml:149) is exactly the four aim inputs and NOTHING else - in
     *  particular it does not include "movement", which is what makes it usable here where vanilla's
     *  own {"map"} is not. {"map"} is `<include name="menu" />`, and "menu" includes "movement", so
     *  it would take WASD straight back off and undo the whole point of this menu.
     *
     *  MEASURED, DO NOT RETRY: HumanInputController.OverrideAimChangeX/Y(ENABLED, 0) does NOT work.
     *  It is the obvious hook - the host mod already drives OverrideMovementSpeed / OverrideRaise /
     *  OverrideFreeLook from that same family in PlayerBase.DisableInput, and those do work. But the
     *  aim pair was tried here first and the camera kept turning, with the edge log confirming the
     *  calls were reaching the live controller on every open and close. Note the tell that was
     *  available beforehand and missed: OverrideRaise and Override3rdIsRightShoulder have real
     *  vanilla call sites, while OverrideAimChangeX/Y have NONE anywhere in P:\scripts - only the
     *  proto declaration at human.c:240.
     *
     *  THE COST IS REAL AND IS ACCEPTED: AddActiveInputExcludes and RemoveActiveInputExcludes both
     *  end in GetUApi().UpdateControls() ("call this on each change of exclusion"), which rebuilds
     *  the control state and drops the HELD state of every input including UATurbo. So opening or
     *  closing the map mid-sprint drops the player out of sprint until Shift is re-pressed. That is
     *  inherent to adding or removing a group at all, not to its membership - it is the same
     *  mechanism that walks a vanilla player when they open their inventory. It is preferred to a
     *  camera that spins while you read the map.
     *
     *  THIS LIVES IN THE MISSION UPDATE RATHER THAN IN THE MENU, and that is the point of the latch:
     *  a leaked exclude group would leave the player permanently unable to aim, and OnUpdate runs
     *  every frame whether or not the menu exists, so the remove edge cannot be missed the way an
     *  OnShow/OnHide pairing can when a menu is torn down without OnHide.
     */
    protected void UpdateAimSuppression()
    {
        bool want = m_UIManager.IsMenuOpen(MENU_VIGRID_MAP);

        //--- Edge-triggered, unlike the per-frame Supress work in the menu: each call rebuilds the
        //--- control state, so re-adding every frame would reset held inputs continuously.
        if (want == m_VigridMapAimOverridden)
            return;

        if (want)
        {
            AddActiveInputExcludes({"aiming"});
            m_VigridMapAimOverridden = true;
            VigridMapLog.Debug("Aim exclude ON - mouse will not turn the camera");
            return;
        }

        RemoveActiveInputExcludes({"aiming"});
        m_VigridMapAimOverridden = false;
        VigridMapLog.Debug("Aim exclude OFF - mouse-look restored");
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

#ifdef VIGRID_MAP_COMPASS
    /**
     *  Show or hide the compass strip, and remember the choice.
     *
     *  Same shape as HandleMinimapToggle, including the silent no-op when the server has switched the
     *  compass off entirely: the preference is still stored, so it comes back on a server that allows
     *  it, but toggling something invisible would just look broken.
     */
    protected void HandleCompassToggle()
    {
        UAInput compass_input = GetUApi().GetInputByName(VIGRID_MAP_INPUT_COMPASS);
        if (!compass_input)
            return;
        if (!compass_input.LocalPress())
            return;
        if (!VigridMapRPC.GetInstance().compass_allowed)
            return;

        bool compass_enabled = VigridMapPrefs.ToggleCompass();
        VigridMapLog.Debug("Compass toggled to " + compass_enabled);
    }
#endif
}
#endif
