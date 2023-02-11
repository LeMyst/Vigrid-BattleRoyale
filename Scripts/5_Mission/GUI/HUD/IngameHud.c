modded class IngameHud
{
    protected bool b_BrVisibilityOverride;
    void IngameHud()
    {
        b_BrVisibilityOverride = false;
    }

    void BR_HIDE()
    {
        Show( false );
        b_BrVisibilityOverride = true;
    }

    override void Show( bool show )
	{
        if(b_BrVisibilityOverride)
            return;

        super.Show( show );
    }

}
