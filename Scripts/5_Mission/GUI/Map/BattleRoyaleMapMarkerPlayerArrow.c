class BattleRoyaleMapMarkerPlayerArrow : ExpansionMapMarker
{
    protected PlayerBase m_Entity;
    protected vector v_Position;
    protected vector v_Direction;

    void SetPlayer(PlayerBase entity)
    {
        m_Entity = entity;
    }

    void SetEntityPosition(vector position)
    {
        v_Position = position;
    }

    void SetEntityDirection(vector direction)
    {
        v_Direction = direction;
    }

    override void Update( float pDt )
    {
        if(m_Entity)
        {
            v_Position = m_Entity.GetPosition();
            v_Direction = m_Entity.GetDirection();
            SetName( m_Entity.GetIdentityName() );
        }

        SetPosition( v_Position );
        //GetIconWidget().LoadImageFile( 0, EXPANSION_NOTIFICATION_ICON_POSITION );
        //GetIconWidget().LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE );
        GetIconWidget().LoadImageFile(0, "DayZExpansion\\Core\\GUI\\icons\\misc\\T_Fox_256x256.edds");
        GetDragWidget().SetRotation( 0, 0, GetMapDirection(), true );
    }

    private int GetMapDirection()
    {
        return Math.Round( Math.NormalizeAngle( v_Direction.VectorToAngles()[0] ) );
    }

    void ShowRoot(bool show)
    {
        GetLayoutRoot().Show(show);
    }
}
