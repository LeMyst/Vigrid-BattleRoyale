#ifdef VPPADMINTOOLS
modded class VPPAdminHud
{
    override void DefineButtons()
    {
        Print("MenuBattleRoyaleManager DefineButtons");
        super.DefineButtons();
        InsertButton("MenuBattleRoyaleManager" , "BattleRoyale", "set:dayz_gui_vpp image:vpp_icon_obj_editor", "Manage the Battle Royale");
    }
};

class MenuBattleRoyaleManager: AdminHudSubMenu
{
    private ButtonWidget nextStateButton;
    private ButtonWidget startSpectateButton;

    void MenuBattleRoyaleManager()
    {
        Print("MenuBattleRoyaleManager MenuBattleRoyaleManager");
        //GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerStats", this, SingleplayerExecutionType.Client);
    }

    override void OnCreate(Widget RootW)
    {
        Print("MenuBattleRoyaleManager OnCreate");
        super.OnCreate(RootW);

        M_SUB_WIDGET  = CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/BattleRoyaleManagerUI/MenuBattleRoyaleManager.layout");
        M_SUB_WIDGET.SetHandler(this);
        m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
        m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );

        nextStateButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnNextState"));
        startSpectateButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnStartSpectate"));
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!IsSubMenuVisible() && M_SUB_WIDGET == null)
            return;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        Print("MenuBattleRoyaleManager OnClick");
        super.OnClick(w, x, y, button);
        switch(w)
        {
            case nextStateButton:
                NextState();
            break;

            case startSpectateButton:
                StartSpectate();
            break;
        }
        return false;
    }

    override void HideSubMenu()
    {
        Print("MenuBattleRoyaleManager HideSubMenu");
        super.HideSubMenu();
    }

    override void ShowSubMenu()
    {
        Print("MenuBattleRoyaleManager ShowSubMenu");
        super.ShowSubMenu();
    }

    private void NextState()
    {
        Print("MenuBattleRoyaleManager NextState");
        if ( IsMissionClient() )
        {
            GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "NextState", NULL, true);
        }
    }

    private void StartSpectate()
    {
        Print("MenuBattleRoyaleManager StartSpectate");
        if ( IsMissionClient() )
        {
            PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
            GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "StartSpectate", NULL, true, NULL, player);
        }
    }
};
#endif
