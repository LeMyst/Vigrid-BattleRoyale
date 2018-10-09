modded class MissionGameplay
{
	ref BattleRoyale BR_GAME;
	
	void MissionGameplay()
	{
	}
	
	override void OnInit()
	{
		super.OnInit();
		
		BR_GAME = new BattleRoyale( NULL );
	}

	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		m_hud.KeyPress(key);
		
		
		if ( key == KeyCode.KC_PERIOD )
		{
			if ( !GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				
				NewGesturesMenu.OpenMenu();
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
	}
	
	override void OnUpdate(float timeslice)
	{
		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);
		TickScheduler(timeslice);
		UpdateDummyScheduler();//for external entities
		UIScriptedMenu menu = m_UIManager.GetMenu();
		InventoryMenuNew inventory = InventoryMenuNew.Cast( m_UIManager.FindMenu(MENU_INVENTORY) );
		//m_inventory_menu_new = inventory;
		InspectMenuNew inspect = InspectMenuNew.Cast( m_UIManager.FindMenu(MENU_INSPECT) );
		Input input = GetGame().GetInput();
		
		//TODO should be switchable
		if (playerPB && playerPB.enterNoteMenuRead)
		{
			playerPB.enterNoteMenuRead = false;
			Paper paper = Paper.Cast(playerPB.GetItemInHands());
			m_note = NoteMenu.Cast( GetUIManager().EnterScriptedMenu(MENU_NOTE, menu) ); //NULL means no parent
			m_note.InitRead(paper.m_AdvancedText);
		}
		
		if (playerPB && playerPB.enterNoteMenuWrite)
		{
			playerPB.enterNoteMenuWrite = false;
			m_note = NoteMenu.Cast( GetUIManager().EnterScriptedMenu(MENU_NOTE, menu) ); //NULL means no parent
			m_note.InitWrite(playerPB.m_paper,playerPB.m_writingImplement,playerPB.m_Handwriting);
		}

#ifdef PLATFORM_CONSOLE
		//Quick Reload Weapon
		if ( !menu && input.GetActionDown( UAQuickReload, false ) )
		{
			if ( !GetGame().IsInventoryOpen() && playerPB && !playerPB.GetActionManager().FindActionTarget().GetObject() )
			{
				EntityAI entity_hands = playerPB.GetHumanInventory().GetEntityInHands();
				
				if ( entity_hands && entity_hands.IsWeapon() )
				{
					playerPB.QuickReloadWeapon( entity_hands );
				}
			}
		}

		//Switch beween weapons in quickslots 
		if( !menu && input.GetActionDown( UAUIRadialMenuPick, false ) )
		{
			if ( !GetGame().IsInventoryOpen() )
			{
				EntityAI entity_in_hands = playerPB.GetHumanInventory().GetEntityInHands();
				EntityAI quickbar_entity;
				int quickbar_index = 0;
				
				if ( entity_in_hands )
				{			
					int quickbar_entity_hands_index = playerPB.FindQuickBarEntityIndex( entity_in_hands );
					
					if ( quickbar_entity_hands_index > -1 && quickbar_entity_hands_index < MAX_QUICKBAR_SLOTS_COUNT - 1 )	//(0->8)
					{
						quickbar_index = quickbar_entity_hands_index + 1;
					}
				}

				//find next weapon
				for ( int iter = 0; iter < MAX_QUICKBAR_SLOTS_COUNT; ++iter )
				{
					quickbar_entity = playerPB.GetQuickBarEntity( quickbar_index );
					
					if ( quickbar_entity && ( quickbar_entity.IsWeapon() || ( quickbar_entity.IsMeleeWeapon() && !quickbar_entity.IsMagazine() ) ) )
					{
						break;
					}
					
					quickbar_index += 1;
					if ( quickbar_index > MAX_QUICKBAR_SLOTS_COUNT - 1 )
					{
						quickbar_index = 0;	//reset
					}
				}
				
				//swap
				int slot_id;
				if ( quickbar_index > -1 )
				{
					slot_id = quickbar_index + 1;
					if ( slot_id == MAX_QUICKBAR_SLOTS_COUNT )
					{
						slot_id = 0;
					}
					
					playerPB.RadialQuickBarSingleUse( slot_id );
				}
			}
		}

		//Gestures
		if(input.GetActionDown(UAUIGesturesOpen, false))
		{
			//open gestures menu
			if ( !GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				GesturesMenu.OpenMenu();
			}
		}
		
		if(input.GetActionUp(UAUIGesturesOpen, false))
		{
			//close gestures menu
			if ( GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				GesturesMenu.CloseMenu();
			}
		}

		//Radial quickbar
		if(input.GetActionDown(UAUIQuickbarRadialOpen, false))
		{
			//open gestures menu
			if ( !GetUIManager().IsMenuOpen( MENU_RADIAL_QUICKBAR ) )
			{
				RadialQuickbarMenu.OpenMenu();
			}
		}
		
		if(input.GetActionUp(UAUIQuickbarRadialOpen, false))
		{
			//close gestures menu
			if ( GetUIManager().IsMenuOpen( MENU_RADIAL_QUICKBAR ) )
			{
				RadialQuickbarMenu.CloseMenu();
			}
		}
		
		//Radial Quickbar from inventory
		if( GetGame().GetInput().GetActionUp( UAUIQuickbarRadialInventoryOpen, false ) )
		{
			//close radial quickbar menu
			if ( GetGame().GetUIManager().IsMenuOpen( MENU_RADIAL_QUICKBAR ) )
			{
				RadialQuickbarMenu.CloseMenu();
				RadialQuickbarMenu.SetItemToAssign( NULL );
			}
		}		
#endif

		if (player && m_life_state == EPlayerStates.ALIVE && !player.IsUnconscious() )
		{
			// enables HUD on spawn
			if (m_hud_root_widget)
			{
				m_hud_root_widget.Show(true);
			}
			
		#ifndef NO_GUI
			// fade out black screen
			
			PlayerBaseClient playerPBC = PlayerBaseClient.Cast(player);
			
			if ( GetUIManager().ScreenFadeVisible() && !playerPBC.FORCE_FADE )
			{
				 GetUIManager().ScreenFadeOut(0.5);
			}
			
		#endif
		
			if( input.GetActionDown(UAGear, false ) )
			{
				if( !inventory )
				{
					if( GetGame().IsOldInventory() )
					{
						ShowInventory();
					}
					else
					{
						ShowInventory2();
					}
				}
				else if( menu == inventory )
				{
					HideInventory();
				}
			}

			if (input.GetActionDown(UAChat, false))
			{
				if (menu == NULL)
				{
					m_chat_channel_hide_timer.Stop();
					m_chat_channel_fade_timer.Stop();
					m_chat_channel_area.Show(false);
					m_UIManager.EnterScriptedMenu(MENU_CHAT_INPUT, NULL);
				}
			}
			
			if ( input.GetActionDown( UAUIQuickbarToggle, false) )
			{
				SetActionDownTime( GetGame().GetTime() );
				bool hud_state = m_hud.GetHudState();
				m_ToggleHudTimer.Run( 0.3, m_hud, "ToggleHud", new Param1<bool>( !hud_state ) );
			}
			
			if ( input.GetActionUp( UAUIQuickbarToggle, false) )
			{
				SetActionUpTime( GetGame().GetTime() );
				
				if ( GetHoldActionTime() < HOLD_LIMIT_TIME )
				{
					if ( menu == NULL )
					{
						if ( m_hud.GetQuickBarState() )
						{
							m_hud.HideQuickbar( false, true );
						}
						else
						{
							m_hud.ShowQuickbar();
						}
					}
				}
				
				m_ToggleHudTimer.Stop();
			}
			
			if ( g_Game.GetInput().GetActionDown( UAZeroingUp, false) || g_Game.GetInput().GetActionDown( UAZeroingDown, false) || g_Game.GetInput().GetActionDown( UAToggleWeapons, false) )
			{
				m_hud.ZeroingKeyPress();
			}
			
			if ( menu == NULL )
			{
				m_actionMenu.Refresh();
				
				if (input.GetActionDown(UANextAction, false))
				{
					m_actionMenu.NextAction();
				}
				
				if (input.GetActionDown(UAPrevAction, false))
				{
					m_actionMenu.PrevAction();
				}
			}
			else
			{
				m_actionMenu.Hide();
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
			if (m_life_state != life_state)
			{
				m_life_state = life_state;
				
				if (m_life_state != EPlayerStates.ALIVE)
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
					if(input.GetActionDown(UAUIBack, false) || input.GetActionDown(UAUIMenu, false))
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
				else if(input.GetActionDown(UAUIBack, false))
				{
					m_UIManager.Back();
				}
			}
			else if (input.GetActionDown(UAUIMenu, false))
			{
				Pause();
			}
		}
		
		UpdateDebugMonitor();
	}
}