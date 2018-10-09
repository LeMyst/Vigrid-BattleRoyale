

class NewGesturesMenu extends UIScriptedMenu
{
	const int KEGAN_1 = 1337;
	const int KEGAN_2 = 1338;
	const int KEGAN_3 = 1339;
	const int KEGAN_4 = 1340;
	const int KEGAN_5 = 1341;
	const int KEGAN_6 = 1342;
	const int KEGAN_7 = 1343;
	
	protected Widget 							m_GestureItemCardPanel;
	protected ref array<ref GestureMenuItem> 	m_GestureItems;
	
	//
	const string 								RADIAL_TEXT		= "RadialText";
	//selections
	protected Widget 							m_SelectedItem;
	protected bool 								m_IsCategorySelected;
	
	//instance
	static NewGesturesMenu							instance;
	
	
	protected MissionBase m_Base;
	
	
	
	//============================================
	// GesturesMenu
	//============================================
	void NewGesturesMenu(MissionBase base)
	{
		m_Base = base;
		m_GestureItems = new ref array<ref GestureMenuItem>;
		
		if ( !instance )
		{
			instance = this;
		}
	}
	
	void ~NewGesturesMenu()
	{
	}

	//============================================
	// Menu Controls
	//============================================	
	static void OpenMenu()
	{
		GetGame().GetUIManager().EnterScriptedMenu( MENU_GESTURES, NULL );
	}
	
	static void CloseMenu()
	{
		instance.OnReleaseExecute();
		
		GetGame().GetUIManager().Back();
	}
		
	
	
	//============================================
	// Init & Widget Events
	//============================================
	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "gui/layouts/radial_menu/radial_gestures/day_z_gestures.layout" );
		m_GestureItemCardPanel = layoutRoot.FindAnyWidget( RadialMenu.RADIAL_ITEM_CARD_CONTAINER );

		//register gestures menu
		RadialMenu.GetInstance().RegisterClass( this );
		
		//set radial menu properties
		RadialMenu.GetInstance().SetWidgetProperties( "gui/layouts/radial_menu/radial_gestures/day_z_gesture_delimiter.layout" );
		
		//create content (widgets) for items
		RefreshGestures();
		
		return layoutRoot;
	}
	
	
	//============================================
	// Gestures
	//============================================
	protected void RefreshGestures( GestureCategories category_id = -1 )
	{
		//create gestures content (widgets) based on categories
		if ( category_id > -1 )
		{
			GetGestureItems( m_GestureItems, category_id );
		}
		else
		{
#ifdef PLATFORM_XBOX
		GetGestureItems( m_GestureItems, GestureCategories.CONSOLE_GESTURES );
#else		
#ifdef PLATFORM_PS4
		GetGestureItems( m_GestureItems, GestureCategories.CONSOLE_GESTURES );
#else
		GetGestureItems( m_GestureItems, GestureCategories.CATEGORIES );
#endif
#endif
		}		
		CreateGestureContent();
	}		
	
	
	
	
	void CreateGestureContent()
	{
		//delete existing gesture widgets
		DeleteGestureItems();
		
		for ( int i = 0; i < m_GestureItems.Count(); ++i )
		{
			GestureMenuItem gesture_item = m_GestureItems.Get( i );
			
			//create item card
			Widget gesture_item_card_widget = Widget.Cast( GetGame().GetWorkspace().CreateWidgets( "gui/layouts/radial_menu/radial_gestures/day_z_gesture_item_card.layout", m_GestureItemCardPanel ) );
			gesture_item.SetRadialItemCard( gesture_item_card_widget );
			
			//set text
			TextWidget text_widget = TextWidget.Cast( gesture_item_card_widget.FindAnyWidget( RADIAL_TEXT ) );
			text_widget.SetText( gesture_item.GetName() );
			
			//set data
			gesture_item_card_widget.SetUserData( gesture_item );
		}
		
		//adjust radial parameters for content
		if ( m_GestureItems.Count() > 0 ) 
		{
			RadialMenu.GetInstance().AdjustRadialMenu( 0, 0.5, 0, 0.25, true );
		}		
		
		//refresh radial menu
		RadialMenu.GetInstance().Refresh();
	}
	
	void DeleteGestureItems()
	{
		Widget child;
		Widget child_to_destroy;
		
		child = m_GestureItemCardPanel.GetChildren();
		while ( child )
		{
			child_to_destroy = child;
			child = child.GetSibling();
			
			delete child_to_destroy;
		}		
	}
	
	
	
	
	
	void GetGestureItems( out ref array<ref GestureMenuItem> gesture_items, GestureCategories category )
	{
		gesture_items.Clear();
		
		//PC PLATFORM
		//All categories
		if ( category == GestureCategories.CATEGORIES )
		{
			gesture_items.Insert( new GestureMenuItem( GestureCategories.CATEGORY_1, "Comms", 	GestureCategories.CATEGORIES ) );
			gesture_items.Insert( new GestureMenuItem( GestureCategories.CATEGORY_2, "Taunts", 	GestureCategories.CATEGORIES ) );
			gesture_items.Insert( new GestureMenuItem( GestureCategories.CATEGORY_3, "Misc.", 	GestureCategories.CATEGORIES ) );
			gesture_items.Insert( new GestureMenuItem( GestureCategories.CATEGORY_4, "Regards", GestureCategories.CATEGORIES ) );
			//gesture_items.Insert( new GestureMenuItem( GestureCategories.CATEGORY_5, "Kegan", 	GestureCategories.CATEGORIES ) );
		}
		//Category 1 - comms
		else if ( category == GestureCategories.CATEGORY_1 )
		{
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TIMEOUT,	"Timeout", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_POINTSELF, 	"Point at Self",GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_WATCHING, 	"Watching", 	GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_HOLD, 		"Stop", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_LISTENING, 	"Listen", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SILENT, 	"Silent", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_POINT, 		"Point", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_MOVE, 		"Move", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_DOWN, 		"Down", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_COME, 		"Come", 		GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_NOD, 		"Nod Head", 	GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SHAKE, 		"Shake Head", 	GestureCategories.CATEGORY_1 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SHRUG, 		"Shrug", 		GestureCategories.CATEGORY_1 ) );
		}
		//Category 2 - taunt
		else if ( category == GestureCategories.CATEGORY_2 )
		{
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_LOOKATME, 	"Look at Me", 	GestureCategories.CATEGORY_2 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TAUNT, 		"Frig off", 	GestureCategories.CATEGORY_2 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TAUNTELBOW, "Taunt elbow", 	GestureCategories.CATEGORY_2 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_THROAT, 	"Die", 			GestureCategories.CATEGORY_2 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TAUNTTHINK, "Think!", 		GestureCategories.CATEGORY_2 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_FACEPALM, 	"Facepalm", 	GestureCategories.CATEGORY_2 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_THUMBDOWN, 	"Thumb Down", 	GestureCategories.CATEGORY_2 ) );
		}
		//Category 3 - misc
		else if ( category == GestureCategories.CATEGORY_3 )
		{
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_DANCE, 		"Dance", 		GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_LYINGDOWN, 	"Lay down", 	GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SOS, 		"Wave", 		GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_RPS, 		"RPS", 			GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SITA, 		"SIT_A", 		GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SITB, 		"SIT_B", 		GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SUICIDE, 	"Suicide", 		GestureCategories.CATEGORY_3 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_CAMPFIRE, 	"Camp?", 		GestureCategories.CATEGORY_3 ) );
			//gesture_items.Insert( new GestureMenuItem( ID_EMOTE_RPS_R, 	"RPS-Rock", 	GestureCategories.CATEGORY_3 ) );
			//gesture_items.Insert( new GestureMenuItem( ID_EMOTE_RPS_P, 	"RPS-Paper", 	GestureCategories.CATEGORY_3 ) );
			//gesture_items.Insert( new GestureMenuItem( ID_EMOTE_RPS_S, 	"RPS-Scisors", 	GestureCategories.CATEGORY_3 ) );
		}
		//Category 4 - regards
		else if ( category == GestureCategories.CATEGORY_4 )
		{
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_GREETING, 	"Hello", 		GestureCategories.CATEGORY_4 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_THUMB, 		"Thumbs up", 	GestureCategories.CATEGORY_4 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_HEART, 		"Heart", 		GestureCategories.CATEGORY_4 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_CLAP, 		"Clap", 		GestureCategories.CATEGORY_4 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TAUNTKISS, 	"Magnifique", 	GestureCategories.CATEGORY_4 ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SALUTE, 	"Salute", 		GestureCategories.CATEGORY_4 ) );
		}
		else if ( category == GestureCategories.CATEGORY_5 )
		{
			gesture_items.Insert( new GestureMenuItem( KEGAN_1, 	"AutoWalk", 		GestureCategories.CATEGORY_5 ) );
			gesture_items.Insert( new GestureMenuItem( KEGAN_2, 		"Add Energy", 	GestureCategories.CATEGORY_5 ) );
			gesture_items.Insert( new GestureMenuItem( KEGAN_3, 		"Suicide", 		GestureCategories.CATEGORY_5 ) );
			gesture_items.Insert( new GestureMenuItem( KEGAN_4, 		"Test BR Wall", 		GestureCategories.CATEGORY_5 ) );
			gesture_items.Insert( new GestureMenuItem( KEGAN_5, 	"Spectator", 	GestureCategories.CATEGORY_5 ) );
			gesture_items.Insert( new GestureMenuItem( KEGAN_6, 	"Item 6", 		GestureCategories.CATEGORY_5 ) );
			gesture_items.Insert( new GestureMenuItem( KEGAN_7, 	"Item 7", 		GestureCategories.CATEGORY_5 ) );
		}
		
		//CONSOLE PLATFORM ONLY
		//Only 1 category
		if ( category == GestureCategories.CONSOLE_GESTURES )
		{
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_GREETING, 	"Hello", 		GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_HEART, 		"Heart", 		GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SOS, 		"Wave", 		GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TAUNTKISS,	"Magnifique", 	GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_SUICIDE, 	"Suicide", 		GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_THROAT, 	"Die", 			GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_TAUNT, 		"Frig off", 	GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_DANCE, 		"Dance", 		GestureCategories.CONSOLE_GESTURES ) );
			gesture_items.Insert( new GestureMenuItem( ID_EMOTE_THUMB, 		"Thumbs up", 	GestureCategories.CONSOLE_GESTURES ) );
		}
	}
	
	
	void OnSelectionDeselect( Widget w )
	{
		m_SelectedItem = NULL;
		
		if ( w )
		{
			GestureMenuItem gesture_item;
			w.GetUserData( gesture_item );

			//is not category
			if ( gesture_item && gesture_item.GetCategory() != GestureCategories.CATEGORIES )
			{			
				//alter item visual
				TextWidget text_widget = TextWidget.Cast( gesture_item.GetRadialItemCard().FindAnyWidget( RADIAL_TEXT ) );
				text_widget.SetColor( ARGB( 255, 255, 255, 255 ) );
			}
			
			//Print("GesturesMenu->OnSelectionDeselect");			
		}
	}	
	
	void OnSelectionExecute( Widget w )
	{
		//only when category is not picked yet
		if ( w && !m_IsCategorySelected )
		{
			GestureMenuItem gesture_item;
			w.GetUserData( gesture_item );
			
			//is category
			if ( gesture_item.GetCategory() == GestureCategories.CATEGORIES )
			{
				//show selected category gestures
				GetGestureItems( m_GestureItems, gesture_item.GetID() );
				CreateGestureContent();
				RefreshGestures( gesture_item.GetID() );
				
				//set category selected
				m_IsCategorySelected = true;
			}
			
			//Print("GesturesMenu->OnSelectionExecuted");			
		}
	}
	void OnSelectionSelect( Widget w )
	{
		m_SelectedItem = w;
		
		if ( w )
		{
			GestureMenuItem gesture_item;
			w.GetUserData( gesture_item );

			//is not category
			if ( gesture_item && gesture_item.GetCategory() != GestureCategories.CATEGORIES )
			{	
				//alter item visual
				TextWidget text_widget = TextWidget.Cast( gesture_item.GetRadialItemCard().FindAnyWidget( RADIAL_TEXT ) );
				text_widget.SetColor( ARGB( 255, 66, 175, 95 ) );
			}

			//Print("GesturesMenu->OnSelectionSelect");			
		}
	}
	
	void OnReleaseExecute()
	{
		if ( m_SelectedItem )
		{
			if ( !GetGame().IsMultiplayer() || GetGame().IsClient() )
			{
				PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
				
				GestureMenuItem gesture_item;
				m_SelectedItem.GetUserData( gesture_item );
			
				if ( gesture_item ) 
				{
					if(gesture_item.GetCategory() != GestureCategories.CATEGORY_5)
					{
						if( player.GetEmoteManager() ) 
						{
							player.GetEmoteManager().CreateEmoteCBFromMenu( gesture_item.GetID() );
						}
					}
					else
					{
						int id = gesture_item.GetID();
						switch(id) {
							case KEGAN_1:
								m_Base.autowalk = !m_Base.autowalk;
								Param1<bool> rp = new Param1<bool>(m_Base.autowalk);
								
								if(m_Base.autowalk)
								{
									player.MessageStatus("Autowalk On");
									player.GetInputController().OverrideMovementSpeed( true, 2 );
									player.GetInputController().OverrideMovementAngle( true, 1 );
								}
								else
								{
									player.MessageStatus("Autowalk Off");
									player.GetInputController().OverrideMovementSpeed( false, 0 );
									player.GetInputController().OverrideMovementAngle( false, 0 );
								}
								
								GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "RequestAutowalk", rp, true, NULL, GetGame().GetPlayer() );
								break;
								
							case KEGAN_2:
								Param1<float> k2_rpc = new Param1<float>(100);
								
								GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "IncreaseStats", k2_rpc, true, NULL, GetGame().GetPlayer() );
								player.MessageStatus("Increased Energy/Water Ect.");
								break;
								
							case KEGAN_3:
								player.MessageStatus("Suicide");
								GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "RequestSuicide", NULL, true, NULL, GetGame().GetPlayer() );
								break;
							
							case KEGAN_4:
								player.MessageStatus("Test Wall");
								GetGame().CreateObject("Land_Br_Wall",player.GetPosition(),true);
								break;
							
							case KEGAN_5:
								GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "EnterSpectator", NULL, true, NULL, GetGame().GetPlayer() );
								break;
							
							case KEGAN_6:
								player.MessageStatus("Kegan 6");
								break;
							
							case KEGAN_7:
								player.MessageStatus("Kegan 7");
								break;
							
							
						}
					}
					
				}
			}
		}
	}
	
}
