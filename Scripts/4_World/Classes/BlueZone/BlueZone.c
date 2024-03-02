#ifdef BLUE_ZONE
class BlueZone : BREffectArea
{
    override void SetupZoneData(  EffectAreaParams params )
    {
        super.SetupZoneData( params );
    }

    override void EEInit()
    {
        BattleRoyaleUtils.Trace("BlueZone EEInit()");

        // We make sure we have the particle array
        if ( !m_ToxicClouds )
            m_ToxicClouds = new array<Particle>;

        m_Name = "BlueZone";
        m_Radius = 10;
        m_PositiveHeight = 25;
        m_NegativeHeight = 10;
        m_InnerRings = 1;
        m_InnerSpacing = 20;
        m_OuterRingToggle = false;
        m_OuterRingOffset = 0;
        m_OuterSpacing = 0;
        m_VerticalLayers = 0;
        m_VerticalOffset = 10; // Do nothing if m_VerticalLayers = 0
        //m_Type = eZoneType.DYNAMIC;
        m_TriggerType = "";
        //m_ParticleID = ParticleList.CONTAMINATED_UNDERGROUND_BIGONE;
        //m_AroundParticleID = ParticleList.CONTAMINATED_UNDERGROUND_AROUND;
        //m_TinyParticleID = ParticleList.CONTAMINATED_UNDERGROUND_TINY;
        //m_PPERequesterType = "PPERequester_NamContamination";
        //m_PPERequesterIdx = PPERequesterBank.GetRequester( m_PPERequesterType.ToType() ).GetRequesterIDX();

        SetSynchDirty();

        InitZone();

        super.EEInit();
    }

    override void InitZoneServer()
    {
        BattleRoyaleUtils.Trace("BlueZone InitZoneServer()");
        super.InitZoneServer();

        if ( m_TriggerType != "" )
            CreateTrigger( m_Position, m_Radius );
    }

    override void InitZoneClient()
    {
        BattleRoyaleUtils.Trace("BlueZone InitZoneClient()");
        super.InitZoneClient();

        PlaceParticles( GetWorldPosition(), m_Radius, m_InnerRings, m_InnerSpacing, m_OuterRingToggle, m_OuterSpacing, m_OuterRingOffset, m_ParticleID );
    }

    override void EEDelete( EntityAI parent )
    {
        BattleRoyaleUtils.Trace("BlueZone EEDelete()");
        super.EEDelete( parent );
    }
}
#endif
