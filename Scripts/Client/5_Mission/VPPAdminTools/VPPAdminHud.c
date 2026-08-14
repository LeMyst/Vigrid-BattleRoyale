#ifdef VPPADMINTOOLS
#ifndef SERVER
modded class VPPAdminHud
{
    override void DefineButtons()
    {
        super.DefineButtons();
        
        //--- Resolved here rather than passed as "#KEY": these go into VPPAdminTools' own widgets
        //--- and it is not ours to assume it routes them through a SetText that would resolve one.
        string label   = Widget.TranslateString("#STR_BR_VPP_BUTTON");
        string tooltip = Widget.TranslateString("#STR_BR_VPP_TOOLTIP");

        InsertButton("MenuBattleRoyaleManager" , label, "set:dayz_gui_vpp image:vpp_icon_obj_editor", tooltip);
    }
};
#endif
#endif