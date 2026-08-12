#ifndef SERVER
/**
 *  Vigrid Party - teammate status panel.
 *
 *  One row per teammate: name, condition icons, distance. Health always comes from the server push,
 *  never from the local entity, because a teammate outside the network bubble has no entity to read.
 *
 *  Condition is shown as the vanilla heart and blood-drop badges, never as a number or a bar. A
 *  DayZ player cannot read their own health as a value, so neither should they be able to read a
 *  teammate's - the row says how bad it is, and nothing finer.
 *
 *  The badges follow vanilla's own system rather than an invented one. The server asks vanilla for
 *  the level outright - GetStatLevelHealth/GetStatLevelBlood, the same calls behind the player's own
 *  HUD badge - and sends that 0..4 EStatLevels value, so no threshold is duplicated here and none
 *  can drift. The level then selects both the artwork (image0..image4, degrading as the stat drops)
 *  and the tint (white, then yellow, then red).
 *
 *  The one vanilla behaviour deliberately left out is the blink on the worst level - vanilla blinks
 *  a CRITICAL badge, which is tolerable for one badge of your own but not for a column of rows.
 *
 *  Rows are pooled and positioned manually rather than through a spacer widget, so the layout
 *  behaves identically regardless of how many rows are live.
 */
class VigridPartyHud
{
    private static const int ROW_HEIGHT = 70;

    private static const int COLOR_HEALTHY = 0xFF6ECF6E;
    private static const int COLOR_HURT = 0xFFE8A33D;
    private static const int COLOR_CRITICAL = 0xFFD64545;
    private static const int COLOR_INACTIVE = 0xFF787878;

    //--- The five buckets vanilla sorts a stat into (EStatLevels). The number is also the image
    //--- index: the layout declares image0..image4 in the same order, so SetImage(level) picks the
    //--- matching artwork exactly the way InGameHud.DisplayTendencyNormal does.
    private static const int LEVEL_GREAT = 0;
    private static const int LEVEL_HIGH = 1;
    private static const int LEVEL_MEDIUM = 2;
    private static const int LEVEL_LOW = 3;
    private static const int LEVEL_CRITICAL = 4;

    //--- Vanilla's icon tints, from InGameHud.DisplayTendencyNormal: white until the stat is half
    //--- gone, then yellow, then red. 220 rather than 255 is vanilla's own value.
    private static const int COLOR_ICON_NORMAL = 0xFFDCDCDC;
    private static const int COLOR_ICON_WARNING = 0xFFDCDC00;
    private static const int COLOR_ICON_DANGER = 0xFFDC0000;


    private Widget m_Root;
    private Widget m_Rows;
    private bool m_RootFailed;

    private ref array<Widget> m_RowWidgets;
    private ref array<TextWidget> m_RowNames;
    private ref array<ImageWidget> m_RowHealthIcons;
    private ref array<ImageWidget> m_RowBloodIcons;
    private ref array<TextWidget> m_RowStatusTexts;
    private ref array<TextWidget> m_RowDistances;

    void VigridPartyHud()
    {
        m_RowWidgets = new array<Widget>();
        m_RowNames = new array<TextWidget>();
        m_RowHealthIcons = new array<ImageWidget>();
        m_RowBloodIcons = new array<ImageWidget>();
        m_RowStatusTexts = new array<TextWidget>();
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

    /**
     *  Load all five frames of one badge into the widget's image slots.
     *
     *  A layout can only ever declare image0 - the parser knows no `image1`, and not one vanilla
     *  layout uses one. SetImage() switches only between slots that have actually been loaded, so
     *  without this it silently returns false and the artwork never changes. InGameHud does exactly
     *  this loop for the vanilla badges before it ever calls SetImage.
     */
    private void LoadIconFrames(ImageWidget icon, string base_name)
    {
        if (!icon)
            return;

        for (int i = LEVEL_GREAT; i <= LEVEL_CRITICAL; i++)
        {
            icon.LoadImageFile(i, "set:dayz_gui image:" + base_name + i.ToString());
        }
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

            ImageWidget health_icon = ImageWidget.Cast(row.FindAnyWidget("RowHealthIcon"));
            ImageWidget blood_icon = ImageWidget.Cast(row.FindAnyWidget("RowBloodIcon"));
            LoadIconFrames(health_icon, "iconHealth");
            LoadIconFrames(blood_icon, "iconBlood");

            m_RowWidgets.Insert(row);
            m_RowNames.Insert(TextWidget.Cast(row.FindAnyWidget("RowName")));
            m_RowHealthIcons.Insert(health_icon);
            m_RowBloodIcons.Insert(blood_icon);
            m_RowStatusTexts.Insert(TextWidget.Cast(row.FindAnyWidget("RowStatusText")));
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

    /**
     *  Vanilla holds white across its top two levels and only starts colouring at MEDIUM, so the
     *  icon degrades by artwork first and by colour second.
     */
    private int LevelColor(int level)
    {
        if (level >= LEVEL_LOW)
            return COLOR_ICON_DANGER;
        if (level == LEVEL_MEDIUM)
            return COLOR_ICON_WARNING;

        return COLOR_ICON_NORMAL;
    }

    /**
     *  Icons and status text are mutually exclusive - they occupy the same strip, and the text is
     *  only there for the states no icon can express (offline, dead, no data yet).
     */
    private void SetIcons(int slot, bool show)
    {
        m_RowHealthIcons.Get(slot).Show(show);
        m_RowBloodIcons.Get(slot).Show(show);
        m_RowStatusTexts.Get(slot).Show(!show);
    }

    private void RenderRow(int slot, int index, VigridPartyRPC rpc, bool have_state, bool stale, vector self_pos)
    {
        Widget row = m_RowWidgets.Get(slot);
        row.Show(true);
        row.SetPos(0, slot * ROW_HEIGHT);

        //--- Through the API, not rpc.roster_names: an offline member's name can arrive as a
        //--- stringtable key and only GetMemberName resolves it - and it has to happen before the
        //--- leader marker is appended, since SetText only localises a string that STARTS with '#'.
        string display_name = VigridPartyAPI.GetMemberName(index);
        if (index == rpc.leader_index)
            display_name = display_name + " *"; //!< leader marker

        m_RowNames.Get(slot).SetText(display_name);

        //--- No usable push yet: show the member but say nothing about their condition rather than
        //--- tinting the icons from stale or zeroed values.
        if (!have_state || index >= rpc.state_health_level.Count())
        {
            SetIcons(slot, false);
            m_RowStatusTexts.Get(slot).SetText("--");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_INACTIVE);
            return;
        }

        int member_flags = rpc.state_flags.Get(index);
        bool online = (member_flags & VIGRID_PARTY_FLAG_ONLINE) != 0;
        bool alive = (member_flags & VIGRID_PARTY_FLAG_ALIVE) != 0;
        int health_level = rpc.state_health_level.Get(index);

        if (!online)
        {
            SetIcons(slot, false);
            m_RowStatusTexts.Get(slot).SetText("#STR_PARTY_HUD_OFFLINE");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_INACTIVE);
            return;
        }

        if (!alive)
        {
            SetIcons(slot, false);
            m_RowStatusTexts.Get(slot).SetText("#STR_PARTY_HUD_DEAD");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_CRITICAL);
            return;
        }

        SetIcons(slot, true);

        //--- The level arrives already decided by vanilla on the server, so there is nothing to
        //--- bucket here - it selects the artwork and the tint directly.
        m_RowHealthIcons.Get(slot).SetImage(health_level);
        m_RowHealthIcons.Get(slot).SetColor(LevelColor(health_level));

        int blood_level = rpc.state_blood_level.Get(index);
        m_RowBloodIcons.Get(slot).SetImage(blood_level);
        m_RowBloodIcons.Get(slot).SetColor(LevelColor(blood_level));

        //--- The name follows the health bucket too, so the row still reads at a glance when the
        //--- icons are too small to pick out individually.
        int color = COLOR_HEALTHY;
        if (health_level >= LEVEL_LOW)
            color = COLOR_CRITICAL;
        else if (health_level == LEVEL_MEDIUM)
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
