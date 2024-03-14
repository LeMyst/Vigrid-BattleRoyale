#ifdef JM_COT
class BRMasterControlsModule: JMRenderableModuleBase
{
    private Class m_EventManager;

    autoptr array<string> Events = new array<string>();
    int MaxEventCount;

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
        return BattleRoyaleCOTStateMachineRPC.INVALID;
    }

    override int GetRPCMax()
    {
        return BattleRoyaleCOTStateMachineRPC.COUNT;
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

    void TestSpectator()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.TestSpectator, true, NULL );
    }

    void AddFakePlayer()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.AddFakePlayer, true, NULL );
    }

    void AddFakeGroup()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.AddFakeGroup, true, NULL );
    }

    void SpawnAirdrop()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.SpawnAirdrop, true, NULL );
    }

    void SpawnHorde()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.SpawnHorde, true, NULL );
    }

    void SpawnChemicals()
    {
        ScriptRPC rpc = new ScriptRPC();
        rpc.Send( GetGame().GetPlayer(), BattleRoyaleCOTStateMachineRPC.SpawnChemicals, true, NULL );
    }
#endif
}
#endif // JM_COT
