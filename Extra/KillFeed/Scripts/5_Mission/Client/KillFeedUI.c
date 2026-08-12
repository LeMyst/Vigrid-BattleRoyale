#ifndef SERVER
/**
 *  KillFeed - the feed itself.
 *
 *  Newest row on top. The widget pool is fixed at KILLFEED_MAX_ROWS and never grows or reorders;
 *  a new kill shifts the model list and the rows are re-bound from it, so there is no widget churn
 *  while people are dying in quick succession.
 *
 *  Rows are re-bound only when something actually changed. SetItem on an ItemPreviewWidget is not
 *  free, and a feed spends most of its life idle.
 */
class KillFeedUI
{
    private Widget m_Root;
    private Widget m_Rows;
    private bool m_RootFailed;

    //--- Newest first. Never longer than KILLFEED_MAX_ROWS.
    private ref array<ref KillFeedRowModel> m_Model;

    private ref array<Widget> m_RowWidgets;
    private ref array<TextWidget> m_RowKillers;
    private ref array<Widget> m_RowMiddles;
    private ref array<ItemPreviewWidget> m_RowWeapons;
    private ref array<ImageWidget> m_RowCauseIcons;
    private ref array<TextWidget> m_RowCauseTexts;
    private ref array<TextWidget> m_RowVictims;
    private ref array<TextWidget> m_RowDistances;

    void KillFeedUI()
    {
        m_Model = new array<ref KillFeedRowModel>();

        m_RowWidgets = new array<Widget>();
        m_RowKillers = new array<TextWidget>();
        m_RowMiddles = new array<Widget>();
        m_RowWeapons = new array<ItemPreviewWidget>();
        m_RowCauseIcons = new array<ImageWidget>();
        m_RowCauseTexts = new array<TextWidget>();
        m_RowVictims = new array<TextWidget>();
        m_RowDistances = new array<TextWidget>();
    }

    void ~KillFeedUI()
    {
        Clear();

        if (m_Root)
            m_Root.Unlink();
    }

    void Update(float timeslice)
    {
        if (!EnsureRoot())
            return;

        bool dirty = Drain();

        if (Expire())
            dirty = true;

        if (dirty)
            Refresh();
    }

    //! Drop every live row and the entities behind them. Used on mission teardown.
    void Clear()
    {
        int count = m_Model.Count();
        for (int i = 0; i < count; i++)
        {
            KillFeedRowModel model = m_Model.Get(i);
            if (model)
                model.Release();
        }

        m_Model.Clear();
    }

    /**
     *  Widgets are created on the first Update rather than in the constructor.
     *
     *  The constructor runs inside MissionGameplay.OnInit, and the only thing proven to work at
     *  that point is what the host mod does - it builds its HUD after super.OnInit() has returned.
     *  Deferring to the first frame means the mission is fully up before any layout is parsed, and
     *  it costs one boolean test per call.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        KillFeedLog.Debug("Creating kill feed layout");
        m_Root = GetGame().GetWorkspace().CreateWidgets(KILLFEED_PREFIX + "GUI/layouts/killfeed.layout");

        if (!m_Root)
        {
            //--- Latch the failure: retrying every frame would spam the log for the whole session.
            m_RootFailed = true;
            KillFeedLog.Error("Could not create killfeed.layout - kill feed disabled");
            return false;
        }

        m_Rows = m_Root.FindAnyWidget("KillFeedRows");
        if (!m_Rows)
        {
            m_RootFailed = true;
            m_Root.Unlink();
            m_Root = NULL;
            KillFeedLog.Error("killfeed.layout has no KillFeedRows widget - kill feed disabled");
            return false;
        }

        BuildRows();
        m_Root.Show(false);
        KillFeedLog.Debug("Kill feed layout ready");
        return true;
    }

    private void BuildRows()
    {
        for (int i = 0; i < KILLFEED_MAX_ROWS; i++)
        {
            Widget row = GetGame().GetWorkspace().CreateWidgets(KILLFEED_PREFIX + "GUI/layouts/killfeed_row.layout", m_Rows);
            if (!row)
            {
                KillFeedLog.Error("Could not create killfeed_row.layout");
                return;
            }

            row.SetPos(0, i * KILLFEED_ROW_HEIGHT);
            row.Show(false);

            m_RowWidgets.Insert(row);
            m_RowKillers.Insert(TextWidget.Cast(row.FindAnyWidget("RowKiller")));
            m_RowMiddles.Insert(row.FindAnyWidget("RowMiddle"));
            m_RowWeapons.Insert(ItemPreviewWidget.Cast(row.FindAnyWidget("RowWeapon")));
            m_RowCauseIcons.Insert(ImageWidget.Cast(row.FindAnyWidget("RowCauseIcon")));
            m_RowCauseTexts.Insert(TextWidget.Cast(row.FindAnyWidget("RowCauseText")));
            m_RowVictims.Insert(TextWidget.Cast(row.FindAnyWidget("RowVictim")));
            m_RowDistances.Insert(TextWidget.Cast(row.FindAnyWidget("RowDistance")));
        }
    }

    //! Move everything the RPC layer has received into the model. Returns true if anything moved.
    private bool Drain()
    {
        KillFeedRPC rpc = KillFeedRPC.GetInstance();
        if (!rpc)
            return false;

        int count = rpc.pending.Count();
        if (count == 0)
            return false;

        for (int i = 0; i < count; i++)
            Push(rpc.pending.Get(i));

        rpc.pending.Clear();
        return true;
    }

    private void Push(KillFeedEntry entry)
    {
        if (!entry)
            return;

        m_Model.InsertAt(new KillFeedRowModel(entry), 0);

        //--- Oldest rows fall off the bottom. Release before dropping the reference so the preview
        //--- entity goes with them rather than lingering in the world.
        while (m_Model.Count() > KILLFEED_MAX_ROWS)
        {
            int last = m_Model.Count() - 1;
            m_Model.Get(last).Release();
            m_Model.Remove(last);
        }
    }

    //! Drop rows whose time is up. Returns true if anything was dropped.
    private bool Expire()
    {
        bool changed = false;
        int now = GetGame().GetTime();

        for (int i = m_Model.Count() - 1; i >= 0; i--)
        {
            KillFeedRowModel model = m_Model.Get(i);
            if (model && model.expires_at > now)
                continue;

            if (model)
                model.Release();

            m_Model.Remove(i);
            changed = true;
        }

        return changed;
    }

    private void Refresh()
    {
        int model_count = m_Model.Count();
        int row_count = m_RowWidgets.Count();

        for (int i = 0; i < row_count; i++)
        {
            Widget row = m_RowWidgets.Get(i);
            if (!row)
                continue;

            if (i >= model_count)
            {
                //--- Drop the reference as the row goes dark, so a hidden widget never keeps a
                //--- deleted entity alive.
                if (m_RowWeapons.Get(i))
                    m_RowWeapons.Get(i).SetItem(NULL);

                row.Show(false);
                continue;
            }

            //--- Show before binding: GetTextSize measures a laid-out widget, and vanilla's own
            //--- measure sites call Show(true) first (actiontargetscursor.c:1148-1151).
            row.Show(true);
            Bind(i, m_Model.Get(i));
        }

        m_Root.Show(model_count > 0);
    }

    private void Bind(int index, KillFeedRowModel model)
    {
        if (!model)
            return;

        KillFeedEntry entry = model.entry;
        if (!entry)
            return;

        TextWidget killer = m_RowKillers.Get(index);
        if (killer)
            killer.SetText(entry.killer_name);

        TextWidget victim = m_RowVictims.Get(index);
        if (victim)
            victim.SetText(entry.victim_name);

        TextWidget distance = m_RowDistances.Get(index);
        if (distance)
        {
            if (entry.distance >= 0)
                distance.SetText(entry.distance.ToString() + " m");
            else
                distance.SetText("");
        }

        //--- The middle cell is exclusive: a model to look at, or an icon and a phrase. Never both.
        bool has_model = model.preview != NULL;

        ItemPreviewWidget weapon = m_RowWeapons.Get(index);
        if (weapon)
        {
            weapon.SetItem(model.preview);
            weapon.Show(has_model);
        }

        ImageWidget icon = m_RowCauseIcons.Get(index);
        if (icon)
            icon.Show(!has_model);

        TextWidget cause = m_RowCauseTexts.Get(index);
        if (cause)
        {
            cause.Show(!has_model);
            if (!has_model)
                cause.SetText(CausePhrase(entry.cause));
        }

        LayoutRow(index, model, has_model);
    }

    /**
     *  Place the four cells and shrink the row to fit them.
     *
     *  Done in script rather than with a spacer's "Size To Content H": a spacer can only collapse to
     *  the sum of its children, and these children are fixed-width boxes, so it had nothing to
     *  collapse to and the background stretched the full declared width.
     *
     *  GetTextSize reports the currently set text in pixels and is valid in the same frame as
     *  SetText - vanilla measures exactly this way in actiontargetscursor.c:1141-1153.
     */
    private void LayoutRow(int index, KillFeedRowModel model, bool has_model)
    {
        Widget row = m_RowWidgets.Get(index);
        if (!row)
            return;

        int x = KILLFEED_ROW_PAD;
        int text_w;
        int text_h;

        //--- Killer. A zone death has nobody to credit, so the cell contributes nothing at all
        //--- rather than an empty gap.
        TextWidget killer = m_RowKillers.Get(index);
        if (killer)
        {
            //--- Update() before every read-back: the row was only just shown, and a stale layout
            //--- measures as zero. TabberUI.c:126 and sizetochild.c:35 do the same.
            killer.Update();
            killer.GetTextSize(text_w, text_h);
            killer.Show(text_w > 0);

            if (text_w > 0)
            {
                killer.SetPos(x, 0);
                killer.SetSize(text_w, KILLFEED_ROW_INNER_HEIGHT);
                x = x + text_w + KILLFEED_ROW_GAP;
            }
        }

        //--- Middle cell: the weapon box, or the icon plus its phrase.
        int middle_w = KILLFEED_ICON_WIDTH;
        if (has_model)
        {
            //--- Match the box to the model's aspect. A fixed box left dead space for anything
            //--- shorter than it was wide, which is what put a gap before the victim name.
            float wanted = KILLFEED_ROW_INNER_HEIGHT * model.preview_aspect;
            middle_w = Math.Round(Math.Clamp(wanted, KILLFEED_WEAPON_MIN_WIDTH, KILLFEED_WEAPON_MAX_WIDTH));

            //--- The preview widget itself defines the render viewport, so it has to be resized
            //--- too - resizing only its parent panel would leave the model in its old box.
            ItemPreviewWidget preview = m_RowWeapons.Get(index);
            if (preview)
                preview.SetSize(middle_w, KILLFEED_ROW_INNER_HEIGHT);
        }
        else
        {
            TextWidget cause = m_RowCauseTexts.Get(index);
            if (cause)
            {
                cause.Update();
                cause.GetTextSize(text_w, text_h);
                cause.SetPos(KILLFEED_ICON_WIDTH + KILLFEED_ROW_GAP, 0);
                cause.SetSize(text_w, KILLFEED_ROW_INNER_HEIGHT);
                middle_w = middle_w + KILLFEED_ROW_GAP + text_w;
            }
        }

        Widget middle = m_RowMiddles.Get(index);
        if (middle)
        {
            middle.SetPos(x, 0);
            middle.SetSize(middle_w, KILLFEED_ROW_INNER_HEIGHT);
        }
        x = x + middle_w + KILLFEED_ROW_GAP;

        TextWidget victim = m_RowVictims.Get(index);
        if (victim)
        {
            victim.Update();
            victim.GetTextSize(text_w, text_h);
            victim.Show(text_w > 0);

            if (text_w > 0)
            {
                victim.SetPos(x, 0);
                victim.SetSize(text_w, KILLFEED_ROW_INNER_HEIGHT);
                x = x + text_w + KILLFEED_ROW_GAP;
            }
        }

        //--- Distance is blank for melee and for every environmental cause.
        TextWidget distance = m_RowDistances.Get(index);
        if (distance)
        {
            distance.Update();
            distance.GetTextSize(text_w, text_h);
            distance.Show(text_w > 0);

            if (text_w > 0)
            {
                distance.SetPos(x, 0);
                distance.SetSize(text_w, KILLFEED_ROW_INNER_HEIGHT);
                x = x + text_w + KILLFEED_ROW_GAP;
            }
        }

        //--- The trailing gap becomes the right-hand inset.
        int total = x - KILLFEED_ROW_GAP + KILLFEED_ROW_PAD;

        KillFeedLog.Trace(string.Format("LayoutRow %1: width %2", index, total));

        row.SetSize(total, KILLFEED_ROW_INNER_HEIGHT);

        //--- Re-anchor after the resize. With halign right_ref, x = 0 puts the row's right edge
        //--- flush against the parent's, whatever its width.
        row.SetPos(0, index * KILLFEED_ROW_HEIGHT);
    }

    /**
     *  The phrase shown when there is no weapon to render. Returned with its '#' so the widget
     *  localises it; the weapon causes never reach here because they render a model instead.
     */
    private string CausePhrase(int cause)
    {
        if (cause == KillFeedCause.ZONE)
            return STR_KILLFEED_ZONE;
        if (cause == KillFeedCause.INFECTED)
            return STR_KILLFEED_INFECTED;
        if (cause == KillFeedCause.ANIMAL)
            return STR_KILLFEED_ANIMAL;
        if (cause == KillFeedCause.BAREHANDS)
            return STR_KILLFEED_BAREHANDS;
        if (cause == KillFeedCause.EXPLOSIVE)
            return STR_KILLFEED_EXPLOSIVE;

        return STR_KILLFEED_ENVIRONMENT;
    }
}
#endif
