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

            Bind(i, m_Model.Get(i));
            row.Show(true);
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
