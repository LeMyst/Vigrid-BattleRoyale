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
            case BattleRoyaleCOTStateMachineRPC.TestSpectator:
                RPC_TestSpectator( ctx, sender, target );
            break;
        }
    }

    private void RPC_Next( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        Server_Next(); //Server received next command
    }

    private void RPC_Pause( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        Server_Pause(); //Server received next command
    }

    private void RPC_Resume( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        Server_Resume(); //Server received next command
    }

    private void RPC_TestSpectator( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
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

    //server-side functionality
    private void Server_Resume()
    {
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            Print("[DayZBR COT] State Machine Resuming!");
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
            Print("[DayZBR COT] State Machine Pausing!");
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
            Print("[DayZBR COT] State Machine Skipping!");
            m_BrServer.GetCurrentState().Deactivate();// super.IsComplete() will return TRUE when this is run
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
    }

    private void Server_TestSpectator(PlayerBase player)
    {
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            Print("[DayZBR COT] Testing Spectating!");
            m_BrServer.TestSpectator(player);
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
    }
}
#endif // JM_COT
#endif