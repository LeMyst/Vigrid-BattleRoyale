#ifndef SERVER
/**
 *  Auto-Run - client lifecycle and the keybind.
 *
 *  A fifth `modded class MissionGameplay` alongside the Battle Royale, party, kill feed and map
 *  ones. They all chain through super, so application order does not matter - but every override
 *  here must keep calling super, or it silently cuts the others out of the chain.
 *
 *  There is no state in this file. It is glue: the feature itself lives in VigridAutoRunClient, one
 *  stage down, so that VigridAutoRunAPI can reach it from 4_World.
 */
modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        //--- The client singleton outlives a server change; anything still held belongs to the
        //--- previous session and would drive a player object that no longer exists.
        VigridAutoRunClient.GetInstance().Reset();

        VigridAutoRunLog.Debug("MissionGameplay::OnInit done");
    }

    override void OnMissionFinish()
    {
        VigridAutoRunClient.GetInstance().Reset();

        super.OnMissionFinish();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        //--- ABOVE the guards below, on purpose. This owns the per-frame re-assert and every cancel
        //--- condition, and a menu that leaves the player moving - the Vigrid map does exactly that,
        //--- keeping keyboard focus with the game - must not silently drop the hold.
        VigridAutoRunClient.GetInstance().Update(timeslice);

        if (!GetUApi())
            return;
        if (m_UIManager.IsMenuOpen(MENU_CHAT_INPUT))
            return;

        //--- Any menu at all, never a list of menu ids. The list is what drifted for Party and the
        //--- map: it named vanilla's MENU_MAP rather than the Vigrid one, so binds kept firing
        //--- underneath menus nobody had thought of.
        if (m_UIManager.GetMenu())
            return;

        HandleAutoRunToggle();
    }

    /**
     *  ⚠️ Undo vanilla's walk pin while auto-run is holding a speed.
     *
     *  The entire body of AddActiveInputRestriction for INVENTORY and MAP is
     *  UAWalkRunForced.ForceEnable(true) (missiongameplay.c:1001-1024) - "force walk on!". So opening
     *  the inventory mid-route would quietly halve auto-run's speed, and the matching
     *  RemoveActiveInputRestriction would hand it back on close, which reads as the feature randomly
     *  losing its mind rather than as a restriction doing its job.
     *
     *  ⚠️ IT IS NOT YET MEASURED whether the movement-speed override already beats this pin - if it
     *  does, this override is dead weight and should be DELETED rather than kept as cargo. The test
     *  is one build: comment out the body, auto-run at a sprint, open the inventory, and watch
     *  whether the character drops to a walk.
     */
    override void AddActiveInputRestriction(int restrictor)
    {
        super.AddActiveInputRestriction(restrictor);

        if (!VigridAutoRunClient.GetInstance().IsActive())
            return;
        if (!GetUApi())
            return;

        if (restrictor == EInputRestrictors.INVENTORY)
            GetUApi().GetInputByID(UAWalkRunForced).ForceEnable(false);
        if (restrictor == EInputRestrictors.MAP)
            GetUApi().GetInputByID(UAWalkRunForced).ForceEnable(false);
    }

    /**
     *  Read the bind.
     *
     *  By NAME, not through the generated UAVigridAutoRunToggle constant: that constant comes from
     *  this PBO's own Inputs.xml and may not exist at compile time. GetInputByName can answer NULL
     *  for a bind the engine never registered - a malformed Inputs.xml is dropped whole, silently -
     *  so it is null-checked rather than chained.
     */
    protected void HandleAutoRunToggle()
    {
        UAInput toggle = GetUApi().GetInputByName(VIGRID_AUTORUN_INPUT_TOGGLE);
        if (!toggle)
            return;
        if (!toggle.LocalPress())
            return;

        VigridAutoRunClient.GetInstance().Toggle();
    }
}
#endif
