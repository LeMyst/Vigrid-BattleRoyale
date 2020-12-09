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
        
        //--- free shirts :)
        m_Skins.Insert(new SkinMap("[Shirt] Podcast","TShirt_DayZPodcast",TShirt_ColorBase));
        m_Skins.Insert(new SkinMap("[Shirt] V++","TShirt_VPP",TShirt_ColorBase));

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
        //--- AK101
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
        //--- AKM - note that because dayz devs are peabrains, AKM derrived guns cannot have these same skins applied
        m_Skins.Insert(new SkinMap("[Gun] Mango", "DZBR_AKM_Mango", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] Apple", "DZBR_AKM_Apple", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] Zerba", "DZBR_AKM_Zerba", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] Exhibit", "DZBR_AKM_Exhibit", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] Unnamed", "DZBR_AKM_Unnamed", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_AKM_FB_Bug", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_AKM_FB_Reward_White_Black", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_AKM_FB_Reward_White_Orange", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_AKM_FB_Reward_White_Blue", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_AKM_FB_Reward_White_Red", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_AKM_FB_Reward_White_Grey", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_AKM_FB_Reward_Black_Orange", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_AKM_FB_Reward_Black_Blue", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_AKM_FB_Reward_Black_Red", AKM_Base, "todo_skins", false));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_AKM_FB_Reward_Black_Grey", AKM_Base, "todo_skins", false));
        //--- FAL
        m_Skins.Insert(new SkinMap("[Gun] Army", "DZBR_FAL_Army", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Snow", "DZBR_FAL_Snow", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Blue Web", "DZBR_FAL_BlueWeb", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Vintage", "DZBR_FAL_Vintage", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_FAL_FB_Bug", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_FAL_FB_Reward_White_Black", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_FAL_FB_Reward_White_Orange", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_FAL_FB_Reward_White_Blue", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_FAL_FB_Reward_White_Red", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_FAL_FB_Reward_White_Grey", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_FAL_FB_Reward_Black_Orange", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_FAL_FB_Reward_Black_Blue", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_FAL_FB_Reward_Black_Red", FAL_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_FAL_FB_Reward_Black_Grey", FAL_Base, "todo_skins"));
        //--- M4A1
        m_Skins.Insert(new SkinMap("[Gun] Mango", "DZBR_M4A1_Mango", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Apple", "DZBR_M4A1_Apple", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Blue Web", "DZBR_M4A1_BlueWeb", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Coral", "DZBR_M4A1_Coral", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Splash", "DZBR_M4A1_Splash", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_M4A1_FB_Bug", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_M4A1_FB_Reward_White_Black", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_M4A1_FB_Reward_White_Orange", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_M4A1_FB_Reward_White_Blue", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_M4A1_FB_Reward_White_Red", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_M4A1_FB_Reward_White_Grey", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_M4A1_FB_Reward_Black_Orange", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_M4A1_FB_Reward_Black_Blue", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_M4A1_FB_Reward_Black_Red", M4A1_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_M4A1_FB_Reward_Black_Grey", M4A1_Base, "todo_skins"));
        //--- MOSIN9130
        m_Skins.Insert(new SkinMap("[Gun] Mango", "DZBR_Mosin9130_Mango", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Zerba", "DZBR_Mosin9130_Zerba", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Old Times", "DZBR_Mosin9130_OldTimes", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Red Wood", "DZBR_Mosin9130_RedWood", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Pink Kush", "DZBR_Mosin9130_PinkKush", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Mosin9130_FB_Bug", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Mosin9130_FB_Reward_White_Black", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Mosin9130_FB_Reward_White_Orange", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Mosin9130_FB_Reward_White_Blue", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Mosin9130_FB_Reward_White_Red", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Mosin9130_FB_Reward_White_Grey", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Mosin9130_FB_Reward_Black_Orange", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Mosin9130_FB_Reward_Black_Blue", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Mosin9130_FB_Reward_Black_Red", Mosin9130, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Mosin9130_FB_Reward_Black_Grey", Mosin9130, "todo_skins"));
        //--- MOSIN9130_SAWEDOFF
        m_Skins.Insert(new SkinMap("[Gun] Zerba", "DZBR_SawedoffMosin9130_Zerba", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Old Times", "DZBR_SawedoffMosin9130_OldTimes", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Red Wood", "DZBR_SawedoffMosin9130_RedWood", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Pink Kush", "DZBR_SawedoffMosin9130_PinkKush", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_SawedoffMosin9130_FB_Bug", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_SawedoffMosin9130_FB_Reward_White_Black", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_SawedoffMosin9130_FB_Reward_White_Orange", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_SawedoffMosin9130_FB_Reward_White_Blue", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_SawedoffMosin9130_FB_Reward_White_Red", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_SawedoffMosin9130_FB_Reward_White_Grey", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_SawedoffMosin9130_FB_Reward_Black_Orange", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_SawedoffMosin9130_FB_Reward_Black_Blue", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_SawedoffMosin9130_FB_Reward_Black_Red", SawedoffMosin9130_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_SawedoffMosin9130_FB_Reward_Black_Grey", SawedoffMosin9130_Base, "todo_skins"));
        //--- MP5
        m_Skins.Insert(new SkinMap("[Gun] Bee", "DZBR_MP5K_Bee", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Blue", "DZBR_MP5K_Blue", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Killer", "DZBR_MP5K_Killer", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Orange", "DZBR_MP5K_Orange", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Zebra", "DZBR_MP5K_Zebra", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_MP5K_FB_Bug", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_MP5K_FB_Reward_White_Black", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_MP5K_FB_Reward_White_Orange", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_MP5K_FB_Reward_White_Blue", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_MP5K_FB_Reward_White_Red", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_MP5K_FB_Reward_White_Grey", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_MP5K_FB_Reward_Black_Orange", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_MP5K_FB_Reward_Black_Blue", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_MP5K_FB_Reward_Black_Red", MP5K_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_MP5K_FB_Reward_Black_Grey", MP5K_Base, "todo_skins"));
        //--- SKS
        m_Skins.Insert(new SkinMap("[Gun] Competition", "DZBR_SKS_Competition", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Love", "DZBR_SKS_Love", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Modern", "DZBR_SKS_Modern", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Redline", "DZBR_SKS_RedLine", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_SKS_FB_Bug", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_SKS_FB_Reward_White_Black", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_SKS_FB_Reward_White_Orange", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_SKS_FB_Reward_White_Blue", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_SKS_FB_Reward_White_Red", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_SKS_FB_Reward_White_Grey", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_SKS_FB_Reward_Black_Orange", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_SKS_FB_Reward_Black_Blue", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_SKS_FB_Reward_Black_Red", SKS_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_SKS_FB_Reward_Black_Grey", SKS_Base, "todo_skins"));
        //--- UMP45
        m_Skins.Insert(new SkinMap("[Gun] Abyss", "DZBR_UMP45_Abyss", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Exhibit", "DZBR_UMP45_Exhibit", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Killer", "DZBR_UMP45_Killer", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rust", "DZBR_UMP45_Rust", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_UMP45_FB_Bug", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_UMP45_FB_Reward_White_Black", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_UMP45_FB_Reward_White_Orange", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_UMP45_FB_Reward_White_Blue", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_UMP45_FB_Reward_White_Red", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_UMP45_FB_Reward_White_Grey", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_UMP45_FB_Reward_Black_Orange", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_UMP45_FB_Reward_Black_Blue", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_UMP45_FB_Reward_Black_Red", UMP45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_UMP45_FB_Reward_Black_Grey", UMP45_Base, "todo_skins"));
        //--- VSS
        m_Skins.Insert(new SkinMap("[Gun] Blue Gem", "DZBR_VSS_BlueGem", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rust", "DZBR_VSS_Rust", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Stellar", "DZBR_VSS_Stellar", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Meptb", "DZBR_VSS_Meptb", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_VSS_FB_Bug", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_VSS_FB_Reward_White_Black", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_VSS_FB_Reward_White_Orange", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_VSS_FB_Reward_White_Blue", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_VSS_FB_Reward_White_Red", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_VSS_FB_Reward_White_Grey", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_VSS_FB_Reward_Black_Orange", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_VSS_FB_Reward_Black_Blue", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_VSS_FB_Reward_Black_Red", VSS, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_VSS_FB_Reward_Black_Grey", VSS, "todo_skins"));
        //--- WINCHESTER70
        m_Skins.Insert(new SkinMap("[Gun] Depth", "DZBR_Winchester70_Depth", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Digital", "DZBR_Winchester70_Digital", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Love", "DZBR_Winchester70_Love", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Old Times", "DZBR_Winchester70_OldTimes", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Winchester70_FB_Bug", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Winchester70_FB_Reward_White_Black", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Winchester70_FB_Reward_White_Orange", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Winchester70_FB_Reward_White_Blue", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Winchester70_FB_Reward_White_Red", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Winchester70_FB_Reward_White_Grey", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Winchester70_FB_Reward_Black_Orange", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Winchester70_FB_Reward_Black_Blue", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Winchester70_FB_Reward_Black_Red", Winchester70_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Winchester70_FB_Reward_Black_Grey", Winchester70_Base, "todo_skins"));
        //--- 1911
        m_Skins.Insert(new SkinMap("[Gun] Tactical", "DZBR_Colt1911_Tactical", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Shadows", "DZBR_Colt1911_Shadows", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Blue", "DZBR_Colt1911_Blue", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Orange", "DZBR_Colt1911_Orange", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Ukraine", "DZBR_Colt1911_Ukraine", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Colt1911_FB_Bug", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Colt1911_FB_Reward_White_Black", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Colt1911_FB_Reward_White_Orange", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Colt1911_FB_Reward_White_Blue", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Colt1911_FB_Reward_White_Red", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Colt1911_FB_Reward_White_Grey", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Colt1911_FB_Reward_Black_Orange", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Colt1911_FB_Reward_Black_Blue", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Colt1911_FB_Reward_Black_Red", Colt1911_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Colt1911_FB_Reward_Black_Grey", Colt1911_Base, "todo_skins"));
        //--- DEAGLE
        m_Skins.Insert(new SkinMap("[Gun] Blue Toy", "DZBR_Deagle_BlueToy", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Exhibit", "DZBR_Deagle_Exhibit", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Pink Toy", "DZBR_Deagle_PinkToy", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Redline", "DZBR_Deagle_RedLine", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Ice", "DZBR_Deagle_Ice", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Deagle_FB_Bug", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Deagle_FB_Reward_White_Black", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Deagle_FB_Reward_White_Orange", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Deagle_FB_Reward_White_Blue", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Deagle_FB_Reward_White_Red", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Deagle_FB_Reward_White_Grey", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Deagle_FB_Reward_Black_Orange", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Deagle_FB_Reward_Black_Blue", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Deagle_FB_Reward_Black_Red", Deagle_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Deagle_FB_Reward_Black_Grey", Deagle_Base, "todo_skins"));
        //--- FNX45
        m_Skins.Insert(new SkinMap("[Gun] Tactical", "DZBR_FNX45_Tactical", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Exhibit", "DZBR_FNX45_Exhibit", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Desert", "DZBR_FNX45_Desert", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Ukraine", "DZBR_FNX45_Ukraine", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_FNX45_FB_Bug", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_FNX45_FB_Reward_White_Black", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_FNX45_FB_Reward_White_Orange", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_FNX45_FB_Reward_White_Blue", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_FNX45_FB_Reward_White_Red", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_FNX45_FB_Reward_White_Grey", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_FNX45_FB_Reward_Black_Orange", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_FNX45_FB_Reward_Black_Blue", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_FNX45_FB_Reward_Black_Red", FNX45_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_FNX45_FB_Reward_Black_Grey", FNX45_Base, "todo_skins"));
        //--- GLOCK
        m_Skins.Insert(new SkinMap("[Gun] Tactical", "DZBR_Glock19_Tactical", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Pink Camo", "DZBR_Glock19_PinkCamo", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Sandstorm", "DZBR_Glock19_Sandstorm", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] White", "DZBR_Glock19_White", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Glock19_FB_Bug", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Glock19_FB_Reward_White_Black", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Glock19_FB_Reward_White_Orange", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Glock19_FB_Reward_White_Blue", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Glock19_FB_Reward_White_Red", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Glock19_FB_Reward_White_Grey", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Glock19_FB_Reward_Black_Orange", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Glock19_FB_Reward_Black_Blue", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Glock19_FB_Reward_Black_Red", Glock19_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Glock19_FB_Reward_Black_Grey", Glock19_Base, "todo_skins"));
        //--- IZH43
        m_Skins.Insert(new SkinMap("[Gun] Black", "DZBR_Izh43Shotgun_Black", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Golden Prestige", "DZBR_Izh43Shotgun_GoldenPrestige", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Silver Prestige", "DZBR_Izh43Shotgun_SilverPrestige", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Zebra", "DZBR_Izh43Shotgun_Zebra", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Izh43Shotgun_FB_Bug", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Izh43Shotgun_FB_Reward_White_Black", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Izh43Shotgun_FB_Reward_White_Orange", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Izh43Shotgun_FB_Reward_White_Blue", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Izh43Shotgun_FB_Reward_White_Red", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Izh43Shotgun_FB_Reward_White_Grey", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Izh43Shotgun_FB_Reward_Black_Orange", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Izh43Shotgun_FB_Reward_Black_Blue", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Izh43Shotgun_FB_Reward_Black_Red", Izh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Izh43Shotgun_FB_Reward_Black_Grey", Izh43Shotgun, "todo_skins"));
        //---IzH43_SAWEDOFF
        m_Skins.Insert(new SkinMap("[Gun] Black", "DZBR_SawedoffIzh43Shotgun_Black", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Golden Prestige", "DZBR_SawedoffIzh43Shotgun_GoldenPrestige", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Silver Prestige", "DZBR_SawedoffIzh43Shotgun_SilverPrestige", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Zebra", "DZBR_SawedoffIzh43Shotgun_Zebra", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_SawedoffIzh43Shotgun_FB_Bug", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Black", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Orange", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Blue", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Red", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Grey", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Orange", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Blue", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Red", SawedoffIzh43Shotgun, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Grey", SawedoffIzh43Shotgun, "todo_skins"));
        //--- MP133
        m_Skins.Insert(new SkinMap("[Gun] Battle", "DZBR_MP133_Battle", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Hotline", "DZBR_MP133_Hotline", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Taser", "DZBR_MP133_Taser", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Rainbow", "DZBR_MP133_Rainbow", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_MP133_FB_Bug", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_MP133_FB_Reward_White_Black", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_MP133_FB_Reward_White_Orange", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_MP133_FB_Reward_White_Blue", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_MP133_FB_Reward_White_Red", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_MP133_FB_Reward_White_Grey", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_MP133_FB_Reward_Black_Orange", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_MP133_FB_Reward_Black_Blue", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_MP133_FB_Reward_Black_Red", Mp133Shotgun_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_MP133_FB_Reward_Black_Grey", Mp133Shotgun_Base, "todo_skins"));
        //--- SAIGA
        m_Skins.Insert(new SkinMap("[Gun] Blue Web", "DZBR_Saiga_BlueWeb", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Neon", "DZBR_Saiga_Neon", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Prestige", "DZBR_Saiga_Prestige", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Silver", "DZBR_Saiga_Silver", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Saiga_FB_Bug", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Saiga_FB_Reward_White_Black", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Saiga_FB_Reward_White_Orange", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Saiga_FB_Reward_White_Blue", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Saiga_FB_Reward_White_Red", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Saiga_FB_Reward_White_Grey", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Saiga_FB_Reward_Black_Orange", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Saiga_FB_Reward_Black_Blue", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Saiga_FB_Reward_Black_Red", Saiga_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Saiga_FB_Reward_Black_Grey", Saiga_Base, "todo_skins"));
        //--- AWP
        m_Skins.Insert(new SkinMap("[Gun] Digital", "DZBR_Expansion_AWM_Digital", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Love", "DZBR_Expansion_AWM_Love", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Marine", "DZBR_Expansion_AWM_Marine", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] White Net", "DZBR_Expansion_AWM_WhiteNet", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Expansion_AWM_FB_Bug", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Expansion_AWM_FB_Reward_White_Black", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Expansion_AWM_FB_Reward_White_Orange", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Expansion_AWM_FB_Reward_White_Blue", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Expansion_AWM_FB_Reward_White_Red", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Expansion_AWM_FB_Reward_White_Grey", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Expansion_AWM_FB_Reward_Black_Orange", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Expansion_AWM_FB_Reward_Black_Blue", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Expansion_AWM_FB_Reward_Black_Red", Expansion_AWM_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Expansion_AWM_FB_Reward_Black_Grey", Expansion_AWM_Base, "todo_skins"));
        //--- MP5A5
        m_Skins.Insert(new SkinMap("[Gun] Abyss", "DZBR_Expansion_MP5_Abyss", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Blue", "DZBR_Expansion_MP5_Blue", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Exhibit", "DZBR_Expansion_MP5_Exhibit", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Orange", "DZBR_Expansion_MP5_Orange", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Killer", "DZBR_Expansion_MP5_Killer", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Expansion_MP5_FB_Bug", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Expansion_MP5_FB_Reward_White_Black", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Expansion_MP5_FB_Reward_White_Orange", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Expansion_MP5_FB_Reward_White_Blue", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Expansion_MP5_FB_Reward_White_Red", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Expansion_MP5_FB_Reward_White_Grey", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Expansion_MP5_FB_Reward_Black_Orange", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Expansion_MP5_FB_Reward_Black_Blue", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Expansion_MP5_FB_Reward_Black_Red", Expansion_MP5_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Expansion_MP5_FB_Reward_Black_Grey", Expansion_MP5_Base, "todo_skins"));
        //--- BENELIM4
        m_Skins.Insert(new SkinMap("[Gun] Killer", "DZBR_Expansion_BenelliM4_Killer", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Neon", "DZBR_Expansion_BenelliM4_Neon", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Army", "DZBR_Expansion_BenelliM4_Army", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Blue Marine", "DZBR_Expansion_BenelliM4_BlueMarine", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] Debug", "DZBR_Expansion_BenelliM4_FB_Bug", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBK", "DZBR_Expansion_BenelliM4_FB_Reward_White_Black", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WO", "DZBR_Expansion_BenelliM4_FB_Reward_White_Orange", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WBL", "DZBR_Expansion_BenelliM4_FB_Reward_White_Blue", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WR", "DZBR_Expansion_BenelliM4_FB_Reward_White_Red", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_WG", "DZBR_Expansion_BenelliM4_FB_Reward_White_Grey", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BO", "DZBR_Expansion_BenelliM4_FB_Reward_Black_Orange", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BB", "DZBR_Expansion_BenelliM4_FB_Reward_Black_Blue", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BR", "DZBR_Expansion_BenelliM4_FB_Reward_Black_Red", Expansion_BenelliM4_Base, "todo_skins"));
        m_Skins.Insert(new SkinMap("[Gun] FB_BG", "DZBR_Expansion_BenelliM4_FB_Reward_Black_Grey", Expansion_BenelliM4_Base, "todo_skins"));

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
        BattleRoyaleClient br_c = BattleRoyaleClient.Cast( GetBR() ); 

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
            
            if(skin.GetFlag() == "" || br_c.HasPurchase(skin.GetFlag()))
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
        BattleRoyaleClient br_c = BattleRoyaleClient.Cast( GetBR() ); 
        string skin_flag = skin_map.GetFlag();
        if(skin_flag != "" && !br_c.HasPurchase(skin_flag))
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