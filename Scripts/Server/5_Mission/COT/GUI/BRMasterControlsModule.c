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
#ifdef SPECTATOR
            case BattleRoyaleCOTStateMachineRPC.TestSpectator:
                RPC_TestSpectator( ctx, sender, target );
            break;
#endif
            case BattleRoyaleCOTStateMachineRPC.SpawnAirdrop:
                RPC_SpawnAirdrop( ctx, sender, target );
            break;
        }
    }

    private void RPC_Next( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        Server_Next(); //Server received next command
    }

    private void RPC_Pause( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        Server_Pause(); //Server received next command
    }

    private void RPC_Resume( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        Server_Resume(); //Server received next command
    }

#ifdef SPECTATOR
    private void RPC_TestSpectator( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        PlayerBase pbTarget;
        if(Class.CastTo( pbTarget, target ))
        {
            Server_TestSpectator( pbTarget );
        }
        else
        {
            Error("Failed to cast TestSpectator target object to playerbase");
        }
    }
#endif

    private void RPC_SpawnAirdrop( ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
#ifdef EXPANSIONMODMISSIONS
		ExpansionNotification(new StringLocaliser( DAYZBR_MSG_TITLE ), new StringLocaliser( "Airdrop sent." ), DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, DAYZBR_MSG_TIME).Create();
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

#ifdef SPECTATOR
    private void Server_TestSpectator(PlayerBase player)
    {
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            BattleRoyaleUtils.Trace("[DayZBR COT] Testing Spectating!");
            m_BrServer.TestSpectator(player);
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
    }
#endif
}
#endif // JM_COT
#endif
