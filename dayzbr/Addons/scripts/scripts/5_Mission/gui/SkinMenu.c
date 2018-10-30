enum TextureTypes
{
	MOSIN_Asiimov,
	MOSIN_Hunter,
	MOSIN_Desert,
	MOSIN_Digital,
	MOSIN_Dirt,
	MOSIN_Snow,
	MOSIN_Wood,
	
	M4_Asiimov,
	M4_Hunter,
	M4_Desert,
	M4_Digital,
	M4_Dirt,
	M4_Snow,
	M4_Wood,
	
	AKM_Asiimov,
	AKM_Hunter,
	AKM_Desert,
	AKM_Digital,
	AKM_Dirt,
	AKM_Snow,
	AKM_Wood,
	
	STANDARD_White,
	STANDARD_Red,
	STANDARD_Green,
	STANDARD_Blue,
	STANDARD_Black,
	
	SHIRT_Kegan,
	SHIRT_DBR,
	SHIRT_KLove,
	SHIRT_DSR,
	SHIRT_DBRv2,
	SHIRT_Gibs,
}

enum SkinCategories
{
	CATEGORIES,				//special category for texture selection
	MOSIN_SKIN,
	M4_SKIN,
	AKM_SKIN,
	STANDARD_SKIN,
	SHIRT_SKIN
}

class SkinMenuItem
{
	protected int 					m_ID;
	protected string 				m_Name;
	protected SkinCategories 	m_Category;
	protected bool m_Enabled;
	//radial menu
	protected Widget 				m_RadialMenuSelector;
	protected Widget				m_RadialMenuItemCard;
	
	void SkinMenuItem( int id, string name, SkinCategories category, bool enabled = true )
	{
		m_ID			= id;
		m_Name 			= name;
		m_Category 		= category;
		m_Enabled 		= enabled;
	}
	
	bool IsEnabled()
	{
		return m_Enabled;
	}
	string GetName()
	{
		return m_Name;
	}
	
	int GetID()
	{
		return m_ID;
	}
	
	SkinCategories GetCategory()
	{
		return m_Category;
	}
	
	Widget GetRadialItemCard()
	{
		return m_RadialMenuItemCard;
	}
	
	void SetRadialItemCard( Widget widget )
	{
		m_RadialMenuItemCard = widget;
	}
}


class SkinMenu extends UIScriptedMenu
{
	
	protected Widget 							m_SkinItemCardPanel;
	protected ref array<ref SkinMenuItem> 	m_SkinItems;
	
	//
	const string 								RADIAL_TEXT		= "RadialText";
	//selections
	protected Widget 							m_SelectedItem;
	protected bool 								m_IsCategorySelected;
	
	//instance
	static SkinMenu							instance;
	
	
	protected MissionBase m_Base;
	
	
	
	//============================================
	// SkinMenu
	//============================================
	void SkinMenu(MissionBase base)
	{
		m_Base = base;
		m_SkinItems = new ref array<ref SkinMenuItem>;
		
		if ( !instance )
		{
			instance = this;
		}
	}
	
	void ~SkinMenu()
	{
	}

	//============================================
	// Menu Controls
	//============================================	
	static void OpenMenu()
	{
		GetGame().GetUIManager().EnterScriptedMenu( MENU_SKINS, NULL );
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
		m_SkinItemCardPanel = layoutRoot.FindAnyWidget( RadialMenu.RADIAL_ITEM_CARD_CONTAINER );

		//register Skins menu
		RadialMenu.GetInstance().RegisterClass( this );
		
		//set radial menu properties
		RadialMenu.GetInstance().SetWidgetProperties( "gui/layouts/radial_menu/radial_gestures/day_z_gesture_delimiter.layout" );
		
		//create content (widgets) for items
		RefreshSkins();
		
		return layoutRoot;
	}
	
	
	//============================================
	// Skins
	//============================================
	protected void RefreshSkins( SkinCategories category_id = -1 )
	{
		//create Skins content (widgets) based on categories
		if ( category_id > -1 )
		{
			GetSkinItems( m_SkinItems, category_id );
		}
		else
		{
			GetSkinItems( m_SkinItems, SkinCategories.CATEGORIES );
		}		
		CreateSkinContent();
	}		
	
	
	
	
	void CreateSkinContent()
	{
		//delete existing Skin widgets
		DeleteSkinItems();
		
		for ( int i = 0; i < m_SkinItems.Count(); ++i )
		{
			SkinMenuItem Skin_item = m_SkinItems.Get( i );
			
			//create item card
			Widget skin_item_card_widget = Widget.Cast( GetGame().GetWorkspace().CreateWidgets( "gui/layouts/radial_menu/radial_gestures/day_z_gesture_item_card.layout", m_SkinItemCardPanel ) );
			Skin_item.SetRadialItemCard( skin_item_card_widget );
			
			//set text
			TextWidget text_widget = TextWidget.Cast( skin_item_card_widget.FindAnyWidget( RADIAL_TEXT ) );
			text_widget.SetText( Skin_item.GetName() );
			
			//set enabled
			text_widget.Enable(Skin_item.IsEnabled());
			
			//set data
			skin_item_card_widget.SetUserData( Skin_item );
		}
		
		//adjust radial parameters for content
		if ( m_SkinItems.Count() > 0 ) 
		{
			RadialMenu.GetInstance().AdjustRadialMenu( 0, 0.5, 0, 0.25, true );
		}		
		
		//refresh radial menu
		RadialMenu.GetInstance().Refresh();
	}
	
	void DeleteSkinItems()
	{
		Widget child;
		Widget child_to_destroy;
		
		child = m_SkinItemCardPanel.GetChildren();
		while ( child )
		{
			child_to_destroy = child;
			child = child.GetSibling();
			
			delete child_to_destroy;
		}		
	}
	
	
	
	
	
	void GetSkinItems( out ref array<ref SkinMenuItem> Skin_items, SkinCategories category )
	{
		Skin_items.Clear();
		
		//All categories
		if ( category == SkinCategories.CATEGORIES )
		{
			//Skin Categories (different items you can skin on your person)
			Skin_items.Insert( new SkinMenuItem( SkinCategories.MOSIN_SKIN, "Mosin",	SkinCategories.CATEGORIES, IsMosin() ) );
			Skin_items.Insert( new SkinMenuItem( SkinCategories.M4_SKIN, "M4", 	SkinCategories.CATEGORIES, IsM4() ) );
			Skin_items.Insert( new SkinMenuItem( SkinCategories.AKM_SKIN, "AKM", 	SkinCategories.CATEGORIES, IsAKM() ) );
			Skin_items.Insert( new SkinMenuItem( SkinCategories.STANDARD_SKIN, "Standard", SkinCategories.CATEGORIES ) );
			Skin_items.Insert( new SkinMenuItem( SkinCategories.SHIRT_SKIN, "Shirt", 	SkinCategories.CATEGORIES ) );
		}
		//Category 1 - MOSIN_SKIN
		else if ( category == SkinCategories.MOSIN_SKIN )
		{
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Asiimov,	"Asiimov", 		SkinCategories.MOSIN_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Hunter, 	"Hunter",SkinCategories.MOSIN_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Desert, 	"Desert", 	SkinCategories.MOSIN_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Digital, 		"Digital", 		SkinCategories.MOSIN_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Dirt, 	"Dirt", 		SkinCategories.MOSIN_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Snow, 	"Snow", 		SkinCategories.MOSIN_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.MOSIN_Wood, 		"Wood", 		SkinCategories.MOSIN_SKIN ) );
		}
		//Category 2 - M4_SKIN
		else if ( category == SkinCategories.M4_SKIN )
		{
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Asiimov, 	"Asiimov", 	SkinCategories.M4_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Hunter, 		"Hunter", 	SkinCategories.M4_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Desert, "Desert", 	SkinCategories.M4_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Digital, 	"Digital", 			SkinCategories.M4_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Dirt, "Dirt", 		SkinCategories.M4_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Snow, 	"Snow", 	SkinCategories.M4_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.M4_Wood, 	"Wood", 	SkinCategories.M4_SKIN ) );
		}
		//Category 3 - AKM_SKIN
		else if ( category == SkinCategories.AKM_SKIN )
		{
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Asiimov, 		"Asiimov", 		SkinCategories.AKM_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Hunter, 	"Hunter", 	SkinCategories.AKM_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Desert, 		"Desert", 		SkinCategories.AKM_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Digital, 		"Digital", 			SkinCategories.AKM_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Dirt, 		"Dirt", 		SkinCategories.AKM_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Snow, 		"Snow", 		SkinCategories.AKM_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.AKM_Wood, 	"Wood", 		SkinCategories.AKM_SKIN ) );
		}
		//Category 4 - STANDARD_SKIN
		else if ( category == SkinCategories.STANDARD_SKIN )
		{
			Skin_items.Insert( new SkinMenuItem( TextureTypes.STANDARD_White, 	"White", 		SkinCategories.STANDARD_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.STANDARD_Red, 		"Red", 	SkinCategories.STANDARD_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.STANDARD_Green, 		"Green", 		SkinCategories.STANDARD_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.STANDARD_Blue, 		"Blue", 		SkinCategories.STANDARD_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.STANDARD_Black, 	"Black", 	SkinCategories.STANDARD_SKIN ) );
		}
		//Category 5 - SHIRT_SKIN
		else if ( category == SkinCategories.SHIRT_SKIN )
		{
			Skin_items.Insert( new SkinMenuItem( TextureTypes.SHIRT_Kegan, 	"Kegan", 		SkinCategories.SHIRT_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.SHIRT_DBR, 		"DayZBR", 	SkinCategories.SHIRT_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.SHIRT_KLove, 		"Kegan Love", 		SkinCategories.SHIRT_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.SHIRT_DSR, 		"Desolation", 		SkinCategories.SHIRT_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.SHIRT_DBRv2, 	"DayZBR v2", 	SkinCategories.SHIRT_SKIN ) );
			Skin_items.Insert( new SkinMenuItem( TextureTypes.SHIRT_Gibs, 	"GibsAndPieces", 		SkinCategories.SHIRT_SKIN ) );
		}
	}
	
	
	void OnSelectionDeselect( Widget w )
	{
		m_SelectedItem = NULL;
		
		if ( w )
		{
			SkinMenuItem Skin_item;
			w.GetUserData( Skin_item );

			//is not category
			if ( Skin_item && Skin_item.GetCategory() != SkinCategories.CATEGORIES )
			{			
				//alter item visual
				TextWidget text_widget = TextWidget.Cast( Skin_item.GetRadialItemCard().FindAnyWidget( RADIAL_TEXT ) );
				text_widget.SetColor( ARGB( 255, 255, 255, 255 ) );
			}
			
			//Print("SkinsMenu->OnSelectionDeselect");			
		}
	}	
	
	void OnSelectionExecute( Widget w )
	{
		//only when category is not picked yet
		if ( w && !m_IsCategorySelected )
		{
			SkinMenuItem Skin_item;
			w.GetUserData( Skin_item );
			
			//is category & this category is enabled
			if ( Skin_item.GetCategory() == SkinCategories.CATEGORIES && Skin_item.IsEnabled() )
			{
				//show selected category Skins
				GetSkinItems( m_SkinItems, Skin_item.GetID() );
				CreateSkinContent();
				RefreshSkins( Skin_item.GetID() );
				
				//set category selected
				m_IsCategorySelected = true;
			}
			
			//Print("SkinsMenu->OnSelectionExecuted");			
		}
	}
	void OnSelectionSelect( Widget w )
	{
		m_SelectedItem = w;
		
		if ( w )
		{
			SkinMenuItem Skin_item;
			w.GetUserData( Skin_item );

			//is not category
			if ( Skin_item && Skin_item.GetCategory() != SkinCategories.CATEGORIES )
			{	
				//alter item visual
				TextWidget text_widget = TextWidget.Cast( Skin_item.GetRadialItemCard().FindAnyWidget( RADIAL_TEXT ) );
				//alter based on enabled state?
				text_widget.SetColor( ARGB( 255, 66, 175, 95 ) );
			}

			//Print("SkinsMenu->OnSelectionSelect");			
		}
	}
	string GetTexture(TextureTypes tType)
	{
		string texture = "#(argb,8,8,3)color(1,1,1,1,ca)";
		switch(tType)
		{
			//Standard colors
			case TextureTypes.STANDARD_Red:
				texture = "#(argb,8,8,3)color(1,0,0,1,ca)";
				break;
			case TextureTypes.STANDARD_Green:
				texture = "#(argb,8,8,3)color(0,1,0,1,ca)";
				break;
			case TextureTypes.STANDARD_Blue:
				texture = "#(argb,8,8,3)color(0,0,1,1,ca)";
				break;
			case TextureTypes.STANDARD_Black:
				texture = "#(argb,8,8,3)color(0,0,0,1,ca)";
				break;
			case TextureTypes.STANDARD_White:
				texture = "#(argb,8,8,3)color(1,1,1,1,ca)";
				break;
				
			//Shirt Colors
			case TextureTypes.SHIRT_DBR:
				texture = "dayzbr\\assets\\textures\\shirt\\tshirt_black_DBR.paa";
				break;
			case TextureTypes.SHIRT_DBRv2:
				texture = "dayzbr\\assets\\textures\\shirt\\tshirt_black_DBRv2.paa";
				break;
			case TextureTypes.SHIRT_DSR:
				texture = "dayzbr\\assets\\textures\\shirt\\tshirt_white_DSR.paa";
				break;
			case TextureTypes.SHIRT_Gibs:
				texture = "dayzbr\\assets\\textures\\shirt\\tshirt_orange_gibs.paa";
				break;
			case TextureTypes.SHIRT_KLove:
				texture = "dayzbr\\assets\\textures\\shirt\\tshirt_black_KLove.paa";
				break;
			case TextureTypes.SHIRT_Kegan:
				texture = "dayzbr\\assets\\textures\\shirt\\tshirt_black_kegan.paa";
				break;
				
			//Mosin Colors
			case TextureTypes.MOSIN_Asiimov:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_asiimov.paa";
				break;
			case TextureTypes.MOSIN_Desert:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_desertcamo.paa";
				break;
			case TextureTypes.MOSIN_Digital:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_digicamo.paa";
				break;
			case TextureTypes.MOSIN_Dirt:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_dirtcamo.paa";
				break;
			case TextureTypes.MOSIN_Hunter:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_camo.paa";
				break;
			case TextureTypes.MOSIN_Snow:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_snow.paa";
				break;
			case TextureTypes.MOSIN_Wood:
				texture = "dayzbr\\assets\\textures\\mosin\\mosin_9130_woodcamo.paa";
				break;
				
			//AKM Colors
			case TextureTypes.AKM_Asiimov:
				texture = "dayzbr\\assets\\textures\\akm\\akm_asiimov.paa";
				break;
			case TextureTypes.AKM_Desert:
				texture = "dayzbr\\assets\\textures\\akm\\akm_desertcamo.paa";
				break;
			case TextureTypes.AKM_Digital:
				texture = "dayzbr\\assets\\textures\\akm\\akm_digicamo.paa";
				break;
			case TextureTypes.AKM_Dirt:
				texture = "dayzbr\\assets\\textures\\akm\\akm_dirtcamo.paa";
				break;
			case TextureTypes.AKM_Hunter:
				texture = "dayzbr\\assets\\textures\\akm\\akm_camo.paa";
				break;
			case TextureTypes.AKM_Snow:
				texture = "dayzbr\\assets\\textures\\akm\\akm_snowcamo.paa";
				break;
			case TextureTypes.AKM_Wood:
				texture = "dayzbr\\assets\\textures\\akm\\akm_woodcamo.paa";
				break;
				
			//M4 Colors
			case TextureTypes.M4_Asiimov:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_asiimov.paa";
				break;
			case TextureTypes.M4_Desert:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_desertcamo.paa";
				break;
			case TextureTypes.M4_Digital:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_digicamo.paa";
				break;
			case TextureTypes.M4_Dirt:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_dirtcamo.paa";
				break;
			case TextureTypes.M4_Hunter:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_camo.paa";
				break;
			case TextureTypes.M4_Snow:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_snowcamo.paa";
				break;
			case TextureTypes.M4_Wood:
				texture = "dayzbr\\assets\\textures\\m4\\m4_body_woodcamo.paa";
				break;
		}
		
		return texture;
	}
	
	bool IsMosin()
	{
		PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
		if(!player)
			return false;
		
		HumanInventory inv = player.GetHumanInventory();
		if(!inv)
			return false;
		
		EntityAI itemInHands = inv.GetEntityInHands();
		if(!itemInHands)
			return false;
		
		Mosin9130 result;
		if(Mosin9130.CastTo(result, itemInHands))
		{
			return true;
		}
		return false;
	}
	bool IsM4()
	{
		PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
		if(!player)
			return false;
		
		HumanInventory inv = player.GetHumanInventory();
		if(!inv)
			return false;
		
		EntityAI itemInHands = inv.GetEntityInHands();
		if(!itemInHands)
			return false;
		
		M4A1 result;
		if(M4A1.CastTo(result, itemInHands))
		{
			return true;
		}
		return false;
	}
	bool IsAKM()
	{
		PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
		if(!player)
			return false;
		
		HumanInventory inv = player.GetHumanInventory();
		if(!inv)
			return false;
		
		EntityAI itemInHands = inv.GetEntityInHands();
		if(!itemInHands)
			return false;
		
		AKM result;
		if(AKM.CastTo(result, itemInHands))
		{
			return true;
		}
		return false;
	}
	
	void OnReleaseExecute()
	{
		if ( m_SelectedItem )
		{
			if ( !GetGame().IsMultiplayer() || GetGame().IsClient() )
			{
				PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
				
				if(player) 
				{
					SkinMenuItem Skin_item;
					m_SelectedItem.GetUserData( Skin_item );
				
					if ( Skin_item ) 
					{
						HumanInventory inv = player.GetHumanInventory();
						if(inv)
						{
							if(Skin_item.GetCategory() != SkinCategories.SHIRT_SKIN)
							{
								
									//set weapon in hand texture
									EntityAI itemInHands = inv.GetEntityInHands();
									if(itemInHands) 
									{
										ref Param1<string> value_string = new Param1<string>(GetTexture(Skin_item.GetID()));
										GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetWeaponTexture", value_string, false , NULL, player);
									}
							}
							else
							{
								EntityAI shirt = inv.FindAttachment(InventorySlots.BODY);
								if(shirt)
								{
									ref Param1<string> shirt_value = new Param1<string>(GetTexture(Skin_item.GetID()));
									GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetShirtTexture", shirt_value, false , NULL, player);
								}
							}
						
						}
					}
				}
			}
		}
	}
	
}
