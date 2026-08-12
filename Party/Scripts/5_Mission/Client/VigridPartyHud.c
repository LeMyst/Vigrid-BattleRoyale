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
 *  Two lines per member, not three: the badges share the second line with the distance. This is a
 *  HUD element a player reads mid-firefight, so it owes the screen as little space as it can get
 *  away with.
 *
 *  Rows are pooled and positioned manually rather than through a spacer widget, so the layout
 *  behaves identically regardless of how many rows are live - and LayoutRow() shrinks each row to
 *  the text it actually holds. A spacer cannot do that job: it can only collapse to the sum of its
 *  children, and these children are fixed-width boxes, so it has nothing to collapse to and the
 *  background stretches the full declared width. KillFeedUI.LayoutRow solves the same problem the
 *  same way.
 *
 *  Two colour channels, deliberately kept apart. The accent bar down the left edge is the member's
 *  party-slot colour and answers WHO - it matches their pings and their map markers, and it does not
 *  change when they are hurt or offline. The name tint answers HOW BAD. Neither reading has to share
 *  a channel with the other.
 */
class VigridPartyHud
{
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
    private ref array<Widget> m_RowBackdrops;
    private ref array<Widget> m_RowAccents;
    private ref array<TextWidget> m_RowNames;
    private ref array<ImageWidget> m_RowHealthIcons;
    private ref array<ImageWidget> m_RowBloodIcons;
    private ref array<TextWidget> m_RowStatusTexts;
    private ref array<TextWidget> m_RowDistances;

    void VigridPartyHud()
    {
        m_RowWidgets = new array<Widget>();
        m_RowBackdrops = new array<Widget>();
        m_RowAccents = new array<Widget>();
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

    //! Vertical distance between two rows' top edges.
    private int RowPitch()
    {
        return VIGRID_PARTY_HUD_ROW_HEIGHT + VIGRID_PARTY_HUD_ROW_GAP;
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

            row.SetPos(0, m_RowWidgets.Count() * RowPitch());

            ImageWidget health_icon = ImageWidget.Cast(row.FindAnyWidget("RowHealthIcon"));
            ImageWidget blood_icon = ImageWidget.Cast(row.FindAnyWidget("RowBloodIcon"));
            LoadIconFrames(health_icon, "iconHealth");
            LoadIconFrames(blood_icon, "iconBlood");

            m_RowWidgets.Insert(row);
            m_RowBackdrops.Insert(row.FindAnyWidget("RowBackdrop"));
            m_RowAccents.Insert(row.FindAnyWidget("RowAccent"));
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

        //--- Fall back to the camera, not vector.Zero, exactly as the nametags do. There is no local
        //--- player during the ordinary load and teardown windows, nor while the host game has the
        //--- client watching through a detached camera - and measuring every row's distance from the
        //--- map origin prints thousands of metres instead of a useful number.
        vector self_pos = GetGame().GetCurrentCameraPosition();
        PlayerBase local_player = PlayerBase.Cast(GetGame().GetPlayer());
        if (local_player)
            self_pos = local_player.GetPosition();

        //--- ...and the local player is not always the right origin either. The host game can leave
        //--- this client watching somebody else while GetPlayer() still hands back a body that is no
        //--- longer where the client is looking, which is not distinguishable from here - the object
        //--- is valid, it is just stale. Every row would then read its distance from that stale
        //--- point. VigridPartyClientAPI is how the host game says where to measure from instead.
        string viewpoint_uid = "";
        if (VigridPartyClientAPI.HasHudViewpoint())
        {
            self_pos = VigridPartyClientAPI.GetHudViewpointPos();
            viewpoint_uid = VigridPartyClientAPI.GetHudViewpointUid();
        }

        m_Root.Show(true);

        int slot = 0;
        for (int i = 0; i < member_count; i++)
        {
            if (i == rpc.self_index)
                continue;
            if (slot >= m_RowWidgets.Count())
                break;

            RenderRow(slot, i, rpc, have_state, stale, self_pos, viewpoint_uid);
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

    private void RenderRow(int slot, int index, VigridPartyRPC rpc, bool have_state, bool stale, vector self_pos, string viewpoint_uid)
    {
        Widget row = m_RowWidgets.Get(slot);
        row.Show(true);
        row.SetPos(0, slot * RowPitch());

        //--- Dim the whole row when the data behind it has gone quiet.
        //---
        //--- Safe on the root only because the root draws nothing: it is a pure alpha carrier, and
        //--- RowBackdrop holds the fill. A widget's colour alpha and its widget alpha are one value,
        //--- so while the root drew the backdrop itself this line could not exist - the unconditional
        //--- SetAlpha(1.0) it replaced was forcing the fill fully opaque every frame, which is why
        //--- the row rendered pitch black at 0.4, 0.45 and 0.25 alike. Setting the root's colour
        //--- instead only moved the problem: the faint backdrop then took the text down with it.
        //---
        //--- Set here rather than at the end because every branch below returns early: a row that
        //--- dimmed once would otherwise never brighten again.
        if (stale)
            row.SetAlpha(0.5);
        else
            row.SetAlpha(1.0);

        //--- Identity, not condition: the slot colour is the member's for the life of the party, so
        //--- it is set the same way in every branch - offline and dead included.
        Widget accent = m_RowAccents.Get(slot);
        if (accent)
            accent.SetColor(VigridPartyAPI.GetMemberColour(index, 1.0));

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
            LayoutRow(slot, false);
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
            LayoutRow(slot, false);
            return;
        }

        if (!alive)
        {
            SetIcons(slot, false);
            m_RowStatusTexts.Get(slot).SetText("#STR_PARTY_HUD_DEAD");
            m_RowDistances.Get(slot).SetText("");
            m_RowNames.Get(slot).SetColor(COLOR_CRITICAL);
            LayoutRow(slot, false);
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

        //--- This member IS the origin, so their distance from it is zero by construction. Blank
        //--- rather than "0m", which reads as a broken readout rather than as "you are looking at
        //--- them". Everyone else on the panel is then measured from here, which is the useful
        //--- question while watching somebody: how far is the rest of the team from THEM.
        bool is_viewpoint = viewpoint_uid != "" && rpc.roster_uids.Get(index) == viewpoint_uid;

        if (is_viewpoint)
            m_RowDistances.Get(slot).SetText("");
        else if (self_pos != vector.Zero && member_pos != vector.Zero)
            m_RowDistances.Get(slot).SetText(VigridPartyScreen.FormatDistance(vector.Distance(self_pos, member_pos)));
        else
            m_RowDistances.Get(slot).SetText("");

        LayoutRow(slot, true);
    }

    /**
     *  Place both lines and shrink the row to fit the wider of them.
     *
     *  Done in script rather than with a spacer's "Size To Content H": a spacer can only collapse to
     *  the sum of its children, and these children are fixed-width boxes, so it has nothing to
     *  collapse to and the background stretches the full declared width. This is the same problem
     *  KillFeedUI.LayoutRow solves, and the same answer.
     *
     *  GetTextSize reports the currently set text in pixels and is valid in the same frame as
     *  SetText - vanilla measures exactly this way in actiontargetscursor.c:1141-1153.
     *
     *  `icons` mirrors the SetIcons() call the caller just made. It is passed rather than read back
     *  off the widget because IsVisible() also answers for the parent chain, and the second line is
     *  laid out completely differently depending on which of the two occupies it.
     */
    private void LayoutRow(int slot, bool icons)
    {
        Widget row = m_RowWidgets.Get(slot);
        if (!row)
            return;

        int text_w;
        int text_h;
        int name_w = 0;
        int stats_w = 0;

        //--- Update() before every read-back: the row may only just have been shown, and a stale
        //--- layout measures as zero. TabberUI.c:126 and sizetochild.c:35 do the same.
        TextWidget name_text = m_RowNames.Get(slot);
        if (name_text)
        {
            name_text.Update();
            name_text.GetTextSize(text_w, text_h);
            name_text.SetPos(VIGRID_PARTY_HUD_PAD_L, VIGRID_PARTY_HUD_NAME_TOP);
            name_text.SetSize(text_w, VIGRID_PARTY_HUD_NAME_HEIGHT);
            name_w = text_w;
        }

        if (icons)
        {
            m_RowHealthIcons.Get(slot).SetPos(VIGRID_PARTY_HUD_PAD_L, VIGRID_PARTY_HUD_STAT_TOP);
            m_RowBloodIcons.Get(slot).SetPos(VIGRID_PARTY_HUD_PAD_L + VIGRID_PARTY_HUD_ICON + VIGRID_PARTY_HUD_ICON_GAP, VIGRID_PARTY_HUD_STAT_TOP);

            stats_w = (2 * VIGRID_PARTY_HUD_ICON) + VIGRID_PARTY_HUD_ICON_GAP;

            //--- Blank while a teammate's position is unknown, and then the badges are the whole
            //--- line - no trailing gap reserved for a distance that is not there.
            TextWidget distance_text = m_RowDistances.Get(slot);
            if (distance_text)
            {
                //--- Shown BEFORE it is measured. GetTextSize only measures a laid-out widget, so a
                //--- cell hidden on the previous frame would measure zero, be hidden again on the
                //--- strength of that, and never come back - vanilla shows first for the same reason
                //--- (actiontargetscursor.c:1148-1151).
                distance_text.Show(true);
                distance_text.Update();
                distance_text.GetTextSize(text_w, text_h);
                distance_text.Show(text_w > 0);

                if (text_w > 0)
                {
                    distance_text.SetPos(VIGRID_PARTY_HUD_PAD_L + stats_w + VIGRID_PARTY_HUD_STAT_GAP, VIGRID_PARTY_HUD_STAT_TOP);
                    distance_text.SetSize(text_w, VIGRID_PARTY_HUD_STAT_HEIGHT);
                    stats_w = stats_w + VIGRID_PARTY_HUD_STAT_GAP + text_w;
                }
            }
        }
        else
        {
            TextWidget status_text = m_RowStatusTexts.Get(slot);
            if (status_text)
            {
                status_text.Update();
                status_text.GetTextSize(text_w, text_h);
                status_text.SetPos(VIGRID_PARTY_HUD_PAD_L, VIGRID_PARTY_HUD_STAT_TOP);
                status_text.SetSize(text_w, VIGRID_PARTY_HUD_STAT_HEIGHT);
                stats_w = text_w;
            }
        }

        int content_w = name_w;
        if (stats_w > content_w)
            content_w = stats_w;

        int row_w = VIGRID_PARTY_HUD_PAD_L + content_w + VIGRID_PARTY_HUD_PAD_R;
        row.SetSize(row_w, VIGRID_PARTY_HUD_ROW_HEIGHT);

        //--- The backdrop is a sibling of the cells rather than the root itself, so it does not
        //--- follow the root's size and has to be resized alongside it.
        Widget backdrop = m_RowBackdrops.Get(slot);
        if (backdrop)
            backdrop.SetSize(row_w, VIGRID_PARTY_HUD_ROW_HEIGHT);

        Widget accent = m_RowAccents.Get(slot);
        if (accent)
            accent.SetSize(VIGRID_PARTY_HUD_ACCENT_W, VIGRID_PARTY_HUD_ROW_HEIGHT);
    }
}
#endif
