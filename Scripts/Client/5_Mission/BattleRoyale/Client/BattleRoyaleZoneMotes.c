#ifndef SERVER
//--- Birth rate as a fraction of zone_edge_motes.ptc's own; tune the look in the .ptc, this is a live knob.
static const float BR_ZONE_MOTES_DENSITY = 1.0;

/**
 *  Blue motes rising slowly off the current zone edge: Data/particles/zone_edge_motes.ptc,
 *  client-side particles only. Each emitter is a 10 m strip turned along the circle.
 */
class BattleRoyaleZoneMotes: BattleRoyaleRingArc
{
    static const float EMITTER_SPACING = 10.0;  // the .ptc's box is 10 m long, so strips abut

    protected float m_Density;
    protected ref map<int, ParticleSource> m_Emitters = new map<int, ParticleSource>();

    void BattleRoyaleZoneMotes(float density)
    {
        m_Spacing = EMITTER_SPACING;
        m_SpawnBudget = 10;
        m_Tag = "[ZoneMotes]";
        m_Density = density;
    }

    void ~BattleRoyaleZoneMotes()
    {
        Clear();
    }

    //! Applied to the emitters already playing too.
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

    //! StopParticle() fades out, then the pool takes it back; the handle must not be used after that.
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

    //! false when the shared particle pool is exhausted (or not allocated yet).
    override protected bool SpawnAt(int idx)
    {
        float angle = AngleAt( idx );
        float cx = m_Center[0];
        float cz = m_Center[2];
        float x = cx + Math.Cos( angle ) * m_Radius;
        float z = cz + Math.Sin( angle ) * m_Radius;
        vector pos = Vector( x, GetGame().SurfaceY( x, z ), z );

        //--- The box's long side is its local Z: face it along the tangent.
        vector tangent = Vector( -Math.Sin( angle ), 0, Math.Cos( angle ) );
        vector ori = tangent.VectorToAngles();

        ParticleSource p = ParticleManager.GetInstance().CreateParticle( ParticleList.BR_ZONE_EDGE_MOTES, pos, true, NULL, ori );
        if ( !p )
            return false;

        ApplyDensity( p );
        m_Emitters.Set( idx, p );
        return true;
    }
}
#endif
