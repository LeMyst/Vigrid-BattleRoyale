modded class MissionGameplay
{
	protected Widget m_BattleRoyaleHudRootWidget;
	protected ref BattleRoyaleHud m_BattleRoyaleHud;

	protected bool is_spectator;

	void MissionGameplay()
	{
		m_BattleRoyaleHudRootWidget = null;
		is_spectator = false;
	}

	override void OnInit()
	{
		super.OnInit();
	
		m_BattleRoyale = new BattleRoyaleClient;

		InitBRhud();
	}


	void InitBRhud()
	{
		Print("Initializing BattleRoyale HUD");
		if(!m_BattleRoyaleHudRootWidget)
		{
			m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("BattleRoyale/GUI/layouts/hud/br_hud.layout");

			m_BattleRoyaleHud = new BattleRoyaleHud( m_BattleRoyaleHudRootWidget );
			m_BattleRoyaleHud.ShowHud( true );
		}
	}
	
	void InitSpectator()
	{
		m_BattleRoyaleHud.InitSpectator();
		is_spectator = true;

		//hide HUD and Quickbar
		IngameHud hud = IngameHud.Cast( GetHud() );
		if ( hud )
		{
			hud.ShowHudUI( false );
			hud.ShowQuickbarUI( false );
		}
	}
	void UpdateKillCount(int count)
	{
		m_BattleRoyaleHud.ShowKillCount( true );
		m_BattleRoyaleHud.SetKillCount( count );
	}
	void HideCountdownTimer()
	{
		m_BattleRoyaleHud.ShowCountdown( false );
	}
	void UpdateCountdownTimer(int seconds)
	{
		m_BattleRoyaleHud.ShowCountdown( true );
		m_BattleRoyaleHud.SetCountdown( seconds );
	}
	void UpdatePlayerCount(int count)
	{
		if(count == 0)
		{
			m_BattleRoyaleHud.ShowCount( false );
			return;
		}
		
		m_BattleRoyaleHud.ShowCount( true );
		m_BattleRoyaleHud.SetCount( count );
	}
	void UpdateZoneDistance(float distance)
	{
		m_BattleRoyaleHud.ShowDistance( true );
		m_BattleRoyaleHud.SetDistance( distance );
	}

	//TODO: move this into modded keybinds systems
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		
		if ( key == KeyCode.KC_SLASH )
		{
			if(!GetGame().GetUIManager().GetMenu())
			{
				SkinSelectionMenu.OpenMenu();
			}
		}

		if( key == KeyCode.KC_F1 )
		{
		
			BattleRoyaleClient.Cast( m_BattleRoyale ).ReadyUp();
			
		}

	}


	override void OnUpdate( float timeslice )
	{	
		

		super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets

		m_BattleRoyale.Update( timeslice ); //send tick to br client

		m_BattleRoyaleHud.Update( timeslice ); //this is really only used for spectator HUD updates


		if(is_spectator)
		{

			//ensure that if anything turns the hud back on, that we disable it again
			IngameHud hud = IngameHud.Cast( GetHud() );
			if ( hud )
			{
				if(hud.GetQuickBarState() || hud.GetHUDUiState())
				{
					hud.ShowHudUI( false );
					hud.ShowQuickbarUI( false );
				}
			}

		}
	}
}