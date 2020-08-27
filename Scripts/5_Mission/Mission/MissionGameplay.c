#define BR_BETA_LOGGING

modded class MissionGameplay
{
	protected Widget m_BattleRoyaleHudRootWidget;
	protected ref m_BattleRoyaleHud;

	void MissionGameplay()
	{
		m_BattleRoyaleHudRootWidget = null;
	}

	override void OnInit()
	{
		super.OnInit();
		
		#ifdef BR_BETA_LOGGING
		BRPrint("MissionGameplay::OnInit()");
		#endif
		
		m_BattleRoyale = new BattleRoyaleClient;

		InitBRhud();
	}


	void InitBRhud()
	{
		if(!m_BattleRoyaleHudRootWidget)
		{
			m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("BattleRoyale/GUI/layouts/hud/br_hud.layout");

			m_BattleRoyaleHud = new BattleRoyaleHud( m_BattleRoyaleHudRootWidget );
			m_BattleRoyaleHud.ShowHud( true );
		}
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



	override void OnUpdate( float timeslice )
	{	
		

		super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets

		m_BattleRoyale.Update( timeslice ); //send tick to br client

		return; 

		//this is psuedocode for toggling UI visibility

		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);
		Input input = GetGame().GetInput();
		UIScriptedMenu topMenu = m_UIManager.GetMenu();

		bool inputIsFocused = false;
		Widget focusedWidget = GetFocus();

		if ( focusedWidget )
		{
			if ( focusedWidget.ClassName().Contains( "EditBoxWidget" ) )
			{
				inputIsFocused = true;
			} 
			else if ( focusedWidget.ClassName().Contains( "MultilineEditBoxWidget" ) )
			{
				input
				IsFocused = true;
			}
		}

		if ( playerPB && playerPB.GetHumanInventory() ) 
		{
			if ( playerPB.GetPlayerState() == EPlayerStates.ALIVE && !playerPB.IsUnconscious() )
			{
				if ( !topMenu && !inputIsFocused )
				{
					if(m_BattleRoyaleHud)
					{
						//--- BR hud toggle
						if ( input.LocalHold( "UAUIQuickbarToggle", false ) )
						{
							if ( m_BattleRoyaleHud.Shown() )
							{
								m_BattleRoyaleHud.ShowHud( false );
							}
							else
							{
								m_BattleRoyaleHud.ShowHud( true );
							}
						}
					}
				}
			}
		}
	}
}