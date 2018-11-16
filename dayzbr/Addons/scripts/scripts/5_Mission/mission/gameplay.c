modded class MissionGameplay
{
	ref BattleRoyale BR_GAME;
	
	/*
	void MissionGameplay()
	{
		//DestroyAllMenus();
		DestroyInventory(); // ????
		
		m_Initialized				= false;
		m_HudRootWidget				= null;
		m_Chat						= new Chat;
		m_ActionMenu				= new ActionMenu;
		m_LifeState					= -1;
		m_Hud						= new IngameHud;
		m_ChatChannelFadeTimer		= new WidgetFadeTimer;
		m_ChatChannelHideTimer		= new Timer(CALL_CATEGORY_GUI);
		
		m_ToggleHudTimer			= new Timer(CALL_CATEGORY_GUI);
		
		g_Game.m_loadingScreenOn	= true;
		
		SyncEvents.RegisterEvents();
		g_Game.SetGameState( DayZGameState.IN_GAME );
		PlayerBase.Event_OnPlayerDeath.Insert( Pause );
	}*/
	
	override void OnInit()
	{
		super.OnInit();
		
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SendGlobalMessage", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SendClientMessage", this );
		
		BR_GAME = new BattleRoyale( this );
	}
	
	//BR Game Messaging remote calls
	void SendGlobalMessage( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data;
		if( !ctx.Read( data ) ) return;
        
		if( GetGame() )
		{
			string msg = data.param1;
			PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
			
			if(!msg.Contains("ALL: "))
			{
				if(me.my_round)
				{
					if(msg.Contains(me.my_round))
					{
						msg.Replace(me.my_round + ": ","");
						m_Chat.Add("Server",msg);
					}
				}
			}
			else
			{
				msg.Replace("ALL: ","");
				m_Chat.Add("Server",msg);
				
			}
		}
	}

	void SendClientMessage( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data;
		if( !ctx.Read( data ) ) return;
        
		if ( type == CallType.Client )
		{
			PlayerBase player = PlayerBase.Cast( target );

			if ( !player ) return;

			m_Chat.Add("Server",data.param1);
		}
	}

	//Unlock gesture menu
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		
		
		if ( key == KeyCode.KC_PERIOD )
		{
			if ( !GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				
				NewGesturesMenu.OpenMenu();
			}
		}
		if( key == KeyCode.KC_COMMA )
		{
			if ( !GetUIManager().IsMenuOpen( MENU_SKINS ) )
			{
				
				SkinMenu.OpenMenu();
			}
		}
	}
	override void OnKeyRelease(int key)
	{
		super.OnKeyRelease(key);
		
		if ( key == KeyCode.KC_PERIOD )
		{
			if ( GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				NewGesturesMenu.CloseMenu();
			}
		}
		if ( key == KeyCode.KC_COMMA )
		{
			if ( GetUIManager().IsMenuOpen( MENU_SKINS ) )
			{
				SkinMenu.CloseMenu();
			}
		}
	}
	
	
	//Fix for UI fade
	override void OnUpdate(float timeslice)
	{
		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);
		TickScheduler(timeslice);
		UpdateDummyScheduler();//for external entities
		UIScriptedMenu menu = m_UIManager.GetMenu();
		InventoryMenu inventory = InventoryMenu.Cast( m_UIManager.FindMenu(MENU_INVENTORY) );
		MapMenu mapmenu = MapMenu.Cast( m_UIManager.FindMenu(MENU_MAP) );
		//m_InventoryMenu = inventory;
		InspectMenuNew inspect = InspectMenuNew.Cast( m_UIManager.FindMenu(MENU_INSPECT) );
		Input input = GetGame().GetInput();
		
		//TODO should be switchable
		if (playerPB && playerPB.enterNoteMenuRead)
		{
			playerPB.enterNoteMenuRead = false;
			Paper paper = Paper.Cast(playerPB.GetItemInHands());
			m_Note = NoteMenu.Cast( GetUIManager().EnterScriptedMenu(MENU_NOTE, menu) ); //NULL means no parent
			m_Note.InitRead(paper.m_AdvancedText);
		}
		
		if (playerPB && playerPB.enterNoteMenuWrite)
		{
			playerPB.enterNoteMenuWrite = false;
			m_Note = NoteMenu.Cast( GetUIManager().EnterScriptedMenu(MENU_NOTE, menu) ); //NULL means no parent
			m_Note.InitWrite(playerPB.m_paper,playerPB.m_writingImplement,playerPB.m_Handwriting);
		}


		if (player && m_LifeState == EPlayerStates.ALIVE && !player.IsUnconscious() )
		{
			// enables HUD on spawn
			if (m_HudRootWidget)
			{
				m_HudRootWidget.Show(true);
			}
			
			// fade out black screen
			
			PlayerBaseClient playerPBC = PlayerBaseClient.Cast(player);
			
			if ( GetUIManager().ScreenFadeVisible() && !playerPBC.FORCE_FADE )
			{
				 GetUIManager().ScreenFadeOut(0.5);
			}
			
		
			if( input.GetActionDown(UAGear, false ) )
			{
				if( !inventory )
				{
					ShowInventory();
				}
				else if( menu == inventory )
				{
					HideInventory();
				}
			}

			if (input.GetActionDown(UAChat, false))
			{
				BrChatMenu chat = BrChatMenu.Cast( m_UIManager.FindMenu(MENU_CHAT) );		
				if( menu == NULL )
				{
					ShowChat();
				}
			}
			
			if ( input.GetActionDown( UAUIQuickbarToggle, false) )
			{
				SetActionDownTime( GetGame().GetTime() );
				bool hud_state = m_Hud.GetHudState();
				m_ToggleHudTimer.Run( 0.3, m_Hud, "ToggleHud", new Param1<bool>( !hud_state ) );
			}
			
			if ( input.GetActionUp( UAUIQuickbarToggle, false) )
			{
				SetActionUpTime( GetGame().GetTime() );
				
				if ( GetHoldActionTime() < HOLD_LIMIT_TIME )
				{
					if ( menu == NULL )
					{
						if ( m_Hud.GetQuickBarState() )
						{
							m_Hud.HideQuickbar( false, true );
						}
						else
						{
							m_Hud.ShowQuickbar();
						}
					}
				}
				
				m_ToggleHudTimer.Stop();
			}
			
			if ( g_Game.GetInput().GetActionDown( UAZeroingUp, false) || g_Game.GetInput().GetActionDown( UAZeroingDown, false) || g_Game.GetInput().GetActionDown( UAToggleWeapons, false) )
			{

				m_Hud.ZeroingKeyPress();
			}
			
			if ( menu == NULL )
			{
				m_ActionMenu.Refresh();
				
				if (input.GetActionDown(UANextAction, false))
				{
					m_ActionMenu.NextAction();
				}
				
				if (input.GetActionDown(UAPrevAction, false))
				{
					m_ActionMenu.PrevAction();
				}
			}
			else
			{
				m_ActionMenu.Hide();
			}
			
			//hologram rotation
			if (menu == NULL && playerPB.IsPlacingLocal())
			{
				if (input.GetActionUp(UANextAction, false))
				{
					playerPB.GetHologramLocal().SubtractProjectionRotation(15);
				}
				
				if (input.GetActionUp(UAPrevAction, false))
				{
					playerPB.GetHologramLocal().AddProjectionRotation(15);
				}
			}
		}
		
		// life state check
		if (player)
		{
			int life_state = player.GetPlayerState();
			
			// life state changed
			if (m_LifeState != life_state)
			{
				m_LifeState = life_state;
				
				if (m_LifeState != EPlayerStates.ALIVE)
				{
					CloseAllMenus();
				}
			}
		}
		
		if (menu && !menu.UseKeyboard() && menu.UseMouse())
		{
			int i;
			for (i = 0; i < 5; i++)
			{
				input.DisableKey(i | INPUT_DEVICE_MOUSE);
				input.DisableKey(i | INPUT_ACTION_TYPE_DOWN_EVENT | INPUT_DEVICE_MOUSE);
				input.DisableKey(i | INPUT_ACTION_TYPE_DOUBLETAP | INPUT_DEVICE_MOUSE);
			}
			
			for (i = 0; i < 6; i++)
			{
				input.DisableKey(i | INPUT_DEVICE_MOUSE_AXIS);
			}
		}
		
		if (!m_UIManager.IsDialogVisible())
		{
			if ( menu )
			{

				if ( IsPaused() )
				{
					if( input.GetActionDown(UAUIBack, false) )
					{
						Continue();						
					}
					else if( input.GetActionDown(UAUIMenu, false) )
					{
						Continue();
					}
				}
				else if ( menu == inventory )
				{
					if(input.GetActionDown(UAUIBack, false))
					{
						if( ItemManager.GetInstance().GetSelectedItem() == NULL )
						{
							HideInventory();
						}
					}
				}
				else if( menu == inspect )
				{
					if(input.GetActionDown(UAGear, false))
					{
						if( ItemManager.GetInstance().GetSelectedItem() == NULL )
						{
							HideInventory();
						}
					}
					else if(input.GetActionDown(UAUIBack, false))
					{
						if( ItemManager.GetInstance().GetSelectedItem() == NULL )
						{
							HideInventory();
						}
					}
				}
				else if(input.GetActionDown(UAUIBack, false))
				{
					m_UIManager.Back();
					PlayerControlEnable();
				}
				else if(menu == mapmenu && !m_ControlDisabled)
				{
					PlayerControlDisable();
				}
				
			}
			else if (input.GetActionDown(UAUIMenu, false))
			{
				Pause();
				PlayerControlDisable();
			}
		}
		
		UpdateDebugMonitor();
		
		SEffectManager.Event_OnFrameUpdate(timeslice);
	}
	
}