#ifndef SERVER
/**
 *  Vigrid Party - teammate status panel.
 *
 *  One row per teammate: name, health bar, health/blood readout, distance. Health always comes from
 *  the server push, never from the local entity, because a teammate outside the network bubble has
 *  no entity to read.
 *
 *  Rows are pooled and positioned manually rather than through a spacer widget, so the layout
 *  behaves identically regardless of how many rows are live.
 */
class VigridPartyHud
{
    private static const int ROW_HEIGHT = 78;

    private static const int COLOR_HEALTHY = 0xFF6ECF6E;
    private static const int COLOR_HURT = 0xFFE8A33D;
    private static const int COLOR_CRITICAL = 0xFFD64545;
    private static const int COLOR_INACTIVE = 0xFF787878;

    private Widget m_Root;
    private Widget m_Rows;
    private bool m_RootFailed;

    private ref array<Widget> m_RowWidgets;
    private ref array<TextWidget> m_RowNames;
    private ref array<ProgressBarWidget> m_RowBars;
    private ref array<TextWidget> m_RowHealthTexts;
    private ref array<TextWidget> m_RowDistances;

    void VigridPartyHud()
    {
        m_RowWidgets = new array<Widget>();
        m_RowNames = new array<TextWidget>();
        m_RowBars = new array<ProgressBarWidget>();
        m_RowHealthTexts = new array<TextWidget>();
        m_RowDistances = new array<TextWidget>();
    }

    /**
     *  Widgets are created on the first Update rather than in the constructor.
     *
     *  The constructor runs inside MissionGameplay.OnInit, and the only thing actually proven to
     *  work at that point is what the Battle Royale mod does - which builds its HUD *after*
     *  super.OnInit() has returned. Deferring to the first frame means the mission is fully up
     *  before any layout is parsed, and it costs one boolean test per call.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        VigridPartyLog.Debug("Creating party HUD layout");
        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_hud.layout");

        if (!m_Root)
        {
            //--- Latch the failure: retrying every frame would spam the log for the whole session.
            m_RootFailed = true;
            VigridPartyLog.Error("Could not create party_hud.layout - HUD panel disabled");
            return false;
        }

        m_Rows = m_Root.FindAnyWidget("PartyRows");
        m_Root.Show(false);
        VigridPartyLog.Debug("Party HUD layout ready");
        return true;
    }

    void ~VigridPartyHud()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    private void EnsureCapacity(int wanted)
    {
        if (!m_Rows)
            return;

        while (m_RowWidgets.Count() < wanted)
        {
            Widget row = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_hud_row.layout", m_Rows);
            if (!row)
                return;

            row.SetPos(0, m_RowWidgets.Count() * ROW_HEIGHT);

            m_RowWidgets.Insert(row);
            m_RowNames.Insert(TextWidget.Cast(row.FindAnyWidget("RowName")));
            m_RowBars.Insert(ProgressBarWidget.Cast(row.FindAnyWidget("RowHealthBar")));
            m_RowHealthTexts.Insert(TextWidget.Cast(row.FindAnyWidget("RowHealthText")));
            m_RowDistances.Insert(TextWidget.Cast(row.FindAnyWidget("RowDistanceText")));
        }
    }

    void Update()
    {
        if (!EnsureRoot())
            return;
        if (!m_Rows)
            return;

        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (!rpc.enabled || !rpc.HasParty())
        {
            m_Root.Show(false);
            return;
        }

        int member_count = rpc.roster_uids.Count();
        EnsureCapacity(member_count);

        bool have_state = rpc.state_version == rpc.roster_version;
        int age_ms = GetGame().GetTime() - rpc.state_recv_ms;
        bool stale = age_ms > (3 * VIGRID_PARTY_DEF_STATE_INTERVAL_MS);

        vector self_pos = vector.Zero;
        PlayerBase local_player = PlayerBase.Cast(GetGame().GetPlayer());
        if (local_player)
            self_pos = local_player.GetPosition();

        m_Root.Show(true);

        int slot = 0;
        for (int i = 0; i < member_count; i++)
        {
            if (i == rpc.self_index)
                continue;
            if (slot >= m_RowWidgets.Count())
                break;

            RenderRow(slot, i, rpc, have_state, stale, self_pos);
            slot = slot + 1;
        }

        for (int j = slot; j < m_RowWidgets.Count(); j++)
        {
            m_RowWidgets.Get(j).Show(false);
        }
    }

    private void RenderRow(int slot, int index, VigridPartyRPC rpc, bool have_state, bool stale, vector self_pos)
    {
        Widget row = m_RowWidgets.Get(slot);
        row.Show(true);
        row.SetPos(0, slot * ROW_HEIGHT);

        string display_name = rpc.roster_names.Get(index);
        if (index == rpc.leader_index)
            display_name = display_name + " *"; //!< leader marker

        m_RowNames.Get(slot).SetText(display_name);

        //--- No usable push yet: show the member but say nothing about their condition rather than
        //--- rendering a stale or zeroed bar.
        if (!have_state || index >= rpc.state_health.Count())
        {
            m_RowBars.Get(slot).SetCurrent(0);
            m_RowHealthTexts.Get(slot).SetText("--");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_INACTIVE);
            return;
        }

        int member_flags = rpc.state_flags.Get(index);
        bool online = (member_flags & VIGRID_PARTY_FLAG_ONLINE) != 0;
        bool alive = (member_flags & VIGRID_PARTY_FLAG_ALIVE) != 0;
        int health = rpc.state_health.Get(index);

        if (!online)
        {
            m_RowBars.Get(slot).SetCurrent(0);
            m_RowHealthTexts.Get(slot).SetText("#STR_PARTY_HUD_OFFLINE");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_INACTIVE);
            return;
        }

        if (!alive)
        {
            m_RowBars.Get(slot).SetCurrent(0);
            m_RowHealthTexts.Get(slot).SetText("#STR_PARTY_HUD_DEAD");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_CRITICAL);
            return;
        }

        m_RowBars.Get(slot).SetCurrent(health);

        StringLocaliser health_text = new StringLocaliser("STR_PARTY_HUD_HP", health.ToString(), rpc.state_blood.Get(index).ToString());
        m_RowHealthTexts.Get(slot).SetText(health_text.Format());

        int color = COLOR_HEALTHY;
        if (health <= 25)
            color = COLOR_CRITICAL;
        else if (health <= 60)
            color = COLOR_HURT;

        m_RowNames.Get(slot).SetColor(color);

        vector member_pos = rpc.state_positions.Get(index);
        if (self_pos != vector.Zero && member_pos != vector.Zero)
            m_RowDistances.Get(slot).SetText(Math.Round(vector.Distance(self_pos, member_pos)).ToString() + "m");
        else
            m_RowDistances.Get(slot).SetText("");

        //--- Dim the whole row when the data behind it has gone quiet.
        if (stale)
            row.SetAlpha(0.5);
        else
            row.SetAlpha(1.0);
    }
}
#endif
