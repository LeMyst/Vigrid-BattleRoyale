#ifdef JM_COT
class BRMasterControlsModule: JMRenderableModuleBase
{
    void BRMasterControlsModule()
    {
        GetPermissionsManager().RegisterPermission( "BattleRoyale.StateMachine.View" );
        GetPermissionsManager().RegisterPermission( "BattleRoyale.StateMachine.Update" );
    }

    override bool HasAccess()
    {
        return GetPermissionsManager().HasPermission( "BattleRoyale.StateMachine.View" );
    }

    override string GetLayoutRoot()
    {
        return "Vigrid-BattleRoyale/GUI/layouts/COT/master_controls.layout";
    }

    override string GetTitle()
    {
        return "[BR] State Machine";
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
        return "BattleRoyale State Machine";
    }

    override int GetRPCMin()
    {
        return BattleRoyaleCOTStateMachineRPC.GET;
    }

    override int GetRPCMax()
    {
        return BattleRoyaleCOTStateMachineRPC.ERROR;
    }

	override void OnRPC( PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx )
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
        if ( IsMissionHost() )
        {
            Server_Next(); //Server received next command
        }
    }

    private void RPC_Pause( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if ( IsMissionHost() )
        {
            Server_Pause(); //Server received next command
        }
    }

    private void RPC_Resume( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if ( IsMissionHost() )
        {
            Server_Resume(); //Server received next command
        }
    }

    private void RPC_TestSpectator( ref ParamsReadContext ctx, PlayerIdentity senderRPC, Object target )
    {
        if ( IsMissionHost() )
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
    }

    //client side functionality (these get called)
    void StateMachine_Next()
    {
        if ( IsMissionClient() )
        {
            //send RPC to server
            ScriptRPC rpc = new ScriptRPC();
            //rpc.Write( 0 ); //I don't think we need to write any data to send this RPC
            rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Next, true, NULL );
        }
        else
        {
            Error("Server called StateMachine_Next()");
        }

    }

    void StateMachine_Pause()
    {
        if ( IsMissionClient() )
        {
            //send RPC to server
            ScriptRPC rpc = new ScriptRPC();
            //rpc.Write( 0 ); //I don't think we need to write any data to send this RPC
            rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Pause, true, NULL );
        }
        else
        {
            Error("Server called StateMachine_Pause()");
        }
    }

    void StateMachine_Resume()
    {
        if ( IsMissionClient() )
        {
            //send RPC to server
            ScriptRPC rpc = new ScriptRPC();
            //rpc.Write( 0 ); //I don't think we need to write any data to send this RPC
            rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Resume, true, NULL );
        }
        else
        {
            Error("Server called StateMachine_Resume()");
        }
    }

    void TestSpectator()
    {
        if ( IsMissionClient() )
        {
            ScriptRPC rpc = new ScriptRPC();
            rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.TestSpectator, true, NULL );
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
