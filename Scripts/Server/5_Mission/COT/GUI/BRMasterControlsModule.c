#ifdef SERVER
#ifdef JM_COT
modded class BRMasterControlsModule
{
    //--- Per-uid earliest next status reply, so a client cannot turn a 1 Hz poll into a busy loop
    //--- that rebuilds the snapshot every frame. Same shape as BattleRoyaleLeaderboard's request
    //--- cooldown and for the same reason: the request is cheap to send and not free to answer.
    protected ref map<string, int> m_StatusCooldown = new map<string, int>();

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

        //--- Read the whole payload before doing anything with it. A short read leaves the context
        //--- half-consumed, and there is nothing useful to do with a truncated action.
        if ( !ctx.Read( action ) )
            return;
        if ( !ctx.Read( arg_i ) )
            return;
        if ( !ctx.Read( arg_f ) )
            return;

        switch ( action )
        {
            case BattleRoyaleAdminAction.LOBBY_SET_HOLD:
                Action_LobbySetHold( senderRPC, arg_i != 0 );
            break;
            case BattleRoyaleAdminAction.LOBBY_START_NOW:
                Action_LobbyStartNow( senderRPC );
            break;
            default:
                BattleRoyaleUtils.Warn("[DayZBR COT] Unknown admin action " + action + " from " + BattleRoyaleServer.GetIdentityLogName(senderRPC));
            break;
        }
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
