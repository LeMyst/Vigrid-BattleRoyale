#ifndef SERVER
/**
 *  Vigrid Map - the fullscreen map.
 *
 *  Pan and zoom are the engine's: MapWidget implements both natively, so this menu adds clicks and
 *  drawing and otherwise stays out of the way. See VigridMapMenuHandler for why the wheel must not
 *  be overridden.
 *
 *  The map does not stop the player. UseKeyboard() is false, so only the mouse focus is taken and
 *  movement keys still reach the game - see the override for the two vanilla mechanisms that makes
 *  work, and for the one that must not be added back.
 *
 *  Drawing is gated rather than done every frame. There is no pan/zoom event to hook, so the
 *  world->screen transform is probed once per frame and compared with the previous probe; the
 *  canvases keep their draw list between frames, so when nothing has moved there is nothing to do.
 *  A watchdog repaint covers the case where a canvas loses its list for a reason we do not model.
 */
class VigridMapMenu extends UIScriptedMenu
{
    protected MapWidget m_MapWidget;
    protected CanvasWidget m_ZoneCanvas;
    protected CanvasWidget m_LineCanvas;
    protected CanvasWidget m_MarkerCanvas;
    protected CanvasWidget m_TeamCanvas;
    protected CanvasWidget m_AdminCanvas;

    //--- Name labels for the admin glyphs, BOUND from the layout rather than created. See the block
    //--- comment on AdminName0 in map_menu.layout for the measurement behind that.
    protected ref array<TextWidget> m_AdminNameTexts;
    protected int m_LastAdminSeq;
    protected int m_NextAdminFunnelMs;

    protected ref VigridMapMenuHandler m_Handler;
    protected IngameHud m_Hud;

    //--- The refusal message and its own timer. m_LastRejectSeq is re-baselined on every OnShow, so
    //--- opening the map cannot surface a refusal that happened while it was closed.
    protected TextWidget m_ToastText;
    protected int m_LastRejectSeq;
    protected int m_ToastUntilMs;

    //--- Transform probe results from the previous frame, used both as the projection basis and as
    //--- the change detector.
    protected vector m_PrevProbeOrigin;
    protected vector m_PrevProbeFar;
    protected bool m_RenderDirty;
    protected int m_LastRepaintMs;
    protected int m_LastMarkerSeq;
    protected int m_LastZoneSeq;
    protected int m_LastHotZoneSeq;

    //--- The team layer has its own clock, not its own dirty flag. Nothing raises an edge when a
    //--- teammate walks, so it repaints on a timer instead - see Update.
    protected int m_LastTeamRepaintMs;

    //--- The zoom the player was last using, carried across opens.
    //---
    //--- Static because it has to be: MapMissionBase.CreateScriptedMenu news a VigridMapMenu on every
    //--- open and the engine destroys it on close, so no member can survive the round trip.
    //---
    //--- Session-scoped on purpose. It is deliberately NOT a field in map_client.json: persisting it
    //--- would mean a synchronous JsonFileLoader write every time the map is shut, which in a match
    //--- is often, to save a preference that costs one wheel scroll to restate.
    //---
    //--- Zero means "never set", so there is no reliance on static-initialiser semantics - see
    //--- GetOpenScale, which folds that case together with an out-of-range value.
    protected static float s_LastScale;

    //--- The view this open is trying to establish, and whether it has. Until it has, the widget is
    //--- still showing the engine's default position and zoom, so nothing may be recorded from it -
    //--- the capture in Update is gated on m_Settled for exactly that reason.
    protected vector m_SettleTarget;
    protected bool m_Settled;
    protected int m_SettleFrames;

    void VigridMapMenu()
    {
        m_AdminNameTexts = new array<TextWidget>();
        m_LastAdminSeq = -1;
        m_NextAdminFunnelMs = 0;

        m_SettleTarget = vector.Zero;
        m_Settled = false;
        m_SettleFrames = 0;
        m_RenderDirty = true;
        m_LastRepaintMs = 0;
        m_LastMarkerSeq = -1;
        m_LastZoneSeq = -1;
        m_LastHotZoneSeq = -1;
        m_LastTeamRepaintMs = 0;
        m_LastRejectSeq = -1;
        m_ToastUntilMs = 0;
    }

    void ~VigridMapMenu()
    {
        if (m_Hud)
        {
            m_Hud.ShowHudUI(true);
            m_Hud.ShowQuickbarUI(true);
        }
    }

    override Widget Init()
    {
        //--- The scale is on this line because it is the one thing about an open that carries over
        //--- from the last one: "did it reopen where I left it" is otherwise only answerable by eye,
        //--- and a small zoom difference is exactly what an eye is worst at.
        VigridMapLog.Debug("VigridMapMenu::Init (opening at scale " + GetOpenScale() + ")");

        layoutRoot = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/map_menu.layout");
        if (!layoutRoot)
        {
            VigridMapLog.Error("Could not create map_menu.layout - map disabled");
            return NULL;
        }

        m_MapWidget = MapWidget.Cast(layoutRoot.FindAnyWidget("VigridMap"));

        if (!m_MapWidget)
        {
            VigridMapLog.Error("map_menu.layout has no VigridMap widget - map disabled");
            return layoutRoot;
        }

        m_ZoneCanvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("ZoneCanvas"));
        m_LineCanvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("LineCanvas"));

        //--- Markers are drawn onto a canvas rather than built as widgets. A canvas declared inside
        //--- the MapWidget is the only overlay mechanism proven to render in this repo - the spawn
        //--- selection screen's heat map uses exactly this - whereas widgets created from script
        //--- over the map are positioned correctly and never appear.
        m_MarkerCanvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("MarkerCanvas"));
        if (!m_MarkerCanvas)
            VigridMapLog.Error("map_menu.layout has no MarkerCanvas - markers disabled");

        //--- Highest priority of the four, so a live player draws over a placed pin. Failing to find
        //--- it is logged rather than fatal: teammates are worth losing before the whole map is.
        m_TeamCanvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("TeamCanvas"));
        if (!m_TeamCanvas)
            VigridMapLog.Error("map_menu.layout has no TeamCanvas - teammates and pings disabled");

        //--- Above TeamCanvas, so a live player draws over every placed pin. Logged rather than
        //--- fatal, like the two above: losing this layer is worth far less than losing the map.
        m_AdminCanvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("AdminCanvas"));
        if (!m_AdminCanvas)
            VigridMapLog.Error("map_menu.layout has no AdminCanvas - the host player layer is disabled");

        //--- Bind the declared label pool. Found on m_MapWidget, like the canvases and for exactly the
        //--- same reason: a DECLARED child of the MapWidget renders, and a script-created one does
        //--- not - measured, see map_menu.layout. The loop stops at the first gap, so the pool size
        //--- lives in the layout alone and this does not have to be told how big it is.
        int name_slot = 0;
        while (name_slot < VIGRID_MAP_ADMIN_NAME_MAX)
        {
            TextWidget label = TextWidget.Cast(m_MapWidget.FindAnyWidget("AdminName" + name_slot.ToString()));
            if (!label)
                break;

            m_AdminNameTexts.Insert(label);
            name_slot++;
        }

        if (m_AdminNameTexts.Count() == 0)
            VigridMapLog.Error("map_menu.layout declares no AdminName<n> labels - host player names disabled");
        else
            VigridMapLog.Debug("Bound " + m_AdminNameTexts.Count().ToString() + " admin name label(s)");

        //--- Found on layoutRoot, not on m_MapWidget: the toast sits in the strip above the map, so
        //--- it is a sibling of the MapWidget rather than a child of it.
        m_ToastText = TextWidget.Cast(layoutRoot.FindAnyWidget("MarkerToast"));
        if (m_ToastText)
            m_ToastText.Show(false);

        m_Handler = new VigridMapMenuHandler(m_MapWidget);

        m_Hud = IngameHud.Cast(GetGame().GetMission().GetHud());
        if (m_Hud)
        {
            m_Hud.ShowHudUI(false);
            m_Hud.ShowQuickbarUI(false);
        }

        CenterOnPlayer();

        layoutRoot.Update();
        return layoutRoot;
    }

    /**
     *  Centre the map on the player.
     *
     *  SetMapPos and SetScale are both ignored until the widget has been through a layout pass, so
     *  the pair issued here lands on a widget that has no size yet and is silently dropped. Until a
     *  later pair takes, the map draws at the engine's own default position and zoom - which is the
     *  visible artefact this settles: the map appears off your position, then jumps.
     *
     *  So it is issued three ways, cheapest first: here, in case the widget is somehow ready; then
     *  once per frame from SettleView until a readback proves it took; and finally from DelayedCenter
     *  as the backstop that was the only mechanism before. The per-frame retry is what removes the
     *  artefact - the fixed delay is ~6 frames at 60 fps, whereas the retry lands on the first frame
     *  the widget is actually laid out.
     *
     *  The POSITION is recentred on every open and the ZOOM is not, which is the asymmetry to keep:
     *  the reason to open a map is to see where you are, but the zoom the player picked is a
     *  preference they would otherwise have to restate on every open.
     */
    protected void CenterOnPlayer()
    {
        vector target = ResolveSelfPos();

        if (target == vector.Zero)
            target = GetGame().GetCurrentCameraPosition();

        m_SettleTarget = target;

        m_MapWidget.SetMapPos(target);
        m_MapWidget.SetScale(GetOpenScale());

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DelayedCenter, VIGRID_MAP_CENTER_DELAY_MS, false, target);
    }

    /**
     *  Where "you" are, for both the self glyph and the recentre-on-open.
     *
     *  The host mod's override wins when one is in force. That is the whole of #277: while an admin
     *  spectates, their body is parked somewhere as a network anchor and the camera is elsewhere, so
     *  GetGame().GetPlayer() answers the anchor and the glyph landed on it. The minimap never had the
     *  bug because it re-centres on the camera every tick and never asks the player.
     *
     *  ⚠ NOT SIMPLY "USE THE CAMERA WHEN ONE DIFFERS". In ordinary third-person play the camera sits
     *  several metres behind the character, and the map is panned by the player rather than pinned to
     *  the camera - so reading the camera unconditionally would put every player's glyph behind
     *  themselves. Only an explicit assertion from the host can tell the two situations apart, which
     *  is why this is a pushed override and not something inferred here.
     *
     *  Returns vector.Zero when there is neither an override nor a player; both callers handle it.
     */
    protected vector ResolveSelfPos()
    {
        if (VigridMapAPI.HasSelfPositionOverride())
            return VigridMapAPI.GetSelfPositionOverride();

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
            return player.GetPosition();

        return vector.Zero;
    }

    /**
     *  Re-issue the view every frame until it demonstrably takes.
     *
     *  Two tests, because neither alone is trustworthy. A widget that has not been laid out reports
     *  a zero screen size, which is the direct question - "has layout run" - but says nothing about
     *  whether the map transform behind it is ready. So the position is also read back: a SetMapPos
     *  that was dropped leaves GetMapPos answering something else.
     *
     *  Capped rather than retried for ever. Past the cap the view is accepted as-is and DelayedCenter
     *  is still coming, so the worst case is exactly the behaviour that shipped before this method
     *  existed - never a map left stuck at the engine default.
     */
    protected void SettleView()
    {
        m_SettleFrames++;

        float widget_w;
        float widget_h;
        m_MapWidget.GetScreenSize(widget_w, widget_h);

        bool laid_out = (widget_w > 0) && (widget_h > 0);

        if (laid_out)
        {
            m_MapWidget.SetMapPos(m_SettleTarget);
            m_MapWidget.SetScale(GetOpenScale());
        }

        //--- ⚠️ MEASURED 2026-08-16: THIS TEST NEVER PASSES, and the log proves it - every open ends
        //--- on "View settled by the delayed backstop, not SettleView". The artefact is nonetheless
        //--- gone, so what fixed it is one of the two OTHER things this method brought with it: the
        //--- per-frame re-issue of SetMapPos/SetScale above, or Update's early return holding
        //--- ClampZoom and the canvases off until the view exists.
        //---
        //--- The likely reason it never passes, UNVERIFIED: m_SettleTarget is a PLAYER position,
        //--- whose Y is terrain height - ~150 m on ChernarusPlus - while GetMapPos almost certainly
        //--- answers at Y 0. A 3D distance is then never below any sane epsilon. Zeroing Y on both
        //--- sides before comparing is the one-line change to try, and the acceptance test is this
        //--- method's own log line reporting a frame count instead of the backstop message.
        //---
        //--- Left as-is deliberately: the behaviour above was tested and confirmed good, and the
        //--- latch only decides when ClampZoom and the canvases resume, so getting it wrong EARLY is
        //--- the one way to bring the artefact back. Do not "fix" it without re-running that test.
        vector actual = m_MapWidget.GetMapPos();
        bool took = laid_out && (vector.Distance(actual, m_SettleTarget) <= VIGRID_MAP_SETTLE_EPSILON_M);

        if (!took && m_SettleFrames < VIGRID_MAP_SETTLE_MAX_FRAMES)
            return;

        m_Settled = true;
        m_RenderDirty = true;

        VigridMapLog.Debug("View settled after " + m_SettleFrames + " frame(s), took=" + took + " size=" + widget_w + "x" + widget_h);
    }

    void DelayedCenter(vector pos)
    {
        if (!m_MapWidget)
            return;

        //--- The backstop. Harmless when SettleView already won - it re-asserts the same pair - and
        //--- the whole mechanism when it did not.
        m_MapWidget.SetMapPos(pos);
        m_MapWidget.SetScale(GetOpenScale());
        m_RenderDirty = true;

        if (!m_Settled)
            VigridMapLog.Debug("View settled by the delayed backstop, not SettleView");

        //--- From this point the widget holds the scale we asked for, so Update may start recording
        //--- what the player does to it.
        m_Settled = true;
    }

    /**
     *  The scale to open at: the one the player left, or the default when there is nothing to restore.
     *
     *  Clamped on READ rather than left to ClampZoom. That would also correct it, one frame later,
     *  at the cost of a frame drawn out of range and the spurious repaint its m_RenderDirty raises.
     *  Doing it here also means a change to either limit cannot strand a value stored under the old
     *  ones - it simply falls back to the default.
     */
    protected static float GetOpenScale()
    {
        if (s_LastScale < VIGRID_MAP_MIN_SCALE)
            return VIGRID_MAP_DEF_SCALE;

        if (s_LastScale > VIGRID_MAP_MAX_SCALE)
            return VIGRID_MAP_DEF_SCALE;

        return s_LastScale;
    }

    /**
     *  The menu takes the mouse and leaves the keyboard to the game, so the player keeps running
     *  while reading the map.
     *
     *  This is the supported hook, not a trick: UIScriptedMenu.LockControls only grabs the keyboard
     *  focus when UseKeyboard() is true (uiscriptedmenu.c:93), and MissionGameplay carries a branch
     *  written for exactly this combination - `!menu.UseKeyboard() && menu.UseMouse()` disables the
     *  five mouse buttons and six mouse axes as GAME inputs (missiongameplay.c:616), so panning and
     *  clicking the map cannot fire the weapon underneath it.
     *
     *  What was NOT used, and must not be added back: AddActiveInputRestriction(EInputRestrictors.MAP).
     *  It looks like the right hook - it is vanilla's "this player has a map open" restriction - but
     *  its whole body is UAWalkRunForced.ForceEnable(true) (missiongameplay.c:1017), which pins the
     *  player to walking speed. That is the opposite of the point.
     */
    override bool UseKeyboard()
    {
        return false;
    }

    override void OnShow()
    {
        super.OnShow();

        //--- No SetBlurMenu here. The menu blur assumes a player standing still behind it; now that
        //--- they can run, blurring the world they are running through is actively unhelpful.
        //---
        //--- No ChangeGameFocus either. super.OnShow() -> LockControls already takes the focus this
        //--- menu is entitled to, per device. The bare ChangeGameFocus(1) that used to sit here took
        //--- a second, all-device focus on top of it, and OnHide paid for it with a global
        //--- ResetGameFocus() that would have zeroed a parent menu's focus as well as ours.
        SetFocus(layoutRoot);

        //--- Nothing on the canvases survives a hide/show cycle, so force a repaint rather than
        //--- waiting for the transform to move.
        m_RenderDirty = true;
        m_LastMarkerSeq = -1;
        m_LastZoneSeq = -1;
        m_LastHotZoneSeq = -1;

        //--- Baselined rather than reset to -1, unlike the three above. A refusal is an event, not a
        //--- state: catching up on one the player could not have caused - the map was shut - would
        //--- pop a message about a click they made a minute ago.
        m_LastRejectSeq = VigridMapRPC.GetInstance().rejected_seq;
        m_ToastUntilMs = 0;
        if (m_ToastText)
            m_ToastText.Show(false);

        //--- The team layer needs no equivalent reset: it repaints on its own clock within 100 ms
        //--- of the map appearing.
    }

    override void OnHide()
    {
        //--- super.OnHide() -> UnlockControls undoes exactly what LockControls took, per device.
        //--- See OnShow for why the blur and the extra focus grab are gone.
        super.OnHide();
    }

    /**
     *  Keep gameplay actions from firing underneath the map.
     *
     *  This is the price of leaving the keyboard focus with the game so the player can keep running:
     *  every action stays live, so a left click to place a marker also swung the melee weapon.
     *  Vanilla's mouse-key disable for !UseKeyboard menus does not cover it - that calls
     *  Input.DisableKey, which is the low-level device, while fire, melee and user actions are read
     *  engine-side from the UApi binding. (UAFire appears in exactly one script in the whole game.)
     *
     *  AN EXCLUDE GROUP IS THE OBVIOUS MECHANISM AND IT IS THE WRONG ONE HERE. It works, and UApi's
     *  own header recommends it over Lock()/Unlock() - but AddActiveInputExcludes and
     *  RemoveActiveInputExcludes both end in GetUApi().UpdateControls(), documented as "call this on
     *  each change of exclusion". That rebuilds the whole control state and drops the HELD state of
     *  every input, including UATurbo, so closing the map mid-sprint dumped the player out of sprint
     *  until Shift was released and re-pressed. It is the same mechanism that knocks vanilla players
     *  down to a walk when they open the inventory, and no amount of choosing which inputs go in the
     *  group avoids it - the reset comes from the group being added or removed at all.
     *
     *  Supress() is per-input and touches nothing global, so there is nothing to undo on close and
     *  no rebuild to trigger. It is also forward-looking - "supress press event for next frame" -
     *  which is what makes it safe to call from a menu Update whose ordering against
     *  MissionGameplay.OnUpdate is not defined: by the time any click arrives the map has been open
     *  for many frames. The one frame it cannot cover is the first, and the key that opens the map
     *  is M.
     *
     *  Note the second half of that doc line: "while not pressed ATM - otherwise until release". A
     *  suppressed input the player is already holding stays dead until they let go, which is exactly
     *  what should happen to a mouse button held across a map close, and exactly why nothing here
     *  may ever be pointed at a movement key.
     */
    protected void SuppressGameplayInputs()
    {
        if (!GetUApi())
            return;

        //--- Attack, fire and the use-action, which is the reported bug.
        GetUApi().GetInputByID(UAFire).Supress();
        GetUApi().GetInputByID(UAHeavyMeleeAttack).Supress();
        GetUApi().GetInputByID(UADefaultAction).Supress();
        GetUApi().GetInputByID(UAAction).Supress();

        //--- Hands and inventory: nothing should change what is held, or open a second screen.
        GetUApi().GetInputByID(UAGear).Supress();
        GetUApi().GetInputByID(UAToggleWeapons).Supress();
        GetUApi().GetInputByID(UAReloadMagazine).Supress();
        GetUApi().GetInputByID(UADropitem).Supress();
        GetUApi().GetInputByID(UAThrowitem).Supress();
        GetUApi().GetInputByID(UAUIQuickbarToggle).Supress();
        GetUApi().GetInputByID(UAUIGesturesOpen).Supress();

        //--- Aim and view, which are meaningless while staring at a map and would fight the cursor.
        GetUApi().GetInputByID(UATempRaiseWeapon).Supress();
        GetUApi().GetInputByID(UAZoomIn).Supress();
        GetUApi().GetInputByID(UAPersonView).Supress();
        GetUApi().GetInputByID(UALookAround).Supress();

        SuppressQuickbarSlots();
    }

    //! The ten quickbar hotkeys, which would otherwise swap what is in the player's hands.
    protected void SuppressQuickbarSlots()
    {
        GetUApi().GetInputByID(UAItem0).Supress();
        GetUApi().GetInputByID(UAItem1).Supress();
        GetUApi().GetInputByID(UAItem2).Supress();
        GetUApi().GetInputByID(UAItem3).Supress();
        GetUApi().GetInputByID(UAItem4).Supress();
        GetUApi().GetInputByID(UAItem5).Supress();
        GetUApi().GetInputByID(UAItem6).Supress();
        GetUApi().GetInputByID(UAItem7).Supress();
        GetUApi().GetInputByID(UAItem8).Supress();
        GetUApi().GetInputByID(UAItem9).Supress();
    }

    /**
     *  A left click on the map places the player's marker, replacing whatever they had.
     *
     *  `screen_pos` is raw screen pixels, which is what ScreenToMap consumes. Its returned y is not
     *  an elevation and is meaningless - VigridMapClient.PlaceMarker flattens it.
     */
    void OnMapLeftClick(vector screen_pos)
    {
        VigridMapLog.Debug("Left click at screen " + screen_pos);

        if (!m_MapWidget)
            return;

        VigridMapClient client = GetVigridMapClient();
        if (!client)
        {
            VigridMapLog.Warn("Left click with no VigridMapClient - marker dropped");
            return;
        }

        vector world_pos = m_MapWidget.ScreenToMap(screen_pos);

        client.PlaceMarker(world_pos);
        m_RenderDirty = true;
    }

    //! A right click clears your own marker. No hit test: you only ever have one.
    void OnMapRightClick()
    {
        VigridMapClient client = GetVigridMapClient();
        if (!client)
            return;

        client.ClearMarker();
        m_RenderDirty = true;
    }

    protected VigridMapClient GetVigridMapClient()
    {
        MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
        if (!mission)
            return NULL;

        return mission.GetVigridMapClient();
    }

    /**
     *  Show why the server refused a marker, and take it away again a few seconds later.
     *
     *  Watches VigridMapRPC's refusal counter directly rather than going through VigridMapClient.
     *  Both watch it, independently, each keeping its own last-seen value - the client stops DRAWING
     *  the prediction, this says why. Routing one through the other would have made the message
     *  depend on a prediction still being outstanding, which it need not be.
     */
    protected void UpdateToast()
    {
        if (!m_ToastText)
            return;

        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (rpc.rejected_seq != m_LastRejectSeq)
        {
            m_LastRejectSeq = rpc.rejected_seq;
            ShowToast(rpc.rejected_key);
        }

        if (m_ToastUntilMs == 0)
            return;
        if (GetGame().GetTime() < m_ToastUntilMs)
            return;

        m_ToastUntilMs = 0;
        m_ToastText.Show(false);
    }

    /**
     *  An EMPTY key is a refusal with nothing worth saying - the place cooldown, or a click outside
     *  the world. It is still a refusal and still cancels the prediction over in VigridMapClient; it
     *  just does not interrupt the player to report a click that will feel like it simply missed.
     *
     *  The key arrives bare and is localised here, with the '#' the widget needs. The server cannot
     *  do it: it has no idea what language this client is running.
     */
    protected void ShowToast(string key)
    {
        if (key == "")
            return;

        m_ToastText.SetText("#" + key);
        m_ToastText.Show(true);
        m_ToastUntilMs = GetGame().GetTime() + VIGRID_MAP_TOAST_MS;

        VigridMapLog.Debug("Toast: " + key);
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);

        //--- Before the early return: a frame that finds no map widget is still a frame in which the
        //--- player must not swing at whatever is in front of them.
        SuppressGameplayInputs();

        //--- Also before it, and for the same shape of reason: a map that failed to find its widget
        //--- is exactly the situation in which an explanation is worth the most.
        UpdateToast();

        if (!m_MapWidget)
            return;

        //--- Above ClampZoom, and it must stay there: until the view has settled the widget holds the
        //--- engine's default scale, and clamping that would write a value we never asked for into a
        //--- widget that is about to be told what to show anyway.
        if (!m_Settled)
        {
            SettleView();
            return;
        }

        ClampZoom();

        //--- Recorded from the live widget every frame rather than on close. There is no zoom event
        //--- to hook - the same reason ClampZoom above reads it per frame - and neither teardown hook
        //--- is a safe substitute: OnHide is not guaranteed to pair with OnShow on every path, and
        //--- the destructor runs while the widget tree is already going away. Reading a float per
        //--- frame is cheaper than either risk, and ClampZoom has already put it in range.
        s_LastScale = m_MapWidget.GetScale();

        vector probe_origin;
        vector probe_far;
        float px_per_m_x;
        float px_per_m_z;
        VigridMapRender.ProbeTransform(m_MapWidget, probe_origin, probe_far, px_per_m_x, px_per_m_z);

        bool transform_moved = false;
        if (probe_origin != m_PrevProbeOrigin)
            transform_moved = true;
        if (probe_far != m_PrevProbeFar)
            transform_moved = true;

        VigridMapClient client = GetVigridMapClient();
        if (client && client.GetMarkerSeq() != m_LastMarkerSeq)
        {
            m_RenderDirty = true;
            m_LastMarkerSeq = client.GetMarkerSeq();
        }

        if (VigridMapAPI.GetZoneSeq() != m_LastZoneSeq)
        {
            m_RenderDirty = true;
            m_LastZoneSeq = VigridMapAPI.GetZoneSeq();
        }

        //--- Hot zones share the ZoneCanvas but not the sequence: they arrive once, from a different
        //--- RPC, and often while the map is already open. Without their own edge the circles would
        //--- not appear until the watchdog fired up to a second later.
        if (VigridMapAPI.GetHotZoneSeq() != m_LastHotZoneSeq)
        {
            m_RenderDirty = true;
            m_LastHotZoneSeq = VigridMapAPI.GetHotZoneSeq();
        }

        //--- The admin layer has an edge AND a clock, unlike every other layer here, because it has
        //--- both kinds of change. The sequence catches the set growing or shrinking - a player dies,
        //--- an admin starts watching - and would be enough on its own if positions were static. They
        //--- are not: the host re-pushes them twice a second, which the sequence does see, but a
        //--- 2 Hz repaint of moving glyphs visibly steps. So it rides the team clock as well.
        if (VigridMapAPI.GetAdminPlayerSeq() != m_LastAdminSeq)
        {
            m_RenderDirty = true;
            m_LastAdminSeq = VigridMapAPI.GetAdminPlayerSeq();
        }

        bool watchdog_due = (GetGame().GetTime() - m_LastRepaintMs) >= VIGRID_MAP_REPAINT_WATCHDOG_MS;

        //--- Two gates over one probe, because the two kinds of drawing change for different
        //--- reasons. Zones and markers move on an edge - a new circle, a placed pin, a pan - and
        //--- are edge-triggered accordingly. Teammates have no edge at all: Party's roster sequence
        //--- moves when the party changes shape, never when somebody walks, and positions are
        //--- interpolated continuously between server pushes, so even a per-push trigger would step
        //--- twice a second instead of gliding. That layer runs on a clock instead.
        //---
        //--- Repainting everything at that clock rate was the obvious alternative and is wrong: it
        //--- would re-emit up to 200 dash segments and two rings of up to 180 chords ten times a
        //--- second, for data that changes once a round.
        bool static_due = m_RenderDirty || transform_moved || watchdog_due;
        bool team_due = (GetGame().GetTime() - m_LastTeamRepaintMs) >= VIGRID_MAP_TEAM_TICK_MS;

        //--- Recorded unconditionally, and NOT inside the branch below.
        //---
        //--- They used to be set after an early return, which was correct while one branch was the
        //--- probe's only consumer. It is not any more: a frame that repaints only the team layer
        //--- would leave the previous probe stale, transform_moved would latch true for ever, and
        //--- the static layers would silently repaint at frame rate. The symptom is invisible on
        //--- screen and shows up only as sixty "Marker drawn at canvas" traces a second.
        m_PrevProbeOrigin = probe_origin;
        m_PrevProbeFar = probe_far;

        if (static_due)
        {
            m_RenderDirty = false;
            m_LastRepaintMs = GetGame().GetTime();

            RenderZones();
            RenderMarkers(client);
        }

        //--- Also on static_due: a pan moves every glyph, and waiting up to a tick to catch up would
        //--- leave teammates visibly lagging the map under the cursor.
        if (static_due || team_due)
        {
            m_LastTeamRepaintMs = GetGame().GetTime();
            RenderTeam();
            RenderAdminPlayers();
        }
    }

    /**
     *  Hold the zoom inside a useful range.
     *
     *  MapWidget handles the wheel itself and raises no event for it, so there is nothing to
     *  intercept - overriding OnMouseWheel would take the whole zoom away rather than bound it.
     *  Reading the scale back each frame and pushing it in is the only lever available.
     *
     *  Writing it back unconditionally would fight the engine and stop the wheel feeling smooth,
     *  so it is only touched when actually out of range.
     */
    protected void ClampZoom()
    {
        float scale = m_MapWidget.GetScale();

        if (scale < VIGRID_MAP_MIN_SCALE)
        {
            m_MapWidget.SetScale(VIGRID_MAP_MIN_SCALE);
            m_RenderDirty = true;
            return;
        }

        if (scale > VIGRID_MAP_MAX_SCALE)
        {
            m_MapWidget.SetScale(VIGRID_MAP_MAX_SCALE);
            m_RenderDirty = true;
        }
    }

    /**
     *  The two play areas, their centres, and a dashed line to the one the player has to be inside.
     *
     *  Both canvases are cleared first: a CanvasWidget keeps its draw list between frames, so
     *  without the Clear every repaint stacks another ring on top of the last one and a zone that
     *  shrank leaves its old outline behind for ever. The clears also have to happen before every
     *  early return below, which is why they are not folded into the two drawing branches.
     *
     *  The line targets the NEXT circle while there is one and the current circle otherwise, which
     *  is always the circle that has to be reached: between shrink phases, and for the whole of the
     *  last round, there is no next one and the current ring is what damage is measured against.
     */
    protected void RenderZones()
    {
        if (m_ZoneCanvas)
            m_ZoneCanvas.Clear();
        if (m_LineCanvas)
            m_LineCanvas.Clear();

        if (!m_ZoneCanvas)
            return;

        bool has_current = VigridMapAPI.HasCurrentZone();
        bool has_next = VigridMapAPI.HasNextZone();

        vector current_center = VigridMapAPI.GetCurrentCenter();
        float current_radius = VigridMapAPI.GetCurrentRadius();
        vector next_center = VigridMapAPI.GetNextCenter();
        float next_radius = VigridMapAPI.GetNextRadius();

        //--- Hot zones first, so the play-area rings draw OVER them. A hot zone is decoration; the
        //--- circle a player has to run to is not, and must never be the one that gets obscured.
        RenderHotZones();

        if (has_current)
        {
            VigridMapRender.WorldRenderOval(m_ZoneCanvas, m_MapWidget, current_center, current_radius, current_radius, VIGRID_MAP_COLOR_CURRENT_ZONE, VIGRID_MAP_ZONE_LINE_WIDTH);
            VigridMapRender.WorldRenderDot(m_ZoneCanvas, m_MapWidget, current_center, VIGRID_MAP_CENTER_DOT_PX, VIGRID_MAP_COLOR_CURRENT_ZONE);
        }

        if (has_next)
        {
            VigridMapRender.WorldRenderOval(m_ZoneCanvas, m_MapWidget, next_center, next_radius, next_radius, VIGRID_MAP_COLOR_NEXT_ZONE, VIGRID_MAP_ZONE_LINE_WIDTH);
            VigridMapRender.WorldRenderDot(m_ZoneCanvas, m_MapWidget, next_center, VIGRID_MAP_CENTER_DOT_PX, VIGRID_MAP_COLOR_NEXT_ZONE);
        }

        //--- The circle to point at, and only one of them is ever pointed at: reaching the next one
        //--- implies reaching the current one, since the circles are nested.
        if (has_next)
        {
            RenderZoneLine(next_center, next_radius);
            return;
        }

        if (has_current)
            RenderZoneLine(current_center, current_radius);
    }

    /**
     *  The host's hot zones - static circles marking regions of interest.
     *
     *  No Clear() of its own: this draws onto the ZoneCanvas that RenderZones has just cleared, and
     *  is only ever called from there. Kept as its own method so the ordering above stays readable,
     *  not because it is independently callable.
     *
     *  An entry with no radius is skipped rather than dropped on arrival, so the indices a host sees
     *  and the ones this addon draws stay the same - which is what makes a log line naming "hot zone
     *  3" mean the same thing on both sides.
     */
    protected void RenderHotZones()
    {
        int count = VigridMapAPI.GetHotZoneCount();

        for (int i = 0; i < count; i++)
        {
            float radius = VigridMapAPI.GetHotZoneRadius(i);
            if (radius <= 0)
                continue;

            vector center = VigridMapAPI.GetHotZoneCenter(i);
            if (center == vector.Zero)
                continue;

            VigridMapRender.WorldRenderOval(m_ZoneCanvas, m_MapWidget, center, radius, radius, VIGRID_MAP_COLOR_HOT_ZONE, VIGRID_MAP_ZONE_LINE_WIDTH, VIGRID_MAP_COLOR_HOT_ZONE_FILL);
        }
    }

    /**
     *  A dashed line from the player to the near edge of one circle, drawn only from outside it.
     *
     *  Both halves of that are the point. A player already inside the circle has nothing to do about
     *  it, so a line pointing at its centre is noise laid over the part of the map they are actually
     *  reading; and a line that runs to the centre crosses the ring it is telling them to reach, so
     *  the one number they want - how far is left - is mixed in with the radius. Stopping at the
     *  edge makes the drawn length the remaining distance and nothing else.
     */
    protected void RenderZoneLine(vector center, float radius)
    {
        if (!m_LineCanvas)
            return;

        //--- The line is drawn from the player, not the camera: in third person the camera sits a
        //--- couple of metres back, which would make the line start off the player icon.
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        vector player_pos = player.GetPosition();

        //--- Flat distance, on XZ. A zone is a disc rather than a sphere, so a player standing on a
        //--- hill inside the circle must not read as further out than one standing in a field.
        float dx = center[0] - player_pos[0];
        float dz = center[2] - player_pos[2];
        float distance = Math.Sqrt((dx * dx) + (dz * dz));

        //--- Inside means no line. It also guards the division below: a live circle always has a
        //--- radius above zero - VigridMapAPI rejects anything else - so passing this test means
        //--- distance is above zero too.
        if (distance <= radius)
            return;

        float travel = (distance - radius) / distance;
        vector edge = Vector(player_pos[0] + (dx * travel), 0, player_pos[2] + (dz * travel));

        float canvas_w;
        float canvas_h;
        m_LineCanvas.GetScreenSize(canvas_w, canvas_h);

        VigridMapRender.WorldRenderDashedLine(m_LineCanvas, m_MapWidget, player_pos, edge, canvas_w, canvas_h, VIGRID_MAP_COLOR_ZONE_LINE);
    }

    /**
     *  Every marker we may see, drawn onto the marker canvas.
     *
     *  Drawn rather than built out of widgets. A CanvasWidget declared inside the MapWidget is the
     *  only overlay this engine reliably renders here - the spawn selection heat map proves it -
     *  while widgets created from script over the map get correct positions and never appear. Since
     *  markers carry no text today, nothing is lost by drawing them.
     */
    protected void RenderMarkers(VigridMapClient client)
    {
        if (!m_MarkerCanvas)
            return;

        //--- A canvas keeps its draw list between frames, so without this every repaint stacks
        //--- another copy and a moved marker leaves its old one behind.
        m_MarkerCanvas.Clear();

        if (!client)
            return;

        int count = client.GetDrawCount();
        for (int i = 0; i < count; i++)
        {
            //--- A teammate's marker takes their party slot colour, so it matches the triangle
            //--- showing where they are. Keyed off the slot stored with the marker, not off the live
            //--- roster - see VigridMapClient.GetVisibleSlot for why that distinction matters.
            int color = VigridMapTeam.GetColorForSlot(client.GetDrawSlot(i), 1.0);
            if (client.IsDrawOwn(i))
                color = VIGRID_MAP_COLOR_OWN_MARKER;

            DrawMarker(client.GetDrawPos(i), color);
        }
    }

    /**
     *  One marker: a ring with a cross through it, so it reads as deliberate at any zoom and is not
     *  mistaken for a zone circle.
     *
     *  Sized in pixels rather than metres - a marker is an annotation, not a thing with an extent,
     *  so it should stay the same size as the map zooms.
     */
    protected void DrawMarker(vector world_pos, int color)
    {
        float cx;
        float cy;
        VigridMapRender.WorldToCanvas(m_MapWidget, world_pos, cx, cy);

        float r = VIGRID_MAP_MARKER_PX * 0.5;

        m_MarkerCanvas.DrawLine(cx - r, cy - r, cx + r, cy + r, VIGRID_MAP_MARKER_LINE_WIDTH, color);
        m_MarkerCanvas.DrawLine(cx - r, cy + r, cx + r, cy - r, VIGRID_MAP_MARKER_LINE_WIDTH, color);

        //--- Ring around the cross. Drawn in screen space, so it needs no map projection.
        int segments = 12;
        float step = 360.0 / segments;
        for (int i = 0; i < segments; i++)
        {
            float a1 = i * step * Math.DEG2RAD;
            float a2 = (i + 1) * step * Math.DEG2RAD;

            m_MarkerCanvas.DrawLine(cx + (r * Math.Cos(a1)), cy + (r * Math.Sin(a1)), cx + (r * Math.Cos(a2)), cy + (r * Math.Sin(a2)), VIGRID_MAP_MARKER_LINE_WIDTH, color);
        }

        VigridMapLog.Trace("Marker drawn at canvas " + cx + "," + cy);
    }

    /**
     *  The live layer: where you are, where your team is, and what they have pinged.
     *
     *  Split into three helpers rather than written as one loop-and-a-loop, because EnfusionScript
     *  allows one declaration per name per method scope - a single method would need `pos` and
     *  `ping_pos`, `color` and `ping_color`, names invented for the compiler rather than the reader.
     */
    protected void RenderTeam()
    {
        if (!m_TeamCanvas)
            return;

        //--- Cleared before anything decides not to draw, and in particular before the availability
        //--- check below. A canvas keeps its draw list, so an early return that skipped this would
        //--- burn the last frame of teammates into the map permanently the moment you left the party.
        m_TeamCanvas.Clear();

        //--- Deliberately outside the Party gate: "you are here" is worth having on a map whether or
        //--- not the party addon is installed at all.
        RenderSelfGlyph();

        if (!VigridMapTeam.IsAvailable())
            return;

        RenderTeammates();
        RenderTeamPings();
    }

    /**
     *  The local player: where you are, and which way you are facing.
     *
     *  Position from ResolveSelfPos - i.e. the player's own body, and never through the team API. The
     *  client entity list does not contain the local player, so asking Party for your own slot returns
     *  the interpolated server push and your glyph would trail you by up to half a second.
     *
     *  POSITION FROM THE BODY, ANGLE FROM THE CAMERA - and the split is deliberate. The minimap
     *  passes the camera position for both only because it re-centres its map on the camera every
     *  tick; here the map is panned by the player, so the glyph has to sit on the character, who in
     *  third person is several metres in front of the camera. The angle is the camera's for the
     *  reason written up at length in VigridMapMinimap.DrawHeadingArrow: body yaw is the animated
     *  leg orientation, which snaps in steps and does not return to its start after a full 360.
     *
     *  ...EXCEPT when the host has asserted an override, which is the one case where the body is not
     *  where the player is playing from - see ResolveSelfPos. The angle needs no such treatment: it
     *  already comes from the camera, and while spectating that is exactly the right camera.
     *
     *  Note the aim axes are excluded while the map is open (MapMissionGameplay.UpdateAimSuppression),
     *  so this reads as the heading you had when you opened the map and holds still while you pan.
     *  That is the intent - it answers "which way was I facing", not "which way am I turning" - but
     *  it does mean the live turning behaviour can only be tested on the minimap.
     */
    protected void RenderSelfGlyph()
    {
        vector self_pos = ResolveSelfPos();
        if (self_pos == vector.Zero)
            return;

        float heading = Math.NormalizeAngle(GetGame().GetCurrentCameraDirection().VectorToAngles()[0]);

        VigridMapRender.WorldRenderHeadingArrow(m_TeamCanvas, m_MapWidget, self_pos, heading, VIGRID_MAP_SELF_PX, VIGRID_MAP_COLOR_SELF, VIGRID_MAP_SELF_LINE_WIDTH);
    }

    /**
     *  Players the host mod asked to be plotted: a square glyph each, and a name label over it.
     *
     *  POSITIONS COME FROM THE HOST, NOT FROM THE ENTITY LIST, and that is what makes this layer
     *  worth having at all - the host's source is its server, so it covers players far outside this
     *  client's network bubble. Building it from ClientData would show only whatever happens to be
     *  within about a kilometre, which on a map is close to nothing.
     *
     *  NO HEADING on the glyph. The push carries no yaw, and a glyph implying a facing it was never
     *  told would be worse than one that admits it does not know.
     *
     *  Both surfaces are cleared before ANY early return, on the rule the whole file follows: a
     *  canvas keeps its draw list and a widget pool keeps its last position, so a return that skipped
     *  either would burn the previous frame into the map until something else happened to repaint.
     */
    protected void RenderAdminPlayers()
    {
        int count = VigridMapAPI.GetAdminPlayerCount();

        if (m_AdminCanvas)
            m_AdminCanvas.Clear();

        if (count == 0)
        {
            HideAdminNames(0);
            return;
        }

        if (!m_AdminCanvas)
        {
            HideAdminNames(0);
            return;
        }

        //--- Names off at a wide zoom. Sixty labels at map-wide scale overlap into a smear that hides
        //--- the terrain and tells the admin nothing; the glyphs alone carry the picture until they
        //--- zoom into the fight they care about. The glyphs are never suppressed.
        //---
        //--- ⚠️ `<=`, NOT `>=`. Scale is 0 fully zoomed IN and 1 fully OUT, so a bigger number means
        //--- less detail and the threshold is a MAXIMUM. The first version had this backwards and
        //--- silently hid every label at the zoom an admin actually uses.
        bool want_names = m_MapWidget.GetScale() <= VIGRID_MAP_ADMIN_NAME_MAX_SCALE;

        float canvas_w;
        float canvas_h;
        m_AdminCanvas.GetScreenSize(canvas_w, canvas_h);

        int drawn = 0;
        int labelled = 0;
        int offscreen = 0;

        for (int i = 0; i < count; i++)
        {
            vector pos = VigridMapAPI.GetAdminPlayerPos(i);
            if (pos == vector.Zero)
                continue;

            int color = VigridMapAPI.GetAdminPlayerColor(i);

            VigridMapRender.WorldRenderSquare(m_AdminCanvas, m_MapWidget, pos, VIGRID_MAP_ADMIN_PX, color, VIGRID_MAP_ADMIN_LINE_WIDTH);
            drawn++;

            if (!want_names)
                continue;

            //--- Both the glyph and the label are now positioned in the SAME canvas-local pixels -
            //--- the label is a child of the MapWidget, like the canvas it is drawn over, so no
            //--- screen-origin correction is needed. It was needed while the labels lived under a
            //--- full-screen sibling frame, which is the arrangement that turned out not to render.
            float cx;
            float cy;
            VigridMapRender.WorldToCanvas(m_MapWidget, Vector(pos[0], 0, pos[2]), cx, cy);

            //--- Outside the visible map rect. The MapWidget carries clipchildren 1 so this is now
            //--- belt and braces, but it also keeps a label from being counted as drawn when it is
            //--- not - which is what makes the funnel line below trustworthy.
            if (cx < 0 || cy < 0 || cx > canvas_w || cy > canvas_h)
            {
                offscreen++;
                continue;
            }

            string name = VigridMapAPI.GetAdminPlayerName(i);
            if (name == "")
                continue;

            //--- Bounded by the DECLARED pool, which cannot grow at runtime. Glyphs are uncapped, so
            //--- a busier match loses names before it loses positions.
            if (labelled >= m_AdminNameTexts.Count())
                continue;

            if (PlaceAdminName(labelled, name, color, cx, cy))
                labelled++;
        }

        HideAdminNames(labelled);

        ReportAdminFunnel(count, drawn, labelled, offscreen, want_names);
    }

    /**
     *  Fill and position one label from the declared pool.
     *
     *  Returns false when the slot does not exist, which is what stops the caller counting a label
     *  it never drew. Nothing is created here - the pool is bound once in Init from the layout.
     *
     *  `cx`/`cy` are CANVAS-LOCAL pixels, the same coordinates the glyph was drawn at, because the
     *  label is a child of the same MapWidget.
     */
    protected bool PlaceAdminName(int slot, string name, int color, float cx, float cy)
    {
        if (slot < 0 || slot >= m_AdminNameTexts.Count())
            return false;

        TextWidget label = m_AdminNameTexts.Get(slot);
        if (!label)
            return false;

        label.SetText(name);
        label.SetColor(color);

        //--- Shown before being measured: a widget that has never been displayed reports a zero
        //--- size, and both offsets below are derived from it.
        label.Show(true);

        float row_w;
        float row_h;
        label.GetScreenSize(row_w, row_h);
        if (row_w <= 0)
            row_w = VIGRID_MAP_ADMIN_NAME_W;
        if (row_h <= 0)
            row_h = VIGRID_MAP_ADMIN_NAME_H;

        //--- SetPos anchors by the top-left corner, so centring on the glyph is half a width left,
        //--- and sitting above it is a full height plus the glyph's own half-size up.
        float px = cx - (row_w * 0.5);
        float py = cy - row_h - (VIGRID_MAP_ADMIN_PX * 0.5) - VIGRID_MAP_ADMIN_NAME_GAP_PX;

        label.SetPos(px, py);
        return true;
    }

    //! Hide every pooled label from `from` on. The pool is declared and fixed, so this only ever
    //! hides - there is nothing to unlink and nothing to free.
    protected void HideAdminNames(int from)
    {
        for (int i = from; i < m_AdminNameTexts.Count(); i++)
        {
            TextWidget label = m_AdminNameTexts.Get(i);
            if (label)
                label.Show(false);
        }
    }

    /**
     *  The funnel that found both of this layer's bugs, kept because it earned its place twice.
     *
     *  "Glyphs but no names" is indistinguishable by eye from five different causes - the zoom gate,
     *  an empty name in the payload, the pool failing to bind, a label clipped out, or the widget
     *  genuinely not rendering. Each reading named one directly:
     *
     *    rows=12 drawn=12 labelled=0  wantnames=false pool=0  -> the zoom GATE, comparison inverted
     *    rows=12 drawn=12 labelled=12 wantnames=true  pool=12 -> everything ran; NOTHING RENDERED
     *
     *  The second is what proved script-created widgets do not draw over a MapWidget even from a
     *  declared sibling frame, and sent the pool into the layout. Neither answer was reachable by
     *  reading the code, and each would otherwise have cost a build per hypothesis.
     *
     *  `scale` is logged raw and not just the verdict it produced: without it, "wantnames=false"
     *  says a gate fired but not whether the THRESHOLD or the COMPARISON was wrong.
     */
    protected void ReportAdminFunnel(int count, int drawn, int labelled, int offscreen, bool want_names)
    {
        if (GetGame().GetTime() < m_NextAdminFunnelMs)
            return;

        m_NextAdminFunnelMs = GetGame().GetTime() + VIGRID_MAP_ADMIN_FUNNEL_MS;

        //--- Built in steps rather than as one expression: a single concatenation of this many terms
        //--- is exactly the shape this engine rejects outright as "Formula too complex".
        string line = "[Admin] rows=" + count.ToString();
        line = line + " drawn=" + drawn.ToString();
        line = line + " labelled=" + labelled.ToString();
        line = line + " offscreen=" + offscreen.ToString();
        line = line + " wantnames=" + want_names.ToString();
        //--- The SCALE itself, not just the verdict it produced. Without it "wantnames=false" says a
        //--- gate fired but not whether the threshold or the COMPARISON was wrong - which is exactly
        //--- the ambiguity that cost this feature a build.
        line = line + " scale=" + m_MapWidget.GetScale().ToString();
        line = line + " pool=" + m_AdminNameTexts.Count().ToString();

        VigridMapLog.Debug(line);
    }

    protected void RenderTeammates()
    {
        //--- Dimmed rather than hidden when the pushes go quiet: a teammate's last known position is
        //--- still worth something, it just should not look as certain as a live one. The harder
        //--- cutoff, at which a member disappears entirely, is inside IsSlotVisible.
        float alpha = 1.0;
        if (VigridMapTeam.IsStale())
            alpha = VIGRID_MAP_TEAM_STALE_ALPHA;

        int count = VigridMapTeam.GetCount();
        int self_index = VigridMapTeam.GetSelfIndex();

        for (int i = 0; i < count; i++)
        {
            if (i == self_index)
                continue;
            if (!VigridMapTeam.IsSlotVisible(i))
                continue;

            //--- vector.Zero is the documented "no data" answer, not a real position at the map
            //--- origin - drawing it would park a triangle in the corner of the world.
            vector pos = VigridMapTeam.GetSlotPos(i);
            if (pos == vector.Zero)
                continue;

            VigridMapRender.WorldRenderTriangle(m_TeamCanvas, m_MapWidget, pos, VIGRID_MAP_TEAM_PX, VigridMapTeam.GetSlotColor(i, alpha), VIGRID_MAP_TEAM_LINE_WIDTH);
        }
    }

    /**
     *  Party's own world pings, finally on a map.
     *
     *  The index is compacted by the Party API, so an expired ping is never handed out here and this
     *  loop needs no expiry test of its own. Colour comes from the ping's owner, so two people
     *  marking at once are still telling you who marked what.
     */
    protected void RenderTeamPings()
    {
        int count = VigridMapTeam.GetPingCount();

        for (int i = 0; i < count; i++)
        {
            vector pos = VigridMapTeam.GetPingPos(i);
            if (pos == vector.Zero)
                continue;

            VigridMapRender.WorldRenderDiamond(m_TeamCanvas, m_MapWidget, pos, VIGRID_MAP_PING_PX, VigridMapTeam.GetPingColor(i, VIGRID_MAP_PING_ALPHA), VIGRID_MAP_PING_LINE_WIDTH);
        }
    }
}
#endif
