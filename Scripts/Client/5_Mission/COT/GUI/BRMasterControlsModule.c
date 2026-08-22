#ifdef JM_COT
class BRMasterControlsModule: JMRenderableModuleBase
{
    //--- The last status the server answered, and a sequence number the form uses to repaint only
    //--- when something actually arrived. Never NULL after the constructor, so the form can render a
    //--- blank-but-valid snapshot before the first reply lands rather than special-casing it.
    protected ref BattleRoyaleAdminStatus m_Status;
    protected int m_StatusSeq;

    protected ref BattleRoyaleAdminRoster m_Roster;
    protected int m_RosterSeq;

    protected ref BattleRoyaleAdminZoneTable m_ZoneTable;
    protected int m_ZoneSeq;

    void BRMasterControlsModule()
    {
        m_Status = new BattleRoyaleAdminStatus();
        m_StatusSeq = 0;
        m_Roster = new BattleRoyaleAdminRoster();
        m_RosterSeq = 0;
        m_ZoneTable = new BattleRoyaleAdminZoneTable();
        m_ZoneSeq = 0;

        //--- ⚠️ Registering a permission is not enforcing one. These decide whether the CLIENT draws
        //--- a control; the server re-checks every one of them in AuthorizeAdminAction, and also
        //--- allows anyone in admins_steamid64 regardless. A client that sends the right ScriptRPC id
        //--- reaches the handler whatever COT thinks, so the client-side check is presentation only.
        //---
        //--- BR_PERM_VIEW keeps its old string ("BattleRoyale.StateMachine.View") so operators who
        //--- have already granted it to a role do not silently lose the panel. The old
        //--- "BattleRoyale.StateMachine.Update" is gone: it was registered and never read anywhere,
        //--- so nothing can depend on it.
        GetPermissionsManager().RegisterPermission( BR_PERM_VIEW );
        GetPermissionsManager().RegisterPermission( BR_PERM_MATCH_CONTROL );
        GetPermissionsManager().RegisterPermission( BR_PERM_LOBBY_CONTROL );
    }

    override bool HasAccess()
    {
        return GetPermissionsManager().HasPermission( BR_PERM_VIEW );
    }

    override string GetLayoutRoot()
    {
        return "Vigrid-BattleRoyale/GUI/layouts/COT/master_controls.layout";
    }

    override string GetTitle()
    {
        //--- Resolved here rather than returned as a bare "#KEY": these strings are handed to
        //--- Community-Online-Tools, and whether it feeds them through a widget SetText (which
        //--- would resolve the key) is not ours to rely on. TranslateString is correct either way.
        return Widget.TranslateString( "#STR_BR_COT_MENUENTRY" );
    }

    override string GetIconName()
    {
        return "BR";
    }

    override bool ImageIsIcon()
    {
        return false;
    }

    override string GetWebhookTitle()
    {
        return Widget.TranslateString( "#STR_BR_COT_WEBHOOKTITLE" );
    }

    //--- One connection per action group, so an operator can subscribe a channel to lobby control
    //--- without also receiving every state-machine skip. COT builds the connection name as
    //--- GetModuleName() + type, which is why these are bare suffixes.
    override void GetWebhookTypes( out array< string > types )
    {
        types.Insert( BR_WEBHOOK_TYPE_MATCH );
        types.Insert( BR_WEBHOOK_TYPE_LOBBY );
        types.Insert( BR_WEBHOOK_TYPE_PLAYER );
        types.Insert( BR_WEBHOOK_TYPE_ANNOUNCE );
    }

    override int GetRPCMin()
    {
        return BattleRoyaleCOTStateMachineRPC.INVALID;
    }

    override int GetRPCMax()
    {
        return BattleRoyaleCOTStateMachineRPC.COUNT;
    }

    //--- What the form renders. Never NULL.
    BattleRoyaleAdminStatus GetStatus()
    {
        return m_Status;
    }

    //--- Bumped on every accepted reply. The form repaints on a change rather than every frame,
    //--- since repainting a UIActionText that has not changed still costs a widget update.
    int GetStatusSeq()
    {
        return m_StatusSeq;
    }

    BattleRoyaleAdminRoster GetRoster()
    {
        return m_Roster;
    }

    int GetRosterSeq()
    {
        return m_RosterSeq;
    }

    BattleRoyaleAdminZoneTable GetZoneTable()
    {
        return m_ZoneTable;
    }

    int GetZoneSeq()
    {
        return m_ZoneSeq;
    }

    //client side functionality (these get called)
#ifndef SERVER
    void StateMachine_Next()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Next, true, NULL );
    }

    void StateMachine_Pause()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Pause, true, NULL );
    }

    void StateMachine_Resume()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Resume, true, NULL );
    }

    void SpawnAirdrop()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.SpawnAirdrop, true, NULL );
    }

    /**
     *  Every state-changing admin action beyond the four legacy ids above.
     *
     *  One wire id carrying the action in its payload, rather than an id per action - the same shape
     *  BRDiagAction uses, and for the same reason: BattleRoyaleCOTStateMachineRPC is append-only
     *  because its values travel on the wire, so a design that needs a new id per feature makes
     *  every feature a wire change.
     */
    //--- ⚠️ The write order here is the contract RPC_AdminAction reads back, and the two string slots
    //--- have FIXED roles rather than being reused per action: arg_uid always names the subject,
    //--- arg_text is always free text. An action that needs neither still sends both as "".
    void SendAdminAction(int action, int arg_i = 0, float arg_f = 0, string arg_uid = "", string arg_text = "")
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Write( action );
        rpc.Write( arg_i );
        rpc.Write( arg_f );
        rpc.Write( arg_uid );
        rpc.Write( arg_text );
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.AdminAction, true, NULL );
    }

    void RequestRoster()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.RosterRequest, true, NULL );
    }

    //--- Asked once when the zone section is first shown, not polled: the chain is generated once per
    //--- process and never changes afterwards. Only `current` moves, and the status readout already
    //--- carries the live radius.
    void RequestZoneTable()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.ZoneRequest, true, NULL );
    }

    //--- Pull, not push: the server has no idea who has the panel open, and a subscription table
    //--- would need a teardown path for every way a client can stop watching (close, disconnect,
    //--- lose permission). Polling makes a closed panel cost exactly nothing.
    void RequestStatus()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.StatusRequest, true, NULL );
    }

#ifdef CF_BUGFIX_REF
    override void OnRPC( PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx )
#else
    override void OnRPC( PlayerIdentity sender, Object target, int rpc_type, ref ParamsReadContext ctx )
#endif
    {
        switch ( rpc_type )
        {
            case BattleRoyaleCOTStateMachineRPC.StatusReply:
                RPC_StatusReply( ctx );
            break;
            case BattleRoyaleCOTStateMachineRPC.RosterReply:
                RPC_RosterReply( ctx );
            break;
            case BattleRoyaleCOTStateMachineRPC.ZoneReply:
                RPC_ZoneReply( ctx );
            break;
        }
    }

    /**
     *  One JSON string in, one status snapshot out.
     *
     *  ⚠️ Deserialized into a FRESH instance and only swapped in on success. Reading straight into
     *  m_Status would let a malformed or truncated payload leave the panel showing a half-updated
     *  snapshot - some fields from this reply, the rest from the last one - which reads as a
     *  plausible match state that never existed. On failure the previous snapshot simply stands.
     */
    protected void RPC_StatusReply( ParamsReadContext ctx )
    {
        string payload;
        if ( !ctx.Read( payload ) )
            return;

        if ( payload == "" )
            return;

        BattleRoyaleAdminStatus incoming = new BattleRoyaleAdminStatus();

        string error;
        JsonSerializer serializer = new JsonSerializer();
        if ( !serializer.ReadFromString( incoming, payload, error ) )
        {
            BattleRoyaleUtils.Warn( "[Admin] status reply could not be parsed: " + error );
            return;
        }

        m_Status = incoming;
        m_StatusSeq++;
    }

    //--- Same deserialize-into-a-fresh-instance-and-swap discipline as the status reply: a truncated
    //--- payload must leave the previous roster standing rather than produce a half-updated one.
    protected void RPC_RosterReply( ParamsReadContext ctx )
    {
        string payload;
        if ( !ctx.Read( payload ) )
            return;
        if ( payload == "" )
            return;

        BattleRoyaleAdminRoster incoming = new BattleRoyaleAdminRoster();

        string error;
        JsonSerializer serializer = new JsonSerializer();
        if ( !serializer.ReadFromString( incoming, payload, error ) )
        {
            BattleRoyaleUtils.Warn( "[Admin] roster reply could not be parsed: " + error );
            return;
        }

        m_Roster = incoming;
        m_RosterSeq++;
    }

    protected void RPC_ZoneReply( ParamsReadContext ctx )
    {
        string payload;
        if ( !ctx.Read( payload ) )
            return;
        if ( payload == "" )
            return;

        BattleRoyaleAdminZoneTable incoming = new BattleRoyaleAdminZoneTable();

        string error;
        JsonSerializer serializer = new JsonSerializer();
        if ( !serializer.ReadFromString( incoming, payload, error ) )
        {
            BattleRoyaleUtils.Warn( "[Admin] zone table reply could not be parsed: " + error );
            return;
        }

        m_ZoneTable = incoming;
        m_ZoneSeq++;
    }
#endif
}
#endif // JM_COT
