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

    //--- One widget per font size, because glyph size is fixed by the DECLARED face: there is no
    //--- SetFont, and SetTextExactSize was measured to do nothing on this mod. PickTier shows one
    //--- pair and hides the other, the same construct as VigridMapCompass's three label tiers.
    private TextWidget m_LabelLarge;
    private TextWidget m_LevelLarge;
    private TextWidget m_LabelSmall;
    private TextWidget m_LevelSmall;

    //--- The pair currently shown. Everything downstream measures and places these two.
    private TextWidget m_Label;
    private TextWidget m_Level;

    private bool m_Shown;
    private bool m_LargeTier;
    private bool m_TierPicked;

    //--- Last geometry actually written, so an unchanged frame writes nothing.
    private float m_LastX;
    private float m_LastW;
    private float m_LastH;

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

            //--- Both tiers, not just the shown one: the hidden pair must already carry the right
            //--- word for the frame a resolution change swaps to it.
            string key = VigridEarPlugsLevels.LabelKey(level);
            m_LevelLarge.SetText(key);
            m_LevelSmall.SetText(key);

            //--- The word changed, so its measured width did too.
            m_LastW = 0;
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

        PickTier();
        LayoutBadge();

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
        m_LabelLarge = TextWidget.Cast(m_Root.FindAnyWidget("EarPlugsLabelLarge"));
        m_LevelLarge = TextWidget.Cast(m_Root.FindAnyWidget("EarPlugsLevelLarge"));
        m_LabelSmall = TextWidget.Cast(m_Root.FindAnyWidget("EarPlugsLabelSmall"));
        m_LevelSmall = TextWidget.Cast(m_Root.FindAnyWidget("EarPlugsLevelSmall"));

        if (!m_Backdrop || !m_LabelLarge || !m_LevelLarge || !m_LabelSmall || !m_LevelSmall)
        {
            //--- Fatal rather than a warning: with any of them missing there is no badge, only a
            //--- fragment of one, and a fragment is worse than nothing for a cue whose whole job is
            //--- to be unambiguous.
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
     *  Choose the font tier for this viewport, and show that pair.
     *
     *  There is no SetFont in EnfusionScript and SetTextExactSize was measured to do nothing on this
     *  mod - 28/18/13 on one widget rendered 28/28/28 - so N sizes means N widgets. Same answer
     *  VigridMapCompass reached for its three label tiers.
     */
    private void PickTier()
    {
        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);

        if (parent_w <= 0)
            return;

        bool large = parent_w >= VIGRID_EARPLUGS_HUD_TIER_W;
        if (m_TierPicked && large == m_LargeTier)
            return;

        m_LargeTier = large;
        m_TierPicked = true;

        m_LabelLarge.Show(large);
        m_LevelLarge.Show(large);
        m_LabelSmall.Show(!large);
        m_LevelSmall.Show(!large);

        if (large)
        {
            m_Label = m_LabelLarge;
            m_Level = m_LevelLarge;
        }
        else
        {
            m_Label = m_LabelSmall;
            m_Level = m_LevelSmall;
        }

        //--- Force a re-measure: the faces just changed underneath us.
        m_LastW = 0;

        string tier_line = "Font tier large=";
        tier_line = tier_line + large.ToString();
        tier_line = tier_line + " for viewport " + parent_w.ToString();
        VigridEarPlugsLog.Debug(tier_line);
    }

    /**
     *  Size the badge to its TEXT, and place it.
     *
     *  ⚠️ THE BOX MUST NOT SCALE WITH THE VIEWPORT, and that was the original bug. The first version
     *  multiplied a 190x34 box by parent_w / 1920, which is the correct treatment for a widget whose
     *  contents scale too - and these do not. At 2560 wide it read fine; at 1280 the box came out
     *  127x23 around text still drawn at its authored pixel size, so the label overran the level word
     *  and the whole badge clipped. A box whose contents cannot scale must be sized BY those contents.
     *
     *  Measuring also retires the old fixed 55/45 split, which was only ever a guess at how long a
     *  localized level word might be. Czech "VYNDÁNO" and French "BOUCHONS" now get exactly the room
     *  they need, in any of the fourteen languages.
     *
     *  Re-measured every shown frame rather than on an edge: GetTextSize straight after SetText can
     *  read the pre-layout value - the same trap as probing a ScrollWidget's content height in its
     *  creation frame - and two proto calls a frame is not worth an edge-detection bug. Nothing is
     *  WRITTEN unless the numbers actually moved.
     */
    private void LayoutBadge()
    {
        if (!m_Label || !m_Level)
            return;

        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);
        if (parent_w <= 0)
            return;

        int label_w;
        int label_h;
        m_Label.GetTextSize(label_w, label_h);

        int level_w;
        int level_h;
        m_Level.GetTextSize(level_w, level_h);

        //--- A zero measurement means the widget has not been laid out yet. Writing a zero-width box
        //--- would flash an empty backdrop for a frame, so wait for the next one instead.
        if (label_w <= 0 || level_w <= 0)
            return;

        float pad = VIGRID_EARPLUGS_HUD_PAD_LARGE;
        float gap = VIGRID_EARPLUGS_HUD_GAP_LARGE;
        float vpad = VIGRID_EARPLUGS_HUD_VPAD_LARGE;

        if (!m_LargeTier)
        {
            pad = VIGRID_EARPLUGS_HUD_PAD_SMALL;
            gap = VIGRID_EARPLUGS_HUD_GAP_SMALL;
            vpad = VIGRID_EARPLUGS_HUD_VPAD_SMALL;
        }

        float text_h = label_h;
        if (level_h > text_h)
            text_h = level_h;

        float w = pad + label_w + gap + level_w + pad;
        float h = text_h + vpad + vpad;

        //--- Only the POSITION follows the viewport, so the badge keeps the same corner on every
        //--- screen while staying legible at its own fixed pixel size.
        float scale = parent_w / VIGRID_EARPLUGS_HUD_REFERENCE_W;
        float x = VIGRID_EARPLUGS_HUD_X * scale;
        float y = VIGRID_EARPLUGS_HUD_Y * scale;

        if (Math.AbsFloat(w - m_LastW) < 0.5 && Math.AbsFloat(h - m_LastH) < 0.5 && Math.AbsFloat(x - m_LastX) < 0.5)
            return;

        m_LastW = w;
        m_LastH = h;
        m_LastX = x;

        m_Backdrop.SetPos(x, y);
        m_Backdrop.SetSize(w, h);

        //--- Each text widget is sized to exactly its own text, so the halign declared in the layout
        //--- never comes into play and a mis-set one cannot shift anything.
        m_Label.SetPos(x + pad, y + vpad);
        m_Label.SetSize(label_w, text_h);

        m_Level.SetPos(x + w - pad - level_w, y + vpad);
        m_Level.SetSize(level_w, text_h);

        string line = "Badge laid out ";
        line = line + w.ToString() + "x" + h.ToString();
        line = line + " label=" + label_w.ToString();
        line = line + " level=" + level_w.ToString();
        VigridEarPlugsLog.Debug(line);
    }
}
#endif
