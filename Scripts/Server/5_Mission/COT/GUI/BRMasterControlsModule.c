#ifdef SERVER
#ifdef JM_COT
modded class BRMasterControlsModule
{
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
        }
    }

    //--- These RPCs drive the match state machine and are reachable by ANY connected client that
    //--- sends the right ScriptRPC id - the COT "BattleRoyale.StateMachine.View" permission only
    //--- gates whether the client renders the module, so it is no protection server-side.
    //--- Authorize against the same admin list BattleRoyaleServer uses.
    private void RPC_Next( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if(!BattleRoyaleServer.IsAdminIdentity(senderRPC))
        {
            BattleRoyaleUtils.Warn("[DayZBR COT] Rejected unauthorized Next request from " + BattleRoyaleServer.GetIdentityLogName(senderRPC));
            return;
        }

        Server_Next(); //Server received next command
    }

    private void RPC_Pause( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if(!BattleRoyaleServer.IsAdminIdentity(senderRPC))
        {
            BattleRoyaleUtils.Warn("[DayZBR COT] Rejected unauthorized Pause request from " + BattleRoyaleServer.GetIdentityLogName(senderRPC));
            return;
        }

        Server_Pause(); //Server received next command
    }

    private void RPC_Resume( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if(!BattleRoyaleServer.IsAdminIdentity(senderRPC))
        {
            BattleRoyaleUtils.Warn("[DayZBR COT] Rejected unauthorized Resume request from " + BattleRoyaleServer.GetIdentityLogName(senderRPC));
            return;
        }

        Server_Resume(); //Server received next command
    }

    private void RPC_SpawnAirdrop( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if(!BattleRoyaleServer.IsAdminIdentity(senderRPC))
        {
            BattleRoyaleUtils.Warn("[DayZBR COT] Rejected unauthorized SpawnAirdrop request from " + BattleRoyaleServer.GetIdentityLogName(senderRPC));
            return;
        }

#ifdef EXPANSIONMODMISSIONS
		//--- StringLocaliser takes a BARE stringtable key with no leading '#' (Expansion's own
		//--- call sites do the same) - a sentence here reaches the player untranslated.
		ExpansionNotification(new StringLocaliser( DAYZBR_MSG_TITLE ), new StringLocaliser( "STR_BR_COT_AIRDROP_SENT" ), DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, DAYZBR_MSG_TIME).Create();
		ExpansionMissionModule.s_Instance.CallAirdrop(senderRPC.GetPlayer().GetPosition());
#endif
    }

    //server-side functionality
    private void Server_Resume()
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

    private void Server_Pause()
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

    private void Server_Next()
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
