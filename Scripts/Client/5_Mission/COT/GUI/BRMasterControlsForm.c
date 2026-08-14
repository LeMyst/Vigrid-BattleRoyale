#ifdef JM_COT
class BRMasterControlsForm: JMFormBase
{
    private UIActionScroller m_sclr_MainActions;
    private Widget m_ActionsWrapper;
    private BRMasterControlsModule m_Module;

    protected override bool SetModule( JMRenderableModuleBase mdl )
    {
        return Class.CastTo( m_Module, mdl );
    }

#ifndef SERVER
    override void OnInit()
    {
        m_sclr_MainActions = UIActionManager.CreateScroller( layoutRoot.FindAnyWidget( "panel" ) );
        m_ActionsWrapper = m_sclr_MainActions.GetContentWidget();

        //create button widgets dynamically
        Widget wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATEMACHINE" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_NEXTSTATE", this, "StateMachine_Next" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PAUSE", this, "StateMachine_Pause" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_RESUME", this, "StateMachine_Resume" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_EVENTMANAGER" );
#ifdef EXPANSIONMODMISSIONS
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_SPAWNAIRDROP", this, "SpawnAirdrop" );
#endif

        //--- No DIAG block here on purpose. "Add Player" / "Add Group" used to live here, but
        //--- BattleRoyaleCOTStateMachineRPC.AddFakePlayer / .AddFakeGroup have no case in the
        //--- server switch, so both buttons sent an RPC into the void. Fake players and fake
        //--- parties now come from the diag menu instead - see PluginDiagMenu.c.

        m_sclr_MainActions.UpdateScroller();
    }

    override void OnShow()
    {
    }

    override void OnHide()
    {
    }

    void StateMachine_Next(UIEvent eid, UIActionBase action)
    {
        m_Module.StateMachine_Next();
    }

    void StateMachine_Pause(UIEvent eid, UIActionBase action)
    {
        m_Module.StateMachine_Pause();
    }

    void StateMachine_Resume(UIEvent eid, UIActionBase action)
    {
        m_Module.StateMachine_Resume();
    }

    void SpawnAirdrop(UIEvent eid, UIActionBase action)
    {
        m_Module.SpawnAirdrop();
    }

    void SpawnHorde(UIEvent eid, UIActionBase action)
    {
        m_Module.SpawnHorde();
    }

    void SpawnChemicals(UIEvent eid, UIActionBase action)
    {
        m_Module.SpawnChemicals();
    }
#endif
}
#endif
