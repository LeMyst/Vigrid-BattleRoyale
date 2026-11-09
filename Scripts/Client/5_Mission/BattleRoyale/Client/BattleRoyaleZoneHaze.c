#ifndef SERVER
//--- Birth rate as a fraction of vanilla's edge gas: thin enough to see through.
static const float BR_ZONE_HAZE_DENSITY = 0.5;

/**
 *  A thin gas haze along the current zone edge: vanilla's contaminated-area edge gas, client-side
 *  particles only. Thinned so nobody can hide in it; set just outside the edge so the safe side stays clear.
 */
class BattleRoyaleZoneHaze: BattleRoyaleRingArc
{
    static const float EMITTER_SPACING = 20.0;  // vanilla EffectArea m_OuterSpacing; the emitter box is 20 m
    static const float OUTSET = 10.0;           // half the box, so the gas starts at the edge

    protected float m_Density;
    protected ref map<int, ParticleSource> m_Emitters = new map<int, ParticleSource>();

    void BattleRoyaleZoneHaze(float density)
    {
        m_Spacing = EMITTER_SPACING;
        m_SpawnBudget = 10;
        m_Tag = "[ZoneHaze]";
        m_Density = density;
    }

    void ~BattleRoyaleZoneHaze()
    {
        Clear();
    }

    //! Birth rate as a fraction of the vanilla effect's, applied to the emitters already playing.
    void SetDensity(float density)
    {
        if ( density == m_Density )
            return;

        m_Density = density;
        foreach ( int idx, ParticleSource p : m_Emitters )
        {
            if ( p )
                ApplyDensity( p );
        }
    }

    protected void ApplyDensity(ParticleSource p)
    {
        p.ScaleParticleParamFromOriginal( EmitorParam.BIRTH_RATE, m_Density );
        p.ScaleParticleParamFromOriginal( EmitorParam.BIRTH_RATE_RND, m_Density );
    }

    override protected bool HasAt(int idx)
    {
        return m_Emitters.Contains( idx );
    }

    override protected void CollectLive(array<int> indices)
    {
        foreach ( int idx, ParticleSource p : m_Emitters )
            indices.Insert( idx );
    }

    //! Stop() fades out, then the pool takes it back; the handle must not be used after that.
    override protected void DropAt(int idx)
    {
        ParticleSource old = m_Emitters.Get( idx );
        if ( old )
            old.StopParticle();
        m_Emitters.Remove( idx );
    }

    override protected void DropAll()
    {
        foreach ( int idx, ParticleSource p : m_Emitters )
        {
            if ( p )
                p.StopParticle();
        }
        m_Emitters.Clear();
    }

    //! NULL when the shared particle pool is exhausted (or not allocated yet).
    override protected bool SpawnAt(int idx)
    {
        float angle = AngleAt( idx );
        float cx = m_Center[0];
        float cz = m_Center[2];
        float r = m_Radius + OUTSET;
        float x = cx + Math.Cos( angle ) * r;
        float z = cz + Math.Sin( angle ) * r;
        vector pos = Vector( x, GetGame().SurfaceY( x, z ), z );
        vector ori = GetGame().GetSurfaceOrientation( x, z );

        ParticleSource p = ParticleManager.GetInstance().CreateParticle( ParticleList.CONTAMINATED_AREA_GAS_AROUND, pos, true, NULL, ori );
        if ( !p )
            return false;

        ApplyDensity( p );
        m_Emitters.Set( idx, p );
        return true;
    }
}
#endif
