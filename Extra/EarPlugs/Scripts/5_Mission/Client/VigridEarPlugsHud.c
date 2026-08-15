#ifndef SERVER
/**
 *  EarPlugs - the on-screen badge.
 *
 *  ⚠️ THIS IS PERSISTENT, AND THAT IS THE POINT. The reference implementation flashed an icon for
 *  about a second and then faded it to nothing, which means a player who plugs their ears and then
 *  forgets is permanently half-deaf with no cue anywhere on screen explaining why they cannot hear
 *  footsteps. So the badge stays up for as long as the plugs are in - full alpha for a couple of
 *  seconds after a change, then eased back to a quiet idle alpha it never drops below.
 *
 *  The one state with no persistent badge is Off, which needs the opposite treatment: nothing to
 *  show afterwards, but the player still has to be told the plugs came OUT. So a change to Off
 *  flashes "EAR PLUGS / OUT" for the same couple of seconds and then hides.
 *
 *  Shape is VigridMapMinimap's throughout - widgets built on the first Update rather than in the
 *  constructor, a latch so a failed build logs once instead of every tick, Show() only on an edge,
 *  and Unlink() in the destructor.
 */
class VigridEarPlugsHud
{
    private Widget m_Root;
    private bool m_RootFailed;

    private Widget m_Backdrop;
    private TextWidget m_Label;
    private TextWidget m_Level;

    private bool m_Shown;

    //--- Cached so the layout arithmetic only re-runs when the viewport actually moves.
    private float m_ParentW;

    private int m_FlashUntilMs;
    private int m_DisplayLevel = -1;

    void ~VigridEarPlugsHud()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Announce a change the player just made.
     *
     *  Called from the keybind handler rather than polled off the controller: the controller has no
     *  business knowing there is a HUD, and an edge the caller already has is not worth rediscovering
     *  by diffing a field every frame.
     */
    void Flash(int level)
    {
        //--- Deliberately does NOT touch m_DisplayLevel or the widget. This can be called before the
        //--- layout exists, and half-applying the change would leave Update's diff believing the
        //--- text was already repainted. Setting the deadline alone keeps one owner for the text.
        m_FlashUntilMs = GetGame().GetTime() + VIGRID_EARPLUGS_HUD_FLASH_MS;
    }

    void Update(int level)
    {
        if (!EnsureRoot())
            return;

        //--- Covers the level being restored from prefs at session start, which reaches the HUD
        //--- through nobody's Flash() call.
        if (level != m_DisplayLevel)
        {
            m_DisplayLevel = level;
            m_Level.SetText(VigridEarPlugsLevels.LabelKey(level));
        }

        bool flashing = GetGame().GetTime() < m_FlashUntilMs;

        if (!ShouldShow(level, flashing))
        {
            if (m_Shown)
            {
                m_Root.Show(false);
                m_Shown = false;
            }
            return;
        }

        ApplyScale();

        if (!m_Shown)
        {
            m_Root.Show(true);
            m_Shown = true;
        }

        m_Root.SetAlpha(ResolveAlpha(level, flashing));
    }

    /**
     *  Built on the first Update rather than in the constructor: that runs inside
     *  MissionGameplay.OnInit, and the only timing proven to work in this mod is after
     *  super.OnInit() has returned.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_EARPLUGS_PREFIX + "GUI/layouts/earplugs.layout");

        if (!m_Root)
        {
            //--- Latched: retrying every tick would spam the log for the whole session.
            m_RootFailed = true;
            VigridEarPlugsLog.Error("Could not create earplugs.layout - badge disabled");
            return false;
        }

        m_Backdrop = m_Root.FindAnyWidget("EarPlugsBackdrop");
        m_Label = TextWidget.Cast(m_Root.FindAnyWidget("EarPlugsLabel"));
        m_Level = TextWidget.Cast(m_Root.FindAnyWidget("EarPlugsLevel"));

        if (!m_Backdrop || !m_Label || !m_Level)
        {
            //--- Fatal rather than a warning: with any of the three missing there is no badge, only
            //--- a fragment of one, and a fragment is worse than nothing for a cue whose whole job
            //--- is to be unambiguous.
            m_RootFailed = true;
            m_Root.Unlink();
            m_Root = NULL;
            VigridEarPlugsLog.Error("earplugs.layout is missing a widget - badge disabled");
            return false;
        }

        m_Root.Show(false);
        VigridEarPlugsLog.Debug("Badge layout ready");
        return true;
    }

    private bool ShouldShow(int level, bool flashing)
    {
        //--- Off and not announcing anything: there is nothing to say.
        if (level == VIGRID_EARPLUGS_LEVEL_OFF && !flashing)
            return false;

        //--- Behind any full-screen menu. The map, the inventory and the death screen all deserve
        //--- the room, and the badge is not urgent enough to sit on top of them.
        UIManager ui = GetGame().GetUIManager();
        if (ui && ui.GetMenu())
            return false;

        return GetGame().GetPlayer() != NULL;
    }

    /**
     *  Full alpha while announcing, then an ease down to the idle floor.
     *
     *  It never reaches zero: the floor IS the feature, and a badge that fades out is the reference
     *  implementation's bug.
     */
    private float ResolveAlpha(int level, bool flashing)
    {
        if (flashing)
            return VIGRID_EARPLUGS_HUD_FLASH_ALPHA;

        //--- Off is only ever visible mid-flash, so anything past the flash is on its way out and
        //--- the next Update will hide it. Holding full alpha until then avoids a one-frame dip.
        if (level == VIGRID_EARPLUGS_LEVEL_OFF)
            return VIGRID_EARPLUGS_HUD_FLASH_ALPHA;

        //--- Held as a float from the start: `int / int` is integer division in EnfusionScript, so
        //--- the progress term would only ever be 0 or 1 and the ease would be a hard cut.
        float since = GetGame().GetTime() - m_FlashUntilMs;
        if (since >= VIGRID_EARPLUGS_HUD_FADE_MS)
            return VIGRID_EARPLUGS_HUD_IDLE_ALPHA;

        float progress = since / VIGRID_EARPLUGS_HUD_FADE_MS;
        float span = VIGRID_EARPLUGS_HUD_FLASH_ALPHA - VIGRID_EARPLUGS_HUD_IDLE_ALPHA;

        return VIGRID_EARPLUGS_HUD_FLASH_ALPHA - (span * progress);
    }

    /**
     *  Place and size the three children in REAL screen pixels.
     *
     *  Everything is authored against a 1920-wide screen and multiplied by parent_w / REFERENCE_W,
     *  which is the same treatment VigridMapCompass gives its strip. Re-run only when the viewport
     *  moves, since nothing else here changes.
     */
    private void ApplyScale()
    {
        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);

        if (parent_w <= 0)
            return;
        if (Math.AbsFloat(parent_w - m_ParentW) < 1.0)
            return;

        m_ParentW = parent_w;

        float scale = parent_w / VIGRID_EARPLUGS_HUD_REFERENCE_W;
        float x = VIGRID_EARPLUGS_HUD_X * scale;
        float y = VIGRID_EARPLUGS_HUD_Y * scale;
        float w = VIGRID_EARPLUGS_HUD_W * scale;
        float h = VIGRID_EARPLUGS_HUD_H * scale;
        float pad = 10.0 * scale;

        m_Backdrop.SetPos(x, y);
        m_Backdrop.SetSize(w, h);

        //--- The label takes the left 55% and the level the right, so a long localized level word
        //--- (Polish "ZATYCZKI", Czech "VYNDÁNO") has somewhere to go without overrunning the label.
        float split = w * 0.55;

        m_Label.SetPos(x + pad, y);
        m_Label.SetSize(split - pad, h);

        m_Level.SetPos(x + split, y);
        m_Level.SetSize(w - split - pad, h);

        VigridEarPlugsLog.Debug("Badge laid out for viewport " + parent_w.ToString());
    }
}
#endif
