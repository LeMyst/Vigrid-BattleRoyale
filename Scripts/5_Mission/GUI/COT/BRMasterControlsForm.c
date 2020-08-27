


class BRMasterControlsForm extends JMFormBase
{
    private UIActionScroller m_sclr_MainActions;
	private Widget m_ActionsWrapper;
    private BRMasterControlsModule m_Module;

    void BRMasterControlsForm()
    {
        //map lists
    }
    void ~BRMasterControlsForm()
    {
        //Delete any mapped lists
    }

    protected override bool SetModule( ref JMRenderableModuleBase mdl )
	{
        return Class.CastTo( m_Module, mdl );
    }

    override void OnInit()
	{
		m_sclr_MainActions = UIActionManager.CreateScroller( layoutRoot.FindAnyWidget( "panel" ) );
		m_ActionsWrapper = m_sclr_MainActions.GetContentWidget();

		m_sclr_MainActions.UpdateScroller();
	}

    override void OnShow()
	{
        //create button widgets dynamically
        Widget wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
        UIActionManager.CreateText( wrapper, "State Machine" );
        UIActionButton button = UIActionManager.CreateButton( wrapper, "Next State", this, "StateMachine_Next" );
        button = UIActionManager.CreateButton( wrapper, "Pause", this, "StateMachine_Pause" );
        button = UIActionManager.CreateButton( wrapper, "Resume", this, "StateMachine_Resume" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
        UIActionManager.CreateText( wrapper, "Loot System" );
        button = UIActionManager.CreateButton( wrapper, "Start", this, "LootSystem_Start" );
        button = UIActionManager.CreateButton( wrapper, "Stop", this, "LootSystem_Stop" );

    }
    override void OnHide() 
	{

    }

    void StateMachine_Next(UIEvent eid, ref UIActionBase action)
    {
        m_Module.StateMachine_Next();
    }
    void StateMachine_Pause(UIEvent eid, ref UIActionBase action)
    {
        m_Module.StateMachine_Pause();
    }
    void StateMachine_Resume(UIEvent eid, ref UIActionBase action)
    {
        m_Module.StateMachine_Resume();
    }

    void LootSystem_Start(UIEvent eid, ref UIActionBase action)
    {
        m_Module.Loot_Start();
    }
    void LootSystem_Stop(UIEvent eid, ref UIActionBase action)
    {
        m_Module.Loot_Stop();
    }
}