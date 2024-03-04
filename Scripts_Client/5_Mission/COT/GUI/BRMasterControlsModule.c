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

    //client side functionality (these get called)
    void StateMachine_Next()
    {
#ifndef SERVER
        //send RPC to server
        ScriptRPC rpc = new ScriptRPC();
        //rpc.Write( 0 ); //I don't think we need to write any data to send this RPC
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Next, true, NULL );
#endif
    }

    void StateMachine_Pause()
    {
#ifndef SERVER
        //send RPC to server
        ScriptRPC rpc = new ScriptRPC();
        //rpc.Write( 0 ); //I don't think we need to write any data to send this RPC
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Pause, true, NULL );
#endif
    }

    void StateMachine_Resume()
    {
#ifndef SERVER
        //send RPC to server
        ScriptRPC rpc = new ScriptRPC();
        //rpc.Write( 0 ); //I don't think we need to write any data to send this RPC
        rpc.Send( NULL, BattleRoyaleCOTStateMachineRPC.Resume, true, NULL );
#endif
    }

    void TestSpectator()
    {
#ifndef SERVER
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.TestSpectator, true, NULL );
#endif
    }
}
#endif // JM_COT
