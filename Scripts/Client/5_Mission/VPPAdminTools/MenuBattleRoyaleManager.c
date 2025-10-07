#ifdef VPPADMINTOOLS
#ifndef SERVER
class MenuBattleRoyaleManager: AdminHudSubMenu
{
    private ButtonWidget nextStateButton;

    void MenuBattleRoyaleManager()
    {
        //GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerStats", this, SingleplayerExecutionType.Client);
    }

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);

        M_SUB_WIDGET  = CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/BattleRoyaleManagerUI/MenuBattleRoyaleManager.layout");
        M_SUB_WIDGET.SetHandler(this);
        m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
        m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );

        nextStateButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnNextState"));
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!IsSubMenuVisible() && M_SUB_WIDGET == null)
            return;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        super.OnClick(w, x, y, button);

        switch(w)
        {
            case nextStateButton:
                NextState();
            break;
        }

        return false;
    }

    private void NextState()
    {
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "NextState", NULL, true);
    }
};
#endif
#endif
