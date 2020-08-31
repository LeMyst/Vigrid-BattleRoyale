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

        m_Skins = new array<ref SkinMap>();

        //TODO: somehow config these so it's easier to add in the future
        //TODO: only show skins that can actively be applied (check current shirt to see if it's a Tshirt)
        m_Skins.Insert(new SkinMap("DBR v1", "tshirt_black_DBR.paa", "TShirt_White", "tshirt_dayzbr")); 
        m_Skins.Insert(new SkinMap("DBR v2", "tshirt_black_DBRv2.paa", "TShirt_White", "tshirt_dayzbr_2")); 
        m_Skins.Insert(new SkinMap("keganW", "tshirt_black_kegan.paa", "TShirt_White", "tshirt_keganhw")); 
        m_Skins.Insert(new SkinMap("I <3 Kegan", "tshirt_black_KLove.paa", "TShirt_White", "tshirt_keganlove")); 
        m_Skins.Insert(new SkinMap("DSR", "tshirt_white_DSR.paa", "TShirt_White", "tshirt_dsr")); 
        m_Skins.Insert(new SkinMap("Chazie", "tshirt_black_Chazie.paa", "TShirt_White", "tshirt_chazie")); 
        m_Skins.Insert(new SkinMap("Psi", "tshirt_black_psi.paa", "TShirt_White", "tshirt_psi")); 
        m_Skins.Insert(new SkinMap("Cuddles", "tshirt_grey_cc.paa", "TShirt_White", "tshirt_captcuddles")); 
        m_Skins.Insert(new SkinMap("Gibs", "tshirt_orange_gibs.paa", "TShirt_White", "tshirt_gibs")); 
        m_Skins.Insert(new SkinMap("Septic", "tshirt_white_septic.paa", "TShirt_White", "tshirt_sceptic")); 
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
        
        m_sclr_MainActions.UpdateScroller();

        InitSkins();

        return layoutRoot;
    }

    void InitSkins()
    {
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();

        for(int i = 0; i < m_Skins.Count(); i++)
        {
            ref SkinMap skin = m_Skins[i];
            Widget wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
            
            UIActionManager.CreateText( wrapper, skin.GetName() );
            UIActionButton button = UIActionManager.CreateButton( wrapper, "Preview", this, "SkinPreview" );
            button.SetUserData( skin );
            string method = "SkinApplyNotOwned";
            string text = "Visit Shop";

            
            if(api.HasPurchase(skin.GetFlag()))
            {
                method = "SkinApply";
                text = "Apply";
            }


            button = UIActionManager.CreateButton( wrapper, text, this, method );
            button.SetUserData( skin );
        }
    }

    void SkinPreview(UIEvent eid, ref UIActionBase action)
    {
        Class skin;
        action.GetUserData( skin );

        PreviewShirt(SkinMap.Cast( skin ));
    }   
    void SkinApplyNotOwned(UIEvent eid, ref UIActionBase action)
    {
        PlayerData web_data = BattleRoyaleAPI.GetAPI().GetCurrentPlayer();
        string player_id = web_data._id; //TODO: get player id from web api
        GetGame().OpenURL("https://dayzbr.dev/shop/" + player_id);
    }
    void SkinApply(UIEvent eid, ref UIActionBase action)
    {
        Class skin;
        action.GetUserData( skin );

        PreviewShirt(SkinMap.Cast( skin ));

        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
        if(api.HasPurchase(skin.GetFlag()))
        {
            ApplyShirt(SkinMap.Cast( skin ));
        }
        else
        {
            Error("SkinApply got called, but the player is not a Patron!");
        }
        
    }

    protected void ApplyShirt(SkinMap skin)
    {
        string skin_texture = skin.GetTexture( BATTLEROYALE_TSHIRT_SKINS_PATH ); 

        BattleRoyaleClient.Cast( GetBR() ).SetShirt( skin_texture );
    }
    protected void PreviewShirt(SkinMap skin)
    {
        string skin_texture = skin.GetTexture( BATTLEROYALE_TSHIRT_SKINS_PATH ); 

        CleanUpLocalObjects();

        m_PreviewPlayer = PlayerBase.Cast( GetGame().CreatePlayer( NULL, GetGame().CreateRandomPlayer(), Vector( 0, 0, 0 ), 0, "NONE") );
        EntityAI item = m_PreviewPlayer.GetInventory().CreateInInventory( skin.GetClassName() );
        m_PreviewPlayer.GetInventory().CreateInInventory( "Jeans_Black" ); //TODO: get this from a setting file (match MissionServer )
        m_PreviewPlayer.GetInventory().CreateInInventory( "Sneakers_Black" );

        item.SetObjectTexture(0,skin_texture);
        item.SetObjectTexture(1,skin_texture);
        item.SetObjectTexture(2,skin_texture);

        m_PlayerPreview.SetPlayer( m_PreviewPlayer );
        m_PlayerPreview.SetModelPosition( Vector( 0, 0, 0.5 ) );
        m_PlayerPreview.SetModelOrientation( vector.Zero );

        m_PlayerPreview.Show( true );
        m_Preview.Show( false );
    }
    protected void PreviewItem(SkinMap skin)
    {
        //unused but will be for weapon skins
        string skin_texture = skin.GetTexture( /*BATTLEROYALE_TSHIRT_SKINS_PATH*/ ); 

        CleanUpLocalObjects();

        m_PreviewObject = GetGame().CreateObject( skin.GetClassName() , vector.Zero, true, false );

        EntityAI ent = EntityAI.Cast( m_PreviewObject );
        ent.SetObjectTexture(0,skin_texture);
        ent.SetObjectTexture(1,skin_texture);
        ent.SetObjectTexture(2,skin_texture);

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


class SkinMap
{
    protected string s_DisplayName;
    protected string s_TexturePath;
    protected string s_ItemClass;
    protected string s_ShopFlag;

    void SkinMap(string name, string texture, string item, string flag)
    {
        s_DisplayName = name;
        s_TexturePath = texture;
        s_ItemClass = item;
        s_ShopFlag = flag;
    }

    string GetFlag()
    {
        return s_ShopFlag;
    }

    string GetName()
    {
        return s_DisplayName;
    }
    string GetClassName()
    {
        return s_ItemClass;
    }
    string GetTexture(string base_path = "")
    {
        Print("Getting Texture For Skin: " + GetName());
        string full_path = s_TexturePath;
        if(base_path != "")
        {
            Print("Base Path Provided");
            full_path = base_path + s_TexturePath;
        }
        Print(full_path);
        return full_path;
    }
}