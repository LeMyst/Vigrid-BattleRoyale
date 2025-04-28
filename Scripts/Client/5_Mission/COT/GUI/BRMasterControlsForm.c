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
            UIActionManager.CreateText( wrapper, "State Machine" );
            UIActionManager.CreateButton( wrapper, "Next State", this, "StateMachine_Next" );
            UIActionManager.CreateButton( wrapper, "Pause", this, "StateMachine_Pause" );
            UIActionManager.CreateButton( wrapper, "Resume", this, "StateMachine_Resume" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
            UIActionManager.CreateText( wrapper, "Event Manager" );
#ifdef EXPANSIONMODMISSIONS
            UIActionManager.CreateButton( wrapper, "Spawn Airdrop", this, "SpawnAirdrop" );
#endif

#ifdef DIAG
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
            UIActionManager.CreateText( wrapper, "DIAG DEBUG" );
            UIActionManager.CreateButton( wrapper, "Add Player", this, "AddFakePlayer" );
            UIActionManager.CreateButton( wrapper, "Add Group", this, "AddFakeGroup" );

        //wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 2 );
        //    UIActionManager.CreateText( wrapper, "Spectating Camera" );
            UIActionManager.CreateButton( wrapper, "Spectate", this, "ToggleSpectating" );
#endif

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

    void AddFakePlayer(UIEvent eid, UIActionBase action)
    {
        m_Module.AddFakePlayer();
    }

    void AddFakeGroup(UIEvent eid, UIActionBase action)
    {
        m_Module.AddFakeGroup();
    }

#ifdef SPECTATOR
    void ToggleSpectating(UIEvent eid, UIActionBase action)
    {
        m_Module.TestSpectator();
    }
#endif

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
