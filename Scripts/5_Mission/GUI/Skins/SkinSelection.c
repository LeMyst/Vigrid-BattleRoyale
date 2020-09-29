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

        m_Skins = new array<ref SkinMap>(); //THIS MUST BE INITIALIZED BEFORE ANYTYHING BELOW

        //--- TShirt skins
        InsertBRTee("DBR v1", "tshirt_black_DBR.paa", "tshirt_black_DBR.paa", "tshirt_dayzbr"); //TODO: add a custom ground texture (param # 1)
        InsertBRTee("DBR v2", "tshirt_black_DBRv2.paa", "tshirt_black_DBRv2.paa", "tshirt_dayzbr_2");
        InsertBRTee("keganW", "tshirt_black_kegan.paa", "tshirt_black_kegan.paa", "tshirt_keganhw");
        InsertBRTee("I <3 Kegan", "tshirt_black_KLove.paa", "tshirt_black_KLove.paa", "tshirt_keganlove");
        InsertBRTee("DSR", "tshirt_white_DSR.paa", "tshirt_white_DSR.paa", "tshirt_dsr");
        InsertBRTee("Chazie", "tshirt_black_Chazie.paa", "tshirt_black_Chazie.paa", "tshirt_chazie");
        InsertBRTee("Psi", "tshirt_black_psi.paa", "tshirt_black_psi.paa", "tshirt_psi");
        InsertBRTee("Gibs", "tshirt_orange_gibs.paa", "tshirt_orange_gibs.paa", "tshirt_gibs");
        InsertBRTee("Cuddles", "tshirt_grey_cc.paa", "tshirt_grey_cc.paa", "tshirt_captcuddles");
        InsertBRTee("Septic", "tshirt_white_septic.paa", "tshirt_white_septic.paa", "tshirt_sceptic");

        //--- default dayz skins (free)
        string DAYZ_TEE_PATH = "DZ\\characters\\tops\\data\\";
        InsertDayZTee("Beige", DAYZ_TEE_PATH + "tshirt_ground_beige_co.paa", DAYZ_TEE_PATH + "tshirt_beige_co.paa");
        InsertDayZTee("Black", DAYZ_TEE_PATH + "tshirt_ground_black_co.paa", DAYZ_TEE_PATH + "tshirt_black_co.paa");
        InsertDayZTee("Blue", DAYZ_TEE_PATH + "tshirt_ground_blue_co.paa", DAYZ_TEE_PATH + "tshirt_blue_co.paa");
        InsertDayZTee("Green", DAYZ_TEE_PATH + "tshirt_ground_green_co.paa", DAYZ_TEE_PATH + "tshirt_green_co.paa");
        InsertDayZTee("Grey", DAYZ_TEE_PATH + "tshirt_ground_grey_co.paa", DAYZ_TEE_PATH + "tshirt_grey_co.paa");
        InsertDayZTee("Orange Striped", DAYZ_TEE_PATH + "tshirt_orangewhitestripes_beige_co.paa", DAYZ_TEE_PATH + "tshirt_orangewhitestripes_co.paa");
        InsertDayZTee("Red", DAYZ_TEE_PATH + "tshirt_ground_red_co.paa", DAYZ_TEE_PATH + "tshirt_red_co.paa");
        InsertDayZTee("Red Striped", DAYZ_TEE_PATH + "tshirt_ground_redblackstripes_co.paa", DAYZ_TEE_PATH + "tshirt_redblackstripes_co.paa");
        InsertDayZTee("White", DAYZ_TEE_PATH + "tshirt_ground_white_co.paa", DAYZ_TEE_PATH + "tshirt_white_co.paa");
    
        //the dayz expansion developer T-Shirt
        ref SkinMap expansion_tee = new SkinMap();
        ref array<string> textures = {
            "DayZExpansion\\Data\\Characters\\Tops\\Data\\expansion_shirt.paa",
            "DayZExpansion\\Data\\Characters\\Tops\\Data\\expansion_shirt.paa"
        };
        expansion_tee.Init("[Shirt] DZ Exp", textures, "TShirt_White", TShirt_ColorBase, "dayz_exp_devs");
        m_Skins.Insert( expansion_tee );

//--------- All TShirt skins above here! ----------------

        //--- AKM skins
        InsertBRGun("Mango", "akm_mango.paa", "AKM", AKM_Base, "");

        //--- M4 skins
        InsertBRGun("Mango", "m4_body_mango.paa", "M4A1", M4A1_Base, "");

        //--- Mosin skins
        InsertBRGun("Mango", "mosin_9130_mango", "Mosin9130", Mosin9130_Base, "");
    }

    protected void InsertBRGun(string name, string body_texture, string preview_item, typename gun_class, string shop_entry = "")
    {
        ref DayZBRGunSkinMap gun = new DayZBRGunSkinMap();
        gun.InitGun( "[Gun] " + name, body_texture, preview_item, gun_class, shop_entry );
        m_Skins.Insert( gun );
    }
    protected void InsertDayZTee(string name, string ground_texture, string shirt_texture)
    {
        ref DayZTSkinMap tee = new DayZTSkinMap();
        tee.InitTee("[Shirt] " + name, ground_texture, shirt_texture);
        m_Skins.Insert( tee );
    }
    protected void InsertBRTee(string name, string ground_texture, string shirt_texture, string item_name)
    {
        ref DayZBRTSkinMap tee = new DayZBRTSkinMap();
        tee.InitTee("[Shirt] " + name, ground_texture, shirt_texture, item_name);
        m_Skins.Insert( tee );
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

        GunSkinMap gun_skin;
        if(skin_map.CastTo(gun_skin, skin_map))
        {
            Print("Previewing gun!");
            PreviewGun(gun_skin);
        }
        else
        {
            PreviewShirt( skin_map );
        }
        
        
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
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
        GunSkinMap gun_skin;
        if(skin_map.CastTo(gun_skin, skin_map))
        {
            Print("Applying Gun!");
            PreviewGun(gun_skin);

            if((gun_skin.GetFlag() == "" || api.HasPurchase(gun_skin.GetFlag()))
            {
                ApplyGun( gun_skin );
            }
            else
            {
                Error("SkinApply got called, but the player does not have access!");
            } 
        }
        else
        {
           
            //not a gun, must be a shirt
            PreviewShirt( skin_map );

            if((skin_map.GetFlag() == "" || api.HasPurchase(skin_map.GetFlag()))
            {
                ApplyShirt( skin_map );
            }
            else
            {
                Error("SkinApply got called, but the player does not have access!");
            } 
        }
        
    }

    protected void ApplyGun(GunSkinMap skin)
    {
        string texture = skin.GetTexture();
        Print(texture);
        BattleRoyaleClient.Cast( GetBR() ).SetGun( texture );
    }
    protected void ApplyShirt(SkinMap skin)
    {
        BattleRoyaleClient.Cast( GetBR() ).SetShirt( skin.GetTexture( 0 ), skin.GetTexture( 1 ) );
    }
    protected void PreviewShirt(SkinMap skin)
    {

        CleanUpLocalObjects();

        m_PreviewPlayer = PlayerBase.Cast( GetGame().CreatePlayer( NULL, GetGame().CreateRandomPlayer(), Vector( 0, 0, 0 ), 0, "NONE") );
        EntityAI item = m_PreviewPlayer.GetInventory().CreateInInventory( skin.GetClassName() );
        m_PreviewPlayer.GetInventory().CreateInInventory( "Jeans_Black" ); //TODO: get this from a setting file (match MissionServer )
        m_PreviewPlayer.GetInventory().CreateInInventory( "Sneakers_Black" );

        item.SetObjectTexture(0,skin.GetTexture( 0 )); //0 is for ground
        item.SetObjectTexture(1,skin.GetTexture( 1 )); //1 is for male
        item.SetObjectTexture(2,skin.GetTexture( 1 )); //1 is for female

        m_PlayerPreview.SetPlayer( m_PreviewPlayer );
        m_PlayerPreview.SetModelPosition( Vector( 0, 0, 0.5 ) );
        m_PlayerPreview.SetModelOrientation( vector.Zero );

        m_PlayerPreview.Show( true );
        m_Preview.Show( false );
    }
    protected void PreviewGun(GunSkinMap skin)
    {
        CleanUpLocalObjects();

        m_PreviewObject = GetGame().CreateObject( skin.GetClassName() , vector.Zero, true, false );

        EntityAI ent = EntityAI.Cast( m_PreviewObject );
        ent.SetObjectTexture(0, skin.GetTexture() );//body

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