#ifdef SERVER
#ifdef JM_COT
modded class BRMasterControlsModule
{
    //--- Per-uid earliest next status reply, so a client cannot turn a 1 Hz poll into a busy loop
    //--- that rebuilds the snapshot every frame. Same shape as BattleRoyaleLeaderboard's request
    //--- cooldown and for the same reason: the request is cheap to send and not free to answer.
    protected ref map<string, int> m_StatusCooldown = new map<string, int>();

    //--- The roster is answered on its own clock, so it needs its own cooldown - sharing the status
    //--- one would let whichever poll landed first silently starve the other.
    protected ref map<string, int> m_RosterCooldown = new map<string, int>();

    //--- Party id -> match-local index, assigned first-seen and kept for the process. See
    //--- ResolvePartyIndex for why it is not derived per reply.
    protected ref map<string, int> m_PartyIndex = new map<string, int>();

#ifdef CF_BUGFIX_REF
    override void OnRPC( PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx )
#else
    override void OnRPC( PlayerIdentity sender, Object target, int rpc_type, ref ParamsReadContext ctx )
#endif
    {
        switch ( rpc_type )
        {
            case BattleRoyaleCOTStateMachineRPC.Next:
                RPC_Next( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.Pause:
                RPC_Pause( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.Resume:
                RPC_Resume( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.SpawnAirdrop:
                RPC_SpawnAirdrop( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.AdminAction:
                RPC_AdminAction( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.StatusRequest:
                RPC_StatusRequest( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.RosterRequest:
                RPC_RosterRequest( ctx, sender, target );
            break;
            case BattleRoyaleCOTStateMachineRPC.ZoneRequest:
                RPC_ZoneRequest( ctx, sender, target );
            break;
        }
    }

    /**
     *  The single authorization gate for everything this module does.
     *
     *  ⚠️ THE COT PERMISSION ALONE IS NOT PROTECTION AND NEVER WAS. These RPCs are reachable by any
     *  connected client that sends the right ScriptRPC id; the permission COT checks client-side only
     *  decides whether the control is drawn. So the check has to happen here, on every action, with
     *  `sender` - which the engine supplies and a client cannot forge, unlike the `Object target`
     *  these handlers are careful to ignore.
     *
     *  Two ways to pass, and both are deliberate:
     *
     *  - **admins_steamid64** - the always-allowed floor. It stays because that list is
     *    MISSION-LOCKED by design (BattleRoyaleGameData.LoadMission snapshots it before the mission
     *    deserialize and restores it after), making it the one authorization that content cannot
     *    grant itself. Replacing it with roles alone would mean a COT misconfiguration can lock an
     *    owner out of their own server mid-match.
     *  - **a named COT permission** - so an operator can hand a moderator role the lobby controls
     *    without handing over the whole state machine. This is the half that did not exist: the old
     *    "BattleRoyale.StateMachine.Update" was registered and never read anywhere.
     *
     *  `out instance` is filled on success and is what the webhook posts attribute the action to.
     */
    protected bool AuthorizeAdminAction(PlayerIdentity sender, string permission, string action_name, out JMPlayerInstance instance)
    {
        instance = NULL;

        //--- Asked first and unconditionally, because it is also how the webhook learns who acted.
        //--- A permissions manager that has no record of this identity leaves it NULL, which
        //--- SendWebhook already treats as "no admin account to name".
        bool has_permission = GetPermissionsManager().HasPermission( permission, sender, instance );

        if ( BattleRoyaleServer.IsAdminIdentity( sender ) )
            return true;

        if ( has_permission )
            return true;

        BattleRoyaleUtils.Warn("[DayZBR COT] Rejected unauthorized " + action_name + " request from " + BattleRoyaleServer.GetIdentityLogName(sender) + " (needs " + permission + " or admins_steamid64)");
        return false;
    }

    //--- Attribute an action to the admin who took it. A no-op when no webhook connection is
    //--- configured for the type, and inert offline, so this costs nothing on a server that does not
    //--- use it. Kept as one helper so no call site has to remember the type strings.
    protected void ReportAdminAction(JMPlayerInstance instance, string type, string message)
    {
        SendWebhook( type, instance, message );
    }

    protected void RPC_Next( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_MATCH_CONTROL, "Next", instance ) )
            return;

        string state_name = "<none>";
        BattleRoyaleServer br_server;
        if ( Class.CastTo( br_server, GetBR() ) && br_server.GetCurrentState() )
            state_name = br_server.GetCurrentState().GetName();

        Server_Next();
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Skipped state: " + state_name );
    }

    protected void RPC_Pause( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_MATCH_CONTROL, "Pause", instance ) )
            return;

        Server_Pause();
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Paused the state machine." );
    }

    protected void RPC_Resume( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_MATCH_CONTROL, "Resume", instance ) )
            return;

        Server_Resume();
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Resumed the state machine." );
    }

    protected void RPC_SpawnAirdrop( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_MATCH_CONTROL, "SpawnAirdrop", instance ) )
            return;

        //--- ⚠️ Not defensive padding. IsAdminIdentity establishes that `senderRPC` is non-null; it
        //--- says nothing about whether that connection still controls an entity. An admin who is
        //--- spectating has had SelectPlayer(identity, NULL) called on them, so GetPlayer() is
        //--- exactly the case that arises in practice rather than a theoretical one.
        Man sender_man = senderRPC.GetPlayer();
        if ( !sender_man )
        {
            BattleRoyaleUtils.Warn("[DayZBR COT] SpawnAirdrop from " + BattleRoyaleServer.GetIdentityLogName(senderRPC) + " ignored: they control no entity (spectating?).");
            return;
        }

#ifdef EXPANSIONMODMISSIONS
		//--- StringLocaliser takes a BARE stringtable key with no leading '#' (Expansion's own
		//--- call sites do the same) - a sentence here reaches the player untranslated.
		ExpansionNotification(new StringLocaliser( DAYZBR_MSG_TITLE ), new StringLocaliser( "STR_BR_COT_AIRDROP_SENT" ), DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, DAYZBR_MSG_TIME).Create();
		ExpansionMissionModule.s_Instance.CallAirdrop(sender_man.GetPosition());
		ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Called an airdrop." );
#endif
    }

    /**
     *  Everything beyond the four legacy ids, dispatched on the BattleRoyaleAdminAction in the
     *  payload. Reads are ordered and must match SendAdminAction exactly.
     */
    protected void RPC_AdminAction( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        int action;
        int arg_i;
        float arg_f;
        string arg_uid;
        string arg_text;

        //--- Read the whole payload before doing anything with it. A short read leaves the context
        //--- half-consumed, and there is nothing useful to do with a truncated action.
        //---
        //--- ⚠️ Order here must match SendAdminAction exactly. The two string slots have FIXED roles -
        //--- arg_uid always names the subject, arg_text is always free text - rather than being
        //--- reused per action, so a handler can never disagree with the sender about which is which.
        if ( !ctx.Read( action ) )
            return;
        if ( !ctx.Read( arg_i ) )
            return;
        if ( !ctx.Read( arg_f ) )
            return;
        if ( !ctx.Read( arg_uid ) )
            return;
        if ( !ctx.Read( arg_text ) )
            return;

        switch ( action )
        {
            case BattleRoyaleAdminAction.LOBBY_SET_HOLD:
                Action_LobbySetHold( senderRPC, arg_i != 0 );
            break;
            case BattleRoyaleAdminAction.LOBBY_START_NOW:
                Action_LobbyStartNow( senderRPC );
            break;
            case BattleRoyaleAdminAction.PLAYER_READY:
                Action_PlayerReady( senderRPC, arg_uid );
            break;
            case BattleRoyaleAdminAction.PLAYER_READY_ALL:
                Action_PlayerReadyAll( senderRPC );
            break;
            case BattleRoyaleAdminAction.PLAYER_REMOVE:
                Action_PlayerRemove( senderRPC, arg_uid );
            break;
            case BattleRoyaleAdminAction.PLAYER_ADD:
                Action_PlayerAdd( senderRPC, arg_uid );
            break;
            case BattleRoyaleAdminAction.PLAYER_UNSTUCK:
                Action_PlayerUnstuck( senderRPC, arg_uid );
            break;
            case BattleRoyaleAdminAction.PLAYER_TP_CIRCLE:
                Action_PlayerTeleportCircle( senderRPC, arg_uid );
            break;
            case BattleRoyaleAdminAction.PLAYER_EXEMPT_LATEJOIN:
                Action_PlayerExemptLateJoin( senderRPC, arg_uid );
            break;
            case BattleRoyaleAdminAction.ANNOUNCE_ALL:
                Action_Announce( senderRPC, "", arg_text, arg_f );
            break;
            case BattleRoyaleAdminAction.ANNOUNCE_PLAYER:
                Action_Announce( senderRPC, arg_uid, arg_text, arg_f );
            break;
            case BattleRoyaleAdminAction.ZONE_LOCK_NOW:
                Action_ZoneLockNow( senderRPC );
            break;
            case BattleRoyaleAdminAction.ZONE_SELFTEST:
                Action_ZoneSelfTest( senderRPC, arg_i );
            break;
            default:
                BattleRoyaleUtils.Warn("[DayZBR COT] Unknown admin action " + action + " from " + BattleRoyaleServer.GetIdentityLogName(senderRPC));
            break;
        }
    }

    /**
     *  Resolve the subject of a player action.
     *
     *  ⚠️ This is the ONLY place a client-supplied uid becomes a PlayerBase, and it is deliberately
     *  a lookup over the live population rather than anything the client could influence further.
     *  The uid names WHO IS ACTED ON; who is ACTING is always `sender`, which the engine supplies
     *  and a client cannot forge. Conflating the two is how an admin panel becomes an exploit.
     */
    protected PlayerBase ResolveSubject( PlayerIdentity senderRPC, string uid, string action_name )
    {
        if ( uid == "" )
        {
            BattleRoyaleUtils.Warn("[Admin] " + action_name + " from " + BattleRoyaleServer.GetIdentityLogName(senderRPC) + " named no player.");
            return NULL;
        }

        PlayerBase subject = BattleRoyaleKillAttribution.FindPlayerByUid( uid );
        if ( !subject )
            BattleRoyaleUtils.Warn("[Admin] " + action_name + ": no connected player with uid " + uid + ".");

        return subject;
    }

    //--- Name for a log line or a webhook. Prefers the mod's corrected name over whatever the player
    //--- connected as - BattleRoyaleNameService exists precisely because a placeholder `Survivor`
    //--- identifies nobody.
    protected string SubjectName( PlayerBase subject )
    {
        if ( !subject )
            return "<unknown>";
        if ( subject.player_name != "" )
            return subject.player_name;

        return "<unnamed>";
    }

    protected void Action_PlayerReady( PlayerIdentity senderRPC, string uid )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerReady", instance ) )
            return;

        BattleRoyaleDebug lobby = GetLobbyState();
        if ( !lobby )
        {
            BattleRoyaleUtils.Warn("[Admin] force-ready ignored: the match is no longer in the lobby.");
            return;
        }

        PlayerBase subject = ResolveSubject( senderRPC, uid, "PlayerReady" );
        if ( !subject )
            return;

        lobby.ReadyUp( subject );
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Force-readied " + SubjectName( subject ) + "." );
    }

    protected void Action_PlayerReadyAll( PlayerIdentity senderRPC )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerReadyAll", instance ) )
            return;

        BattleRoyaleDebug lobby = GetLobbyState();
        if ( !lobby )
        {
            BattleRoyaleUtils.Warn("[Admin] force-ready-all ignored: the match is no longer in the lobby.");
            return;
        }

        //--- Snapshot the roster before touching it. ReadyUp messages every player, and iterating the
        //--- live array while that happens is the kind of thing that only bites on a full server.
        array<PlayerBase> players = lobby.GetPlayers();
        int readied = 0;
        for ( int i = 0; i < players.Count(); i++ )
        {
            PlayerBase player = players.Get( i );
            if ( !player )
                continue;

            lobby.ReadyUp( player );
            readied++;
        }

        BattleRoyaleUtils.Info("[Admin] force-readied " + readied + " lobby player(s).");
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Force-readied every lobby player (" + readied + ")." );
    }

    protected void Action_PlayerRemove( PlayerIdentity senderRPC, string uid )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerRemove", instance ) )
            return;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return;

        PlayerBase subject = ResolveSubject( senderRPC, uid, "PlayerRemove" );
        if ( !subject )
            return;

        if ( !state.ContainsPlayer( subject ) )
        {
            BattleRoyaleUtils.Warn("[Admin] remove-from-match: " + SubjectName( subject ) + " is not on the current state's roster.");
            return;
        }

        //--- ⚠️ Removing from the roster is NOT a disconnect, and that is the point of the action -
        //--- but it does make them a player the current state no longer holds, which is precisely what
        //--- OnPlayerTick's not-in-state branch schedules a late-join kick for. Exempt them, or
        //--- "remove from match" silently becomes "kick in 15 seconds".
        br_server.ExemptFromLateJoinKick( uid );

        //--- ⚠️ RemovePlayer IS NOT A NEUTRAL ROSTER EDIT. It is the mod's single leaderboard
        //--- recording point: it calls BattleRoyaleLeaderboard.RecordExit and
        //--- BattleRoyaleMatchStats.RecordExit, booking this player a finishing placement. Both
        //--- dedupe by uid and latch, so a later Add + Remove cannot double-score - but equally,
        //--- PLAYER_ADD CANNOT UNDO THE RECORDING. Mid-match this is arguably right (they are out of
        //--- the match), and in the lobby it is harmless because neither recorder is armed until the
        //--- match begins. It is called out because "remove" reads like something reversible and the
        //--- ladder half of it is not.
        state.RemovePlayer( subject );

        BattleRoyaleUtils.Info("[Admin] removed " + SubjectName( subject ) + " from the match roster (still connected).");
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Removed " + SubjectName( subject ) + " from the match (not disconnected)." );
    }

    /**
     *  Put a removed player back on the roster.
     *
     *  The undo for a mis-clicked Remove, which otherwise had none - once off the roster a player was
     *  stuck outside the match with no way back short of a server restart.
     *
     *  ⚠️ It restores the ROSTER only. If the match had already begun, RemovePlayer will have
     *  recorded their exit on the leaderboard and in the match summary, and both recorders latch by
     *  uid - so their placement stands and this cannot revoke it. In the lobby, where neither is
     *  armed yet, the undo is complete. The log line says which case applied rather than leaving the
     *  operator to guess.
     */
    protected void Action_PlayerAdd( PlayerIdentity senderRPC, string uid )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerAdd", instance ) )
            return;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return;

        PlayerBase subject = ResolveSubject( senderRPC, uid, "PlayerAdd" );
        if ( !subject )
            return;

        if ( state.ContainsPlayer( subject ) )
        {
            BattleRoyaleUtils.Warn("[Admin] add-to-match: " + SubjectName( subject ) + " is already on the roster.");
            return;
        }

        state.AddPlayer( subject );

        //--- Said out loud, because the two cases differ in a way the operator cannot see. Asked of
        //--- the match summary rather than the leaderboard because the leaderboard is additionally
        //--- gated on enable_leaderboard, so it would answer "no" on a server that simply has the
        //--- ladder switched off.
        BattleRoyaleMatchStats stats = BattleRoyaleMatchStats.GetInstance();
        if ( stats && stats.IsRecording() )
            BattleRoyaleUtils.Warn("[Admin] added " + SubjectName( subject ) + " back to the match - but their exit was already recorded and that cannot be undone.");
        else
            BattleRoyaleUtils.Info("[Admin] added " + SubjectName( subject ) + " back to the match roster.");

        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Added " + SubjectName( subject ) + " back to the match." );
    }

    protected void Action_PlayerUnstuck( PlayerIdentity senderRPC, string uid )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerUnstuck", instance ) )
            return;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return;

        PlayerBase subject = ResolveSubject( senderRPC, uid, "PlayerUnstuck" );
        if ( !subject )
            return;

        //--- Deliberately bypasses both the per-player cooldown and AllowsUnstuck(). A player who has
        //--- just burned their own F2 on a bad spot is exactly who needs an admin to do it for them,
        //--- and the phase gate exists to stop players self-teleporting mid-fight, not to stop an
        //--- operator recovering somebody.
        subject.wait_unstuck = false;
        subject.next_unstuck_time = 0;
        state.DeferredUnstuck( subject );

        BattleRoyaleUtils.Info("[Admin] unstuck " + SubjectName( subject ) + ".");
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Unstuck " + SubjectName( subject ) + "." );
    }

    protected void Action_PlayerTeleportCircle( PlayerIdentity senderRPC, string uid )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerTeleportCircle", instance ) )
            return;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return;

        PlayerBase subject = ResolveSubject( senderRPC, uid, "PlayerTeleportCircle" );
        if ( !subject )
            return;

        //--- Resolves the circle actually in play - not the shrink target - and falls back to the
        //--- lobby centre before one is live. Same helper the mid-match admin join uses.
        vector destination;
        br_server.GetAdminSpawnPosition( destination );

        //--- BR_DiagTeleport is NOT #ifdef-guarded: it is compiled into every build and only its diag
        //--- callers are gated. It goes through BR_SYNC_JUNCTURE_TELEPORT, which is what makes the
        //--- client stop predicting the old command instead of sliding back.
        state.BR_DiagTeleport( subject, destination );

        BattleRoyaleUtils.Info("[Admin] teleported " + SubjectName( subject ) + " to the live circle.");
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Teleported " + SubjectName( subject ) + " to the circle in play." );
    }

    protected void Action_PlayerExemptLateJoin( PlayerIdentity senderRPC, string uid )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "PlayerExemptLateJoin", instance ) )
            return;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        if ( uid == "" )
            return;

        if ( !br_server.BR_CancelLateJoinKick( uid ) )
        {
            BattleRoyaleUtils.Warn("[Admin] cancel late-join kick: no connected player with uid " + uid + ".");
            return;
        }

        BattleRoyaleUtils.Info("[Admin] cancelled the late-join kick for " + uid + ".");
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_PLAYER, "Cancelled the late-join kick for uid " + uid + "." );
    }

    /**
     *  Broadcast, or whisper to one player.
     *
     *  ⚠️ The message goes out through MessagePlayers*UNTRANSLATED*, which is the variant that ships
     *  its payload as-is instead of as a stringtable key. That is correct for admin text - it is not
     *  a key - but it means the string reaches StringLocaliser.Format() on the client, which
     *  substitutes %1..%5. Admin text containing a percent-token would therefore mangle itself or
     *  swallow the rest of the line, so those are stripped here rather than trusted.
     */
    protected void Action_Announce( PlayerIdentity senderRPC, string uid, string text, float seconds )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_ANNOUNCE, "Announce", instance ) )
            return;

        string message = SanitizeAnnouncement( text );
        if ( message == "" )
        {
            BattleRoyaleUtils.Warn("[Admin] announcement from " + BattleRoyaleServer.GetIdentityLogName(senderRPC) + " was empty after sanitizing.");
            return;
        }

        //--- Clamped server-side. The panel clamps too, but a client that sends the RPC directly
        //--- never went through the panel.
        float duration = seconds;
        if ( duration < BR_ANNOUNCE_MIN_SECONDS )
            duration = BR_ANNOUNCE_MIN_SECONDS;
        if ( duration > BR_ANNOUNCE_MAX_SECONDS )
            duration = BR_ANNOUNCE_MAX_SECONDS;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return;

        if ( uid == "" )
        {
            state.MessagePlayersUntranslatedTimed( message, duration );
            BattleRoyaleUtils.Info("[Admin] announced to everyone: " + message);
            ReportAdminAction( instance, BR_WEBHOOK_TYPE_ANNOUNCE, "Announced to everyone: " + message );
            return;
        }

        PlayerBase subject = ResolveSubject( senderRPC, uid, "Announce" );
        if ( !subject )
            return;

        state.MessagePlayerUntranslatedTimed( subject, message, duration );
        BattleRoyaleUtils.Info("[Admin] whispered to " + SubjectName( subject ) + ": " + message);
        ReportAdminAction( instance, BR_WEBHOOK_TYPE_ANNOUNCE, "Whispered to " + SubjectName( subject ) + ": " + message );
    }

    //--- Strip what the notification path would misread, and bound the length. Percent signs go
    //--- because StringLocaliser.Format() consumes %1..%5; newlines go because the notification is a
    //--- single-line detail field.
    protected string SanitizeAnnouncement( string text )
    {
        string cleaned = text;

        //--- Replace mutates in place and returns a count; Trim does NOT - it returns a new string,
        //--- so `cleaned.Trim();` on its own is a silent no-op. TrimInPlace is the mutating one.
        cleaned.Replace( "%", " " );
        cleaned.Replace( "\n", " " );
        cleaned.Replace( "\r", " " );
        cleaned.TrimInPlace();

        //--- ⚠️ The Utf8 variants, not Length()/Substring(). Those count BYTES, so a cap applied to
        //--- an accented message would cut mid-character and hand the client a broken sequence -
        //--- and every language this mod ships a stringtable for has accents.
        if ( cleaned.LengthUtf8() > BR_ANNOUNCE_MAX_CHARS )
            cleaned = cleaned.SubstringUtf8( 0, BR_ANNOUNCE_MAX_CHARS );

        return cleaned;
    }

    protected void Action_ZoneLockNow( PlayerIdentity senderRPC )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_ZONE_CONTROL, "ZoneLockNow", instance ) )
            return;

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return;

        //--- ⚠️ Two casts, because IsLocked/LockNewZone are not on the base and the two states that
        //--- have them are SIBLINGS - BattleRoyaleLastRound extends BattleRoyaleState directly, not
        //--- BattleRoyaleRound.
        BattleRoyaleRound round;
        if ( Class.CastTo( round, state ) )
        {
            if ( round.IsLocked() )
            {
                BattleRoyaleUtils.Warn("[Admin] lock-now ignored: this circle is already locked.");
                return;
            }

            //--- 0 seconds of remaining travel window: lock it as of now.
            round.LockNewZone( 0 );
            BattleRoyaleUtils.Info("[Admin] locked the incoming circle early.");
            ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Locked the incoming circle early." );
            return;
        }

        BattleRoyaleLastRound last_round;
        if ( Class.CastTo( last_round, state ) )
        {
            if ( last_round.IsLocked() )
            {
                BattleRoyaleUtils.Warn("[Admin] lock-now ignored: the final circle is already locked.");
                return;
            }

            last_round.LockFinalZone();
            BattleRoyaleUtils.Info("[Admin] locked the final circle early.");
            ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Locked the final circle early." );
            return;
        }

        BattleRoyaleUtils.Warn("[Admin] lock-now ignored: no round is in play.");
    }

    protected void Action_ZoneSelfTest( PlayerIdentity senderRPC, int runs )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_ZONE_CONTROL, "ZoneSelfTest", instance ) )
            return;

        int iterations = runs;
        if ( iterations <= 0 )
            iterations = BR_ADMIN_SELFTEST_RUNS;
        if ( iterations > BR_ADMIN_SELFTEST_RUNS )
            iterations = BR_ADMIN_SELFTEST_RUNS;

        BattleRoyaleZone zone = BattleRoyaleZone.GetZone( 1 );
        if ( !zone )
        {
            BattleRoyaleUtils.Warn("[Admin] zone self test: no zone to run it on.");
            return;
        }

        //--- ⚠️ SYNCHRONOUS, and on a live server with players connected. RunSelfTest restores
        //--- s_ChainRadii and reseeds afterwards so it cannot corrupt the match, but it does block
        //--- the main thread for roughly 5-9 ms per generation - which is why the iteration count is
        //--- capped far below the 200 the boot-time gate uses.
        BattleRoyaleUtils.Info("[Admin] running the zone generator self test, " + iterations + " iteration(s)...");
        zone.RunSelfTest( iterations );

        ReportAdminAction( instance, BR_WEBHOOK_TYPE_MATCH, "Ran the zone generator self test (" + iterations + " iterations) - see the server log." );
    }

    //--- Resolve the lobby state, or NULL when the match has moved past it. Every lobby action needs
    //--- this and every one of them has to cope with the answer being NULL: the panel's controls are
    //--- drawn from a status snapshot that is up to BR_ADMIN_STATUS_INTERVAL_MS old, so an admin can
    //--- legitimately press "Start Match Now" a moment after the lobby closed on its own.
    protected BattleRoyaleDebug GetLobbyState()
    {
        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return NULL;

        BattleRoyaleDebug lobby;
        if ( !Class.CastTo( lobby, br_server.GetCurrentState() ) )
            return NULL;

        return lobby;
    }

    protected void Action_LobbySetHold( PlayerIdentity senderRPC, bool held )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_LOBBY_CONTROL, "LobbySetHold", instance ) )
            return;

        BattleRoyaleDebug lobby = GetLobbyState();
        if ( !lobby )
        {
            BattleRoyaleUtils.Warn("[Admin] lobby hold ignored: the match is no longer in the lobby.");
            return;
        }

        lobby.BR_SetManualStart( held );

        if ( held )
            ReportAdminAction( instance, BR_WEBHOOK_TYPE_LOBBY, "Held the lobby - it will not start on its own." );
        else
            ReportAdminAction( instance, BR_WEBHOOK_TYPE_LOBBY, "Released the lobby - the ordinary start gates apply again." );
    }

    protected void Action_LobbyStartNow( PlayerIdentity senderRPC )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_LOBBY_CONTROL, "LobbyStartNow", instance ) )
            return;

        BattleRoyaleDebug lobby = GetLobbyState();
        if ( !lobby )
        {
            BattleRoyaleUtils.Warn("[Admin] manual start ignored: the match is no longer in the lobby.");
            return;
        }

        //--- BR_AdminStartMatch logs its own refusal reason, which is the part an admin needs. Only
        //--- an accepted start is worth a webhook - a refused one changed nothing.
        if ( !lobby.BR_AdminStartMatch() )
            return;

        ReportAdminAction( instance, BR_WEBHOOK_TYPE_LOBBY, "Started the match manually." );
    }

    /**
     *  Answer one admin's status poll.
     *
     *  ⚠️ PER IDENTITY, NEVER BROADCAST. The payload is aggregate today and carries nothing
     *  identifying, but the addressing is what keeps that true as fields get added - and it is what
     *  lets the reply be gated on a permission at all.
     */
    protected void RPC_StatusRequest( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_VIEW, "StatusRequest", instance ) )
            return;

        string uid = senderRPC.GetPlainId();
        if ( uid == "" )
            return;

        int now = GetGame().GetTime();
        int next_allowed;
        if ( m_StatusCooldown.Find( uid, next_allowed ) && now < next_allowed )
            return;

        //--- Deliberately a fraction of the client's poll interval rather than the whole of it: the
        //--- two clocks are unrelated, so a cooldown equal to the interval would drop roughly every
        //--- other poll through pure phase drift and the readout would update at half the rate it
        //--- looks like it should.
        m_StatusCooldown.Set( uid, now + (BR_ADMIN_STATUS_INTERVAL_MS / 2) );

        BattleRoyaleAdminStatus status = BuildStatus();
        if ( !status )
            return;

        string payload;
        JsonSerializer serializer = new JsonSerializer();
        if ( !serializer.WriteToString( status, false, payload ) )
        {
            BattleRoyaleUtils.Warn("[Admin] status could not be serialized.");
            return;
        }

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write( payload );
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.StatusReply, true, senderRPC );
    }

    /**
     *  Answer one admin's roster poll. Same authorization and cooldown shape as the status poll.
     *
     *  ⚠️ THIS PAYLOAD CARRIES SteamID64s, so it is per-identity and gated on Player.Manage rather
     *  than on the View permission the status readout uses. A moderator who may look at the match is
     *  not automatically somebody who should be handed the population's Steam IDs.
     */
    protected void RPC_RosterRequest( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_PLAYER_MANAGE, "RosterRequest", instance ) )
            return;

        string uid = senderRPC.GetPlainId();
        if ( uid == "" )
            return;

        int now = GetGame().GetTime();
        int next_allowed;
        if ( m_RosterCooldown.Find( uid, next_allowed ) && now < next_allowed )
            return;

        m_RosterCooldown.Set( uid, now + (BR_ADMIN_ROSTER_INTERVAL_MS / 2) );

        BattleRoyaleAdminRoster roster = BuildRoster();
        if ( !roster )
            return;

        string payload;
        JsonSerializer serializer = new JsonSerializer();
        if ( !serializer.WriteToString( roster, false, payload ) )
        {
            BattleRoyaleUtils.Warn("[Admin] roster could not be serialized.");
            return;
        }

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write( payload );
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.RosterReply, true, senderRPC );
    }

    /**
     *  Every connected player, with whatever the current phase knows about them.
     *
     *  Walks the CONNECTED POPULATION rather than the current state's roster, which is deliberate:
     *  an admin who joined mid-match, a spectator and a late joiner on a kick countdown are all
     *  connected and none of them is in a state. Those are exactly the people an operator needs to
     *  see - a roster that only listed state members would hide the player who is about to be
     *  disconnected.
     */
    protected BattleRoyaleAdminRoster BuildRoster()
    {
        BattleRoyaleAdminRoster roster = new BattleRoyaleAdminRoster();

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return roster;

        BattleRoyaleState state = br_server.GetCurrentState();
        BattleRoyaleDebug lobby = GetLobbyState();

        //--- The scoreboard columns only mean anything while stats are recording. Asked once rather
        //--- than per row.
        BattleRoyaleMatchStats stats = BattleRoyaleMatchStats.GetInstance();
        map<string, ref BattleRoyaleLastMatchRow> stat_rows = new map<string, ref BattleRoyaleLastMatchRow>();
        if ( stats && stats.IsRecording() )
        {
            roster.scoring = true;

            array<ref BattleRoyaleLastMatchRow> live_rows = stats.GetRows();
            if ( live_rows )
            {
                for ( int r = 0; r < live_rows.Count(); r++ )
                {
                    BattleRoyaleLastMatchRow stat_row = live_rows.Get( r );
                    if ( stat_row && stat_row.uid != "" )
                        stat_rows.Set( stat_row.uid, stat_row );
                }
            }
        }

        array<Man> population = new array<Man>();
        GetGame().GetPlayers( population );

        for ( int i = 0; i < population.Count(); i++ )
        {
            if ( roster.rows.Count() >= BR_ADMIN_ROSTER_MAX_ROWS )
            {
                roster.truncated = true;
                break;
            }

            PlayerBase player = PlayerBase.Cast( population[i] );
            if ( !player )
                continue;

            string player_uid = player.player_steamid;
            if ( player_uid == "" )
                continue;

            BattleRoyaleAdminRosterRow row = new BattleRoyaleAdminRosterRow();
            row.uid = player_uid;
            row.name = SubjectName( player );

            if ( state )
                row.in_state = state.ContainsPlayer( player );

            row.alive = row.in_state && player.IsAlive();
            row.spectating = BattleRoyaleSpectators.GetInstance().IsSpectator( player_uid );
            row.loaded = player.br_loaded_in;
            row.group = ResolvePartyIndex( player );
            row.late_join_seconds = br_server.BR_GetLateJoinSecondsLeft( player_uid );

            if ( lobby )
                row.ready = lobby.BR_IsReady( player );

            //--- br_kills is the authoritative per-player counter and is maintained outside the match
            //--- stats, so it is right even before stats begin recording.
            row.kills = player.br_kills;

            BattleRoyaleLastMatchRow stat_entry;
            if ( stat_rows.Find( player_uid, stat_entry ) && stat_entry )
            {
                row.damage = stat_entry.damage;
                row.hits = stat_entry.hits;
                row.place = stat_entry.place;
                row.survived_s = stat_entry.survived_s;
            }

            roster.rows.Insert( row );
        }

        roster.valid = true;
        return roster;
    }

    /**
     *  A stable, match-local index for the player's party.
     *
     *  ⚠️ Keyed on GetPartyId() into a map that persists for the process, NOT on a position in
     *  GetGroups(), which re-partitions the living population on every call - a team's index would
     *  change whenever somebody in an earlier group died, and the roster would appear to reshuffle
     *  itself between polls. Same reasoning, and the same failure, as BattleRoyaleTeamColour's
     *  party index (#276).
     *
     *  Answers the unknown sentinel when the party manager cannot speak, so the panel omits the
     *  column rather than printing an index that means nothing.
     */
    protected int ResolvePartyIndex( PlayerBase player )
    {
#ifdef VIGRID_PARTY
        if ( !VigridPartyAPI.IsReady() )
            return BR_ADMIN_GROUPS_UNKNOWN;

        string party_id = VigridPartyAPI.GetPartyId( player );
        if ( party_id == "" )
            return BR_ADMIN_GROUPS_UNKNOWN;

        int existing;
        if ( m_PartyIndex.Find( party_id, existing ) )
            return existing;

        int assigned = m_PartyIndex.Count();
        m_PartyIndex.Set( party_id, assigned );
        return assigned;
#else
        return BR_ADMIN_GROUPS_UNKNOWN;
#endif
    }

    /**
     *  Answer one admin's zone table request.
     *
     *  Gated on the View permission rather than Zone.Control: this is read-only, carries no uids and
     *  nothing an ordinary player could not eventually infer from their own map.
     */
    protected void RPC_ZoneRequest( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        JMPlayerInstance instance;
        if ( !AuthorizeAdminAction( senderRPC, BR_PERM_VIEW, "ZoneRequest", instance ) )
            return;

        BattleRoyaleAdminZoneTable table = BuildZoneTable();
        if ( !table )
            return;

        string payload;
        JsonSerializer serializer = new JsonSerializer();
        if ( !serializer.WriteToString( table, false, payload ) )
        {
            BattleRoyaleUtils.Warn("[Admin] zone table could not be serialized.");
            return;
        }

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write( payload );
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.ZoneReply, true, senderRPC );
    }

    /**
     *  The generated circle chain, in PLAY order.
     *
     *  ⚠️ m_PlayAreas is built SMALLEST FIRST - index 0 is the tight final circle and each later
     *  index is a larger circle containing it - so walking it forwards shows the match backwards.
     *  This is the single most surprising fact in the zone subsystem and the reason the loop counts
     *  down. settings_index carries the original index so a row can still be matched against
     *  zone_settings.json.
     */
    protected BattleRoyaleAdminZoneTable BuildZoneTable()
    {
        BattleRoyaleAdminZoneTable table = new BattleRoyaleAdminZoneTable();

        if ( !BattleRoyaleZone.m_PlayAreas )
            return table;

        BattleRoyaleZoneData zone_settings = BattleRoyaleConfig.GetConfig().GetZoneData();
        if ( !zone_settings )
            return table;

        table.generation_seed = BattleRoyaleZone.s_GenerationSeed;
        table.num_zones = zone_settings.num_zones;
        table.derive_timers = zone_settings.derive_timers_from_geometry;
        table.allow_flex = zone_settings.allow_zone_size_flex;

        BattleRoyaleZone zone = BattleRoyaleZone.GetZone( 1 );

        //--- What is being played right now, so the table can mark it. Read off the announced
        //--- statics rather than re-derived, so it agrees with every client's map by construction.
        float live_radius = BattleRoyaleState.GetAnnouncedCurrentRadius();

        int count = BattleRoyaleZone.m_PlayAreas.Count();
        int order = 0;

        for ( int i = count - 1; i >= 0; i-- )
        {
            BattleRoyalePlayArea area = BattleRoyaleZone.m_PlayAreas.Get( i );
            if ( !area )
                continue;

            order++;

            BattleRoyaleAdminZoneRow row = new BattleRoyaleAdminZoneRow();
            row.settings_index = i;
            row.play_order = order;
            row.radius = area.GetRadius();

            vector center = area.GetCenter();
            row.center_x = center[0];
            row.center_z = center[2];

            //--- Entries past num_zones exist but are never played; so are the largest circles when
            //--- a dynamic starting zone skips them. Marked rather than hidden, so the table still
            //--- lines up with zone_settings.json.
            row.skipped = (i >= zone_settings.num_zones);

            if ( i < zone_settings.static_timers.Count() )
                row.timer_s = zone_settings.static_timers.Get( i );

            if ( BattleRoyaleZone.s_PlayAreaDurationOffsets && i < BattleRoyaleZone.s_PlayAreaDurationOffsets.Count() )
                row.offset_s = BattleRoyaleZone.s_PlayAreaDurationOffsets.Get( i );

            if ( zone )
            {
                row.derived_timer_s = zone.GetDerivedTimer( i );
                row.growth_m = zone.GetRadiusGrowth( i );
            }

            //--- Radius rather than index, because the live circle is identified by what was
            //--- announced and the announcement carries no index.
            row.current = (live_radius > 0 && Math.AbsFloat( live_radius - row.radius ) < 1.0);

            table.rows.Insert( row );
        }

        table.valid = true;
        return table;
    }

    /**
     *  Assemble the snapshot.
     *
     *  Server-level facts here; per-state facts come from BR_FillAdminStatus, which is a virtual so
     *  that the lobby can publish its start gate without exposing a dozen protected accessors to an
     *  unrelated module (see BattleRoyaleDebug's override).
     */
    protected BattleRoyaleAdminStatus BuildStatus()
    {
        BattleRoyaleAdminStatus status = new BattleRoyaleAdminStatus();

        BattleRoyaleServer br_server;
        if ( !Class.CastTo( br_server, GetBR() ) )
            return status;

        status.state_index = br_server.i_CurrentStateIndex;
        if ( br_server.m_States )
            status.state_count = br_server.m_States.Count();

        //--- Everyone on the server, which is deliberately NOT the state's roster: an admin who
        //--- joined mid-match, a spectator and a late joiner awaiting a kick are all connected and
        //--- none of them is in a state. "12 alive of 20 connected" is the reading that makes a
        //--- lobby that will not start legible.
        array<Man> connected = new array<Man>();
        GetGame().GetPlayers( connected );
        status.connected = connected.Count();

        status.spectators = BattleRoyaleSpectators.GetInstance().GetSpectatorCount();

        //--- Statics on the state base, written by SendCurrentPlayArea / SendFuturePlayArea, so they
        //--- are readable whatever state is running and stay correct across a transition. Reached
        //--- through accessors because the fields themselves are `protected` and this module is not
        //--- a state.
        status.current_radius = BattleRoyaleState.GetAnnouncedCurrentRadius();
        status.future_radius = BattleRoyaleState.GetAnnouncedFutureRadius();
        status.generation_seed = BattleRoyaleZone.s_GenerationSeed;

        BattleRoyaleState state = br_server.GetCurrentState();
        if ( !state )
            return status;

        state.BR_FillAdminStatus( status );

        //--- ⚠️ TWO casts, because IsLocked() is not on the base and the two states that have it are
        //--- SIBLINGS: BattleRoyaleLastRound extends BattleRoyaleState directly, not
        //--- BattleRoyaleRound. Same shape as GetAdminJoinPosition's two-cast dance, and for the
        //--- same reason.
        BattleRoyaleRound round;
        if ( Class.CastTo( round, state ) )
            status.zone_locked = round.IsLocked();

        BattleRoyaleLastRound last_round;
        if ( Class.CastTo( last_round, state ) )
            status.zone_locked = last_round.IsLocked();

        return status;
    }

    //server-side functionality
    protected void Server_Resume()
    {
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            BattleRoyaleUtils.Trace("[DayZBR COT] State Machine Resuming!");
            m_BrServer.GetCurrentState().Resume();// allow super.IsComplete() to return TRUE again
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
    }

    protected void Server_Pause()
    {
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            BattleRoyaleUtils.Trace("[DayZBR COT] State Machine Pausing!");
            m_BrServer.GetCurrentState().Pause(); // super.IsComplete() will return FALSE until Resume is called
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
    }

    protected void Server_Next()
    {
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            BattleRoyaleUtils.Trace("[DayZBR COT] State Machine Skipping!");
            m_BrServer.GetCurrentState().Deactivate();// super.IsComplete() will return TRUE when this is run
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
    }
}
#endif // JM_COT
#endif
