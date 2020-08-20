#define BR_BETA_LOGGING

modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		
		#ifdef BR_BETA_LOGGING
		BRPrint("MissionGameplay::OnInit()");
		#endif
		
		m_BattleRoyale = new BattleRoyaleClient;
	}

	override void OnUpdate( float timeslice )
	{	
		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);

		//only run our custom update if it is absolutely necessary
		if(player && playerPB && playerPB.allow_fade && m_LifeState == EPlayerStates.ALIVE && !player.IsUnconscious() && GetUIManager().ScreenFadeVisible())
		{
			DayZOnUpdate( timeslice );
			ExpansionOnUpdate( timeslice );
		}
		else
		{
			//since we are not fading out screen, run the standard onUpdate
			super.OnUpdate( timeslice );
		}
		
	}

	//This is a patch for allowing fade to take place. This will need updated with every game update & expansion update
	void DayZOnUpdate( float timeslice )
	{
		//--- 1st: DayZ onUpdate Code
		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);
		TickScheduler(timeslice);
		UpdateDummyScheduler();
		UIScriptedMenu menu = m_UIManager.GetMenu();
		InventoryMenu inventory = InventoryMenu.Cast( m_UIManager.FindMenu(MENU_INVENTORY) );
		MapMenu map_menu = MapMenu.Cast( m_UIManager.FindMenu(MENU_MAP) );
		NoteMenu note_menu = NoteMenu.Cast( m_UIManager.FindMenu(MENU_NOTE) );
		GesturesMenu gestures_menu = GesturesMenu.Cast(m_UIManager.FindMenu(MENU_GESTURES));
		RadialQuickbarMenu quickbar_menu = RadialQuickbarMenu.Cast(m_UIManager.FindMenu(MENU_RADIAL_QUICKBAR));
		InspectMenuNew inspect = InspectMenuNew.Cast( m_UIManager.FindMenu(MENU_INSPECT) );
		Input input = GetGame().GetInput();

		if( playerPB )
		{
			if( playerPB.GetHologramLocal() )
			{
				playerPB.GetHologramLocal().UpdateHologram( timeslice );
			}
		}

		if( input.LocalPress("UAUIGesturesOpen",false) )
		{
			if ( !playerPB.IsRaised() && !playerPB.GetCommand_Vehicle() )
			{
				if ( !GetUIManager().IsMenuOpen( MENU_GESTURES ) )
				{
					GesturesMenu.OpenMenu();
					m_Hud.ShowHudUI( false );
				}
			}
		}

		if( input.LocalRelease("UAUIGesturesOpen",false) )
		{
			if ( GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				GesturesMenu.CloseMenu();
				gestures_menu.SetMenuClosing(true);
				PlayerControlEnable(false);
				m_Hud.ShowHudUI( true );
			}
		}
		if (player && m_LifeState == EPlayerStates.ALIVE && !player.IsUnconscious() )
		{
			if (m_HudRootWidget)
			{
				m_HudRootWidget.Show(true);
			}
			//--- NOTE: we are skipping fadeout check her (because we want manual fade control)
			if( input.LocalPress("UAGear",false) )
			{
				if( !inventory && playerPB.CanManipulateInventory() )
				{
					ShowInventory();
					menu = m_InventoryMenu;
				}
				else if( menu == inventory )
				{
					HideInventory();
				}
			}
			if( input.LocalPress("UAChat",false) )
			{
				ChatInputMenu chat = ChatInputMenu.Cast( m_UIManager.FindMenu(MENU_CHAT) );		
				if( menu == NULL )
				{
					ShowChat();
				}
			}
			if( input.LocalPress("UAVoiceLevel",false) )
			{
				int oldLevel = GetGame().GetVoiceLevel();
				int newLevel = ( oldLevel + 1 ) % ( VoiceLevelShout + 1 );
				
				// update general voice icon
				UpdateVoiceLevelWidgets(newLevel);
				GetGame().SetVoiceLevel(newLevel);	
			}
			if( input.LocalHold("UAUIQuickbarToggle",false) )
			{
				if( !m_QuickbarHold )
				{
					m_QuickbarHold = true;
					SetActionDownTime( GetGame().GetTime() );
					//m_Hud.ShowHudPlayer( m_Hud.IsHideHudPlayer() );
					m_Hud.ShowHud( !m_Hud.GetHudState() );
				}
			}
			if( input.LocalRelease("UAUIQuickbarToggle",false) )
			{
				if( !m_QuickbarHold )
					m_Hud.ShowQuickbarPlayer( m_Hud.IsHideQuickbarPlayer() );
				m_QuickbarHold = false;
			}
			if ( g_Game.GetInput().LocalPress("UAZeroingUp",false) || g_Game.GetInput().LocalPress("UAZeroingDown",false) || g_Game.GetInput().LocalPress("UAToggleWeapons",false) )
			{

				m_Hud.ZeroingKeyPress();
			}
			if ( menu == NULL )
			{
				m_ActionMenu.Refresh();
				
				if (input.LocalPress("UANextAction",false))
				{
					m_ActionMenu.NextAction();
				}
				
				if (input.LocalPress("UAPrevAction",false))
				{
					m_ActionMenu.PrevAction();
				}
			}
			else
			{
				m_ActionMenu.Hide();
			}
			if (menu == NULL && playerPB.IsPlacingLocal())
			{
				if( input.LocalRelease("UANextAction",false) )
				{
					playerPB.GetHologramLocal().SubtractProjectionRotation(15);
				}
				
				if( input.LocalRelease("UAPrevAction",false) )
				{
					playerPB.GetHologramLocal().AddProjectionRotation(15);
				}
			}
		}
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
				if( menu == inspect )
				{
					if(input.LocalPress("UAGear",false))
					{
						if( ItemManager.GetInstance().GetSelectedItem() == NULL )
						{
							HideInventory();
						}
					}
					else if(input.LocalPress("UAUIBack",false))
					{
						if( ItemManager.GetInstance().GetSelectedItem() == NULL )
						{
							HideInventory();
						}
					}
				}
				else if(menu == map_menu && !m_ControlDisabled)
				{
					PlayerControlDisable(INPUT_EXCLUDE_INVENTORY);
				}
				else if(menu == note_menu && !m_ControlDisabled)
				{
					PlayerControlDisable(INPUT_EXCLUDE_INVENTORY);
				}
				else if(menu == gestures_menu && !m_ControlDisabled && !gestures_menu.IsMenuClosing())
				{
					PlayerControlDisable(INPUT_EXCLUDE_MOUSE_RADIAL);
					GetUApi().GetInputByName("UAUIGesturesOpen").Unlock();
				}
				else if(menu == quickbar_menu && !m_ControlDisabled && !quickbar_menu.IsMenuClosing())
				{
					PlayerControlDisable(INPUT_EXCLUDE_MOUSE_RADIAL);
					GetUApi().GetInputByName("UAUIQuickbarRadialOpen").Unlock();
				}
				else if( IsPaused() )
				{
					InGameMenuXbox menu_xb = InGameMenuXbox.Cast( GetGame().GetUIManager().GetMenu() );
					if( !g_Game.GetUIManager().ScreenFadeVisible() && ( !menu_xb || !menu_xb.IsOnlineOpen() ) )
					{
						if( input.LocalPress("UAUIMenu",false) )
						{
							Continue();
						}
						else if( input.LocalPress( "UAUIBack", false ) )
						{
							Continue();
						}
					}
					else if( input.LocalPress( "UAUIBack", false ) )
					{
						if( menu_xb && menu_xb.IsOnlineOpen() )
						{
							menu_xb.CloseOnline();
						}
					}
				}
			}
			else if (input.LocalPress("UAUIMenu",false))
			{
				Pause();
			}
			
			//final controls check that suppresses inputs to avoid input collision. If anything needed to be handled without forced input suppression, it had been at this point.
			if( playerPB )
			{
				if( !menu && m_ControlDisabled && !playerPB.GetCommand_Melee2() )
				{
					PlayerControlEnable(true);
				}
			}
		}

		UpdateDebugMonitor();
		
		SEffectManager.Event_OnFrameUpdate(timeslice);
				
	}

	void ExpansionOnUpdate( float timeslice )
	{
		if ( !m_bLoaded )
		{
			#ifdef EXPANSIONEXPRINT
			EXPrint("MissionGameplay::OnUpdate - End");
			#endif

			return;
		}

		// GetDayZExpansion().OnUpdate( timeslice );

		//! Checking for keyboard focus
		bool inputIsFocused = false;
		
		//! Reference to focused windget
		Widget focusedWidget = GetFocus();

		if ( focusedWidget )
		{
			if ( focusedWidget.ClassName().Contains( "EditBoxWidget" ) )
			{
				inputIsFocused = true;
			} 
			else if ( focusedWidget.ClassName().Contains( "MultilineEditBoxWidget" ) )
			{
				inputIsFocused = true;
			}
		}

		//! Refernce to man
		Man man = GetGame().GetPlayer();

		//! Reference to input
		Input input = GetGame().GetInput();

		//! Expansion reference to menu
		UIScriptedMenu topMenu = m_UIManager.GetMenu();

		//! Expansion reference to player
		PlayerBase playerPB = PlayerBase.Cast( man );
		
		if ( playerPB && playerPB.GetHumanInventory() ) 
		{
			//! Expansion reference to item in hands
			ItemBase itemInHands = ItemBase.Cast(playerPB.GetHumanInventory().GetEntityInHands());

			//! Expansion reference to hologram
			ref Hologram hologram;	

			if ( playerPB.GetPlayerState() == EPlayerStates.ALIVE && !playerPB.IsUnconscious() )
			{
				//TODO: Make ExpansionInputs class and handle stuff there to keep this clean

				//! Chat
				if ( input.LocalPress( "UAChat", false ) && !inputIsFocused && !topMenu )
				{
					ShowChat();
				}
				
				//! Switch Chat Channel
				if ( input.LocalPress( "UAExpansionChatSwitchChannel", false ) && !inputIsFocused )
				{
					SwitchChannel();

					exp_m_ChannelNameFadeTimer.FadeIn(m_ChatChannelName, EXP_FADE_IN_DURATION);
					exp_m_ChannelNameTimeoutTimer.Run(EXP_FADE_TIMEOUT, exp_m_ChannelNameFadeTimer, "FadeOut", new Param2<Widget, float>(m_ChatChannelName, EXP_FADE_OUT_DURATION));

					exp_m_ChannelFadeTimer.FadeIn(m_WidgetChatChannel, EXP_FADE_IN_DURATION);
					exp_m_ChannelTimeoutTimer.Run(EXP_FADE_TIMEOUT, exp_m_ChannelFadeTimer, "FadeOut", new Param2<Widget, float>(m_WidgetChatChannel, EXP_FADE_OUT_DURATION));
				}

				if ( !topMenu && !inputIsFocused )
				{
					//! Autorun
					if ( input.LocalPress( "UAExpansionAutoRunToggle", false ) )
					{
						if ( !man.GetParent() && GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnableAutoRun )
						{
							m_AutoRunModule.AutoRun();
						}
					}
					
					//! Book Menu
					if ( input.LocalPress( "UAExpansionBookToggle", false ) )
					{
						if ( !GetGame().GetUIManager().GetMenu() && GetExpansionSettings() && GetExpansionSettings().GetBook().EnableBook )
						{
							GetGame().GetUIManager().EnterScriptedMenu( MENU_EXPANSION_BOOK_MENU, NULL );
						}
					}
					
					//! Map Menu
					if ( input.LocalPress( "UAExpansionMapToggle", false ) )
					{
						ExpansionMapMenu map_menu;
						if ( Class.CastTo( map_menu, GetGame().GetUIManager().FindMenu( MENU_EXPANSION_MAP ) ) )
						{
							map_menu.Hide();
							map_menu.Close();
						} else if ( !GetGame().GetUIManager().GetMenu() && GetExpansionSettings().GetMap() && GetExpansionSettings().GetMap().CanOpenMapWithKeyBinding )
						{
							if ( GetExpansionSettings().GetGeneral().NeedMapItemForKeyBinding )
							{
								if ( PlayerBase.Cast( GetGame().GetPlayer() ).HasItemMap() || PlayerBase.Cast( GetGame().GetPlayer() ).HasItemGPS() )
									GetGame().GetUIManager().EnterScriptedMenu( MENU_EXPANSION_MAP, NULL );
							} else
							{
								GetGame().GetUIManager().EnterScriptedMenu( MENU_EXPANSION_MAP, NULL );
							}
						}
					}
					
					//! GPS	
					if ( input.LocalPress( "UAExpansionGPSToggle", false ) )
					{
						#ifdef EXPANSIONEXLOGPRINT
						EXLogPrint("MissionGameplay::OnUpdate - UAExpansionGPSToggle pressed and setting for item is: " + GetExpansionSettings().GetGeneral().NeedGPSItemForKeyBinding.ToString() );
						#endif

						if ( GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnableHUDGPS/*&& m_ExpansionHud.IsInitialized() && m_ExpansionHud.GetGPSState()*/ )
						{		
							if ( GetExpansionSettings().GetGeneral().NeedGPSItemForKeyBinding )
							{
								#ifdef EXPANSIONEXLOGPRINT
								EXLogPrint("MissionGameplay::OnUpdate - UAExpansionGPSToggle pressed and player has gps: " + PlayerBase.Cast( GetGame().GetPlayer() ).HasItemGPS().ToString() );
								#endif
								
								if ( PlayerBase.Cast( GetGame().GetPlayer() ).HasItemGPS() )
									ToggleHUDGPSMode();
							}
							else
							{
								ToggleHUDGPSMode();
							}
						}
					}
					
					if ( input.LocalPress( "UAExpansionGPSMapScaleDown", false ) )
					{
						if ( GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnableHUDGPS && m_ExpansionHud.IsInitialized() && m_ExpansionHud.GetGPSMapState() )
						{							
							DecreaseGPSMapScale();
						}
					}
					
					if ( input.LocalPress( "UAExpansionGPSMapScaleUp", false ) )
					{
						if ( GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnableHUDGPS && m_ExpansionHud.IsInitialized() && m_ExpansionHud.GetGPSMapState() )
						{
							IncreaseGPSMapScale();
						}
					}
					
					//! Player List Menu
					if ( input.LocalPress( "UAExpansionPlayerListToggle", false ) )
					{
						if ( !GetGame().GetUIManager().GetMenu() && GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnablePlayerList )
						{
							GetGame().GetUIManager().EnterScriptedMenu( MENU_EXPANSION_PLAYER_LIST_MENU, NULL );
						}
					}
					
					//! Expansion Hud
					if ( input.LocalHold( "UAUIQuickbarToggle", false ) )
					{
						if ( !m_Hud.GetHudState() )
						{
							m_ExpansionHud.ShowHud( false );
						}
						else
						{
							m_ExpansionHud.ShowHud( true );
						}
					}
					
					//! Gestures
					if ( input.LocalPress( "UAUIGesturesOpen",false ) )
					{
						//! Open gestures menu
						if ( !playerPB.IsRaised() && !playerPB.GetCommand_Vehicle() )
						{
							if ( !GetUIManager().IsMenuOpen( MENU_GESTURES ) )
							{
								m_ExpansionHud.ShowHud( false );
							}
						}
					}
					
					//! Toggle Earplugs
					if ( input.LocalPress( "UAExpansionEarplugsToggle", false ) )
					{
						m_ExpansionHud.ToggleEarplugs();
					}

					if ( m_MarkerModule )
					{
						//! Toggle 3d marker visiblity
						if ( input.LocalPress( "UAExpansion3DMarkerToggle", false ) )
						{
							//if ( m_MarkerModule.IsWorldVisible() )
							//{
							//	m_MarkerModule.RemoveVisibility( EXPANSION_MARKER_VIS_WORLD );
							//} else
							//{
							//	m_MarkerModule.SetVisibility( EXPANSION_MARKER_VIS_WORLD );
							//}
						}
						
						//! Toggle 3d party members markers visiblity
						if ( input.LocalPress( "UAExpansionOnlyPartyMembersMarkersToggle", false ) )
						{
							//m_MarkerModule.ToggleOnlyPartyMembersMarkers();
						}
					}
				}

				//! Basebuilding Snaping
				if ( playerPB && playerPB.IsPlacingLocal() && !inputIsFocused )
				{
					hologram = playerPB.GetHologramLocal();

					if ( hologram )
					{
						if ( input.LocalPress( "UAExpansionSnappingToggle" ) )
						{
							hologram.SetUsingSnap( !hologram.IsUsingSnap() );
						}

						if ( input.LocalValue( "UAExpansionSnappingDirectionNext" ) != 0 )
						{
							hologram.NextDirection();
						}

						if ( input.LocalValue( "UAExpansionSnappingDirectionPrevious" ) != 0 )
						{
							hologram.PreviousDirection();
						}
					}
				}

				if ( m_AutoRunModule )
				{
					//! Autowalk
					if ( GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnableAutoRun )
					{
						m_AutoRunModule.UpdateAutoWalk();
					}
					
					//! Stop autorun when different inputs are pressed
					if ( !m_AutoRunModule.IsDisabled() )
					{
						if ( INPUT_FORWARD() || INPUT_BACK() || INPUT_LEFT() || INPUT_RIGHT() || INPUT_GETOVER() || INPUT_STANCE() )
						{
							m_AutoRunModule.AutoRun();
						}
					}
				}
						
				//! Data
				if ( !m_DataSent ) 
				{
					ExpansionPlayerData();
					m_DataSent = true;
				}
			}
		}
		
		if ( GetExpansionSettings() && GetExpansionSettings().GetGeneral().EnableHUDNightvisionOverlay )
		{
			PlayerCheckNV( playerPB );
		}
		
		//! Toggle HUD elements in different menus
		if ( m_Hud && m_ExpansionHud.IsInitialized() && m_Chat && GetCommunityOnlineTools() )
			RefreshHUDElements();
		
		//! Expansion hud update
		if ( m_Hud &&  m_ExpansionHud.IsInitialized() )
			m_ExpansionHud.Update( timeslice );
		
		//! Chat update
		m_Chat.Update( timeslice );
	}

}