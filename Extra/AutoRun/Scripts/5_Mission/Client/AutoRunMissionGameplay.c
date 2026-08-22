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

    /*
     *  ✅ THERE IS DELIBERATELY NO AddActiveInputRestriction OVERRIDE HERE, and it is worth saying so
     *  because the sources argue loudly that there should be.
     *
     *  Vanilla's entire body for the INVENTORY and MAP restrictors is
     *  UAWalkRunForced.ForceEnable(true) (missiongameplay.c:1001-1024) - "force walk on!" - which is
     *  what knocks a player down to a walk when they open their inventory. Read on its own that says
     *  opening the inventory mid-route must quietly halve auto-run's speed, and one build shipped
     *  with an override countering it for exactly that reason.
     *
     *  MEASURED 2026-08-16: it does not. OverrideMovementSpeed sits downstream of the walk pin, so an
     *  auto-running sprint stays a sprint with the inventory open and the counter-call changed
     *  nothing. It was deleted rather than kept "just in case" - a no-op guarded on IsActive() is
     *  indistinguishable from a working one, so it would have read as load-bearing forever.
     *
     *  The same reasoning covers any other EInputRestrictors member: they act on UApi inputs, and the
     *  override does not go through UApi.
     */

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
