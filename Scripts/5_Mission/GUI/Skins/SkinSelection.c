class SkinSelectionMenu extends UIScriptedMenu
{

    static void OpenMenu()
    {
        GetGame().GetUIManager().EnterScriptedMenu( DAYZBR_SKIN_SELECTION_MENU, NULL );
    }

    protected TextWidget m_Title;
    protected ButtonWidget m_ExitBtn;
    protected Widget m_ElementPanel;
    protected ItemPreviewWidget m_Preview;
    protected PlayerPreviewWidget m_PlayerPreview;
    protected Widget m_Window;

    protected Object m_PreviewObject;
    protected PlayerBase m_PreviewPlayer;

    protected ref array<ref SkinMap> m_Skins;


    private UIActionScroller m_sclr_MainActions;
    private Widget m_ActionsWrapper;

    void SkinSelectionMenu()
    {
        Print("Initializing Skin Objects!");
        m_Skins = new array<ref SkinMap>(); //THIS MUST BE INITIALIZED BEFORE ANYTYHING BELOW

        Print("Initializing TShirts");
        //--- TShirt skins
        m_Skins.Insert(new SkinMap("[Shirt] DBR v1", "TShirt_DBR", TShirt_ColorBase, "tshirt_dayzbr"));
        m_Skins.Insert(new SkinMap("[Shirt] DBR v2", "TShirt_DBRv2", TShirt_ColorBase, "tshirt_dayzbr_2"));
        m_Skins.Insert(new SkinMap("[Shirt] keganW", "TShirt_kegan", TShirt_ColorBase, "tshirt_keganhw"));
        m_Skins.Insert(new SkinMap("[Shirt] I <3 Kegan", "TShirt_KLove", TShirt_ColorBase, "tshirt_keganlove"));
        m_Skins.Insert(new SkinMap("[Shirt] DSR", "TShirt_DSR", TShirt_ColorBase, "tshirt_dsr"));
        m_Skins.Insert(new SkinMap("[Shirt] Chazie", "TShirt_Chazie", TShirt_ColorBase, "tshirt_chazie"));
        m_Skins.Insert(new SkinMap("[Shirt] Psi", "TShirt_psi", TShirt_ColorBase, "tshirt_psi"));
        m_Skins.Insert(new SkinMap("[Shirt] Gibs", "TShirt_gibs", TShirt_ColorBase, "tshirt_gibs"));
        m_Skins.Insert(new SkinMap("[Shirt] Cuddles", "TShirt_cc", TShirt_ColorBase, "tshirt_captcuddles"));
        m_Skins.Insert(new SkinMap("[Shirt] Septic", "TShirt_septic", TShirt_ColorBase, "tshirt_sceptic"));
        

        //--- default dayz skins (free)
        m_Skins.Insert(new SkinMap("[Shirt] Beige", "TShirt_Beige", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Black", "TShirt_Black", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Blue", "TShirt_Blue", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Green", "TShirt_Green", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Grey", "TShirt_Grey", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Orange Striped", "TShirt_OrangeWhiteStripes", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Red", "TShirt_Red", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] Red Striped", "TShirt_RedBlackStripes", TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] White", "TShirt_White", TShirt_ColorBase));

        //the dayz expansion developer T-Shirt
        m_Skins.Insert(new SkinMap("[Shirt] Expansion", "TShirt_DayZExpansion", TShirt_ColorBase, "dayz_exp_devs"));
        
        Print(m_Skins);
//--------- All TShirt skins above here! ----------------
        Print("Intializing Weapons");
        //--- AK74
        m_Skins.Insert(new SkinMap("[Gun] Bee", "DZBR_AK74_Bee", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rainbow", "DZBR_AK74_Rainbow", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Apple", "DZBR_AK74_Apple", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Glacier", "DZBR_AK74_Glacier", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Depth", "DZBR_AK74_Depth", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_AK74_FB_Bug", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_AK74_FB_Reward_White_Black", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_AK74_FB_Reward_White_Orange", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_AK74_FB_Reward_White_Blue", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_AK74_FB_Reward_White_Red", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_AK74_FB_Reward_White_Grey", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_AK74_FB_Reward_Black_Orange", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_AK74_FB_Reward_Black_Blue", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_AK74_FB_Reward_Black_Red", AK74, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_AK74_FB_Reward_Black_Grey", AK74, "todo_skins"));
        //--- AKS74U
        m_Skins.Insert(new SkinMap("[Gun] Bee", "DZBR_AKS74U_Bee", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rainbow", "DZBR_AKS74U_Rainbow", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rattlesnake", "DZBR_AKS74U_RattleSnake", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Tactical", "DZBR_AKS74U_Tactical", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_AKS74U_FB_Bug", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_AKS74U_FB_Reward_White_Black", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_AKS74U_FB_Reward_White_Orange", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_AKS74U_FB_Reward_White_Blue", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_AKS74U_FB_Reward_White_Red", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_AKS74U_FB_Reward_White_Grey", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_AKS74U_FB_Reward_Black_Orange", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_AKS74U_FB_Reward_Black_Blue", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_AKS74U_FB_Reward_Black_Red", AKS74U, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_AKS74U_FB_Reward_Black_Grey", AKS74U, "todo_skins"));
        //--- AK1010
        m_Skins.Insert(new SkinMap("[Gun] Bee", "DZBR_AK101_Bee", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rainbow", "DZBR_AK101_Rainbow", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Apple", "DZBR_AK101_Apple", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Glacier", "DZBR_AK101_Glacier", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Depth", "DZBR_AK101_Depth", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_AK101_FB_Bug", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_AK101_FB_Reward_White_Black", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_AK101_FB_Reward_White_Orange", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_AK101_FB_Reward_White_Blue", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_AK101_FB_Reward_White_Red", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_AK101_FB_Reward_White_Grey", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_AK101_FB_Reward_Black_Orange", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_AK101_FB_Reward_Black_Blue", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_AK101_FB_Reward_Black_Red", AK101, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_AK101_FB_Reward_Black_Grey", AK101, "todo_skins"));
        //--- AKM

        //--- FAL

        //--- M4

        //--- MOSIN9130

        //--- MOSIN9130_SAWEDOFF

        //--- MP5

        //--- SKS

        //--- UMP45

        //--- VSS

        //--- WINCHESTER70

        //--- 1911

        //--- DEAGLE

        //--- FNX45

        //--- GLOCK

        //--- IZH43

        //--- MP133

        //--- SAIGA

        //--- AWP

        //--- MP5A5

        //--- BENELIM4

        Print(m_Skins);
    }
    
    void ~SkinSelectionMenu()
    {
        CleanUpLocalObjects();
        

        delete m_Skins;
    }

    protected void CleanUpLocalObjects()
    {
        if ( m_PreviewObject )
        {
            GetGame().ObjectDelete( m_PreviewObject );
        }
        if(m_PreviewPlayer)
        {
            //not sure if this is correct ?
            GetGame().ObjectDelete( m_PreviewPlayer );
        }
    }

    override Widget Init()
	{
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("BattleRoyale/GUI/layouts/skin_selection.layout");

        m_Window = layoutRoot.FindAnyWidget("SkinSelectionWindow");
        m_ElementPanel = m_Window.FindAnyWidget("ElementPanel");
        m_Preview = ItemPreviewWidget.Cast( m_Window.FindAnyWidget("ItemPreview") );
        m_PlayerPreview = PlayerPreviewWidget.Cast( m_Window.FindAnyWidget("PlayerPreview") );
        m_ExitBtn = ButtonWidget.Cast( m_Window.FindAnyWidget("ExitButton") );
        m_Title = TextWidget.Cast( m_Window.FindAnyWidget("Title") );

        m_sclr_MainActions = UIActionManager.CreateScroller( m_ElementPanel );
        m_ActionsWrapper = m_sclr_MainActions.GetContentWidget();
        
        InitSkins();

        m_sclr_MainActions.UpdateScroller();

        return layoutRoot;
    }

    bool IsValidSkin(EntityAI item, ref SkinMap skin)
    {
        if(!item)
            return false;

        return skin.IsValidSkinForEntity(item);
    }

    void InitSkins()
    {
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();

        Widget wrapper;
        UIActionButton button;

        PlayerBase currentPlayer = PlayerBase.Cast( GetGame().GetPlayer() );
        HumanInventory inv = currentPlayer.GetInventory();

        EntityAI in_hands = currentPlayer.GetItemInHands();
        EntityAI shirt_slot = inv.FindAttachmentByName("Body");
        
        Print("InitSkins()");
        Print(in_hands);
        Print(shirt_slot);

        //list all owned skins (and the default skins)
        for(int i = 0; i < m_Skins.Count(); i++)
        {
            ref SkinMap skin = m_Skins[i];

            if(skin.GetFlag() == "" || api.HasPurchase(skin.GetFlag()))
            {
                if(IsValidSkin(in_hands, skin) || IsValidSkin(shirt_slot, skin))
                {
                    wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
                    
                    UIActionManager.CreateText( wrapper, skin.GetName() );
                    button = UIActionManager.CreateButton( wrapper, "Preview", this, "SkinPreview" );
                    button.SetUserData( skin );
                    
                    button = UIActionManager.CreateButton( wrapper, "Apply", this, "SkinApply" );
                    button.SetUserData( skin );
                }
            } 
            else
            {
                Print("player does not own flag: " + skin.GetFlag());
            }           
        }

        //Buy more skins at the shop button
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 2 );
        UIActionManager.CreateText( wrapper, "Want More Skins?" );
        button = UIActionManager.CreateButton( wrapper, "Visit The Shop!", this, "VisitShop" );

    }

    void SkinPreview(UIEvent eid, ref UIActionBase action)
    {
        Class skin;
        action.GetUserData( skin );

        ref SkinMap skin_map = SkinMap.Cast( skin );

        //preview skin_map  
        PreviewSkin(skin_map);      
    }   
    void VisitShop(UIEvent eid, ref UIActionBase action)
    {
        PlayerData web_data = BattleRoyaleAPI.GetAPI().GetCurrentPlayer();
        string player_id = web_data._id;
        GetGame().OpenURL("https://dayzbr.dev/shop/" + player_id);
    }
    void SkinApply(UIEvent eid, ref UIActionBase action)
    {
        Class skin;
        action.GetUserData( skin );

        ref SkinMap skin_map = SkinMap.Cast( skin );
        
        //verify the player can apply this skin!
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
        string skin_flag = skin_map.GetFlag();
        if(skin_flag != "" && !api.HasPurchase(skin_flag))
        {
            Error("SkinApply got called, but the player does not have access!");
            return;
        }

        //preview skin!
        PreviewSkin(skin_map);
        //apply skin!
        ApplySkin(skin_map);
        
    }

    protected void PreviewSkin(ref SkinMap skin)
    {
        if(skin.IsClothing())
        {
            Print("Previewing Clothing!");
            //preview shirt
            PreviewShirt(skin);
        }
        else
        {
            Print("Previewing Gun!");
            //preview gun
            PreviewGun(skin);
        }
    }
    protected void ApplySkin(ref SkinMap skin)
    {
        ref array<string> textures = skin.GetTextures();
        ref array<string> materials = skin.GetMaterials();
        Print("Applying Skin");
        Print(textures);
        Print(materials);
        if(skin.IsClothing())
        {
            BattleRoyaleClient.Cast( GetBR() ).SetSkin( 0, textures, materials ); //type 0 for shirt
        }
        else
        {
            BattleRoyaleClient.Cast( GetBR() ).SetSkin( 1, textures, materials ); //type 1 for gun
        }
    }

    protected void PreviewShirt(SkinMap skin)
    {

        CleanUpLocalObjects();

        string class_name = skin.GetClassName();
        Print("Preview Class: " + class_name)

        m_PreviewPlayer = PlayerBase.Cast( GetGame().CreatePlayer( NULL, GetGame().CreateRandomPlayer(), Vector( 0, 0, 0 ), 0, "NONE") );
        EntityAI item = m_PreviewPlayer.GetInventory().CreateInInventory( class_name );
        m_PreviewPlayer.GetInventory().CreateInInventory( "Jeans_Black" ); //TODO: get this from a setting file (match MissionServer )
        m_PreviewPlayer.GetInventory().CreateInInventory( "Sneakers_Black" );

       // skin.ApplyTo( item );

        m_PlayerPreview.SetPlayer( m_PreviewPlayer );
        m_PlayerPreview.SetModelPosition( Vector( 0, 0, 0.5 ) );
        m_PlayerPreview.SetModelOrientation( vector.Zero );

        m_PlayerPreview.Show( true );
        m_Preview.Show( false );
    }
    protected void PreviewGun(SkinMap skin)
    {
        CleanUpLocalObjects();

        string class_name = skin.GetClassName();
        Print("Preview Class: " + class_name)

        m_PreviewObject = GetGame().CreateObject( class_name, vector.Zero, true, false );

        EntityAI ent = EntityAI.Cast( m_PreviewObject );

       // skin.ApplyTo( ent );

        m_Preview.SetItem( ent );
        m_Preview.SetModelPosition( Vector( 0, 0, 0.5 ) );
        m_Preview.SetModelOrientation( vector.Zero );
        m_Preview.SetView( ent.GetViewIndex() );

        m_PlayerPreview.Show( false );
        m_Preview.Show( true );
    }


    void Show()
    {
        if(layoutRoot)
            layoutRoot.Show( true );
    }
    void Hide()
    {
        if(layoutRoot)
            layoutRoot.Show( false );
    }

    override void OnShow()
	{
		super.OnShow();

        SetFocus(layoutRoot);
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		
		PPEffects.SetBlurMenu(0.5);

        Show();
    }
    override void OnHide()
	{
		super.OnHide();
	
		GetGame().GetMission().PlayerControlEnable(true);
		
		PPEffects.SetBlurMenu( 0.0 );

        Hide();
    }

    override bool OnClick(Widget w, int x, int y, int button)
	{
		
        if(w == m_ExitBtn)
        {
            Hide();
            Close();
            return true;
        }

        return super.OnClick(w, x, y, button);
    }
}