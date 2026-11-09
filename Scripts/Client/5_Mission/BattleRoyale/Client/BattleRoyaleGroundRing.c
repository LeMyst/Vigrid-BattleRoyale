#ifndef SERVER
//--- Colours as on the map (VIGRID_MAP_COLOR_CURRENT_ZONE / _NEXT_ZONE), at the opacity picked in game:
//--- white reads far stronger than blue on terrain, hence half the alpha.
static const string BR_GROUND_RING_TEXTURE_CURRENT = "#(argb,8,8,3)color(0.05,0.25,1,0.5,ca)";
static const string BR_GROUND_RING_TEXTURE_NEXT = "#(argb,8,8,3)color(1,1,1,0.25,ca)";

/**
 *  One zone circle drawn on the terrain as a band of flat local quads (BR_GroundSegment).
 *
 *  Local objects, nothing on the wire. Engine decals cannot do this from script (no terrain
 *  entity reachable; measured 2026-11-09), hence the quads. The near-camera arc is BattleRoyaleRingArc.
 */
class BattleRoyaleGroundRing: BattleRoyaleRingArc
{
    static const float SEGMENT_LENGTH = 2.0;    // ground_segment.p3d is 2 m along +Z
    static const float SURFACE_OFFSET = 0.05;   // 2 cm z-fought, 5 cm did not

    protected string m_Texture;
    protected float m_SegmentScale;

    protected ref map<int, Object> m_Segments = new map<int, Object>();

    void BattleRoyaleGroundRing(string texture)
    {
        m_Spacing = SEGMENT_LENGTH;
        m_SpawnBudget = 150;
        m_Tag = "[GroundRing]";
        m_Texture = texture;
    }

    void ~BattleRoyaleGroundRing()
    {
        Clear();
    }

    //! Recolour the segments already on the ground.
    void SetTexture(string texture)
    {
        if ( texture == m_Texture )
            return;

        m_Texture = texture;
        foreach ( int idx, Object seg : m_Segments )
        {
            EntityAI entity = EntityAI.Cast( seg );
            if ( entity )
                entity.SetObjectTexture( 0, m_Texture );
        }
    }

    int GetSegmentCount()
    {
        return m_Segments.Count();
    }

    override protected void OnCircleChanged()
    {
        //--- Chord length over model length, so neighbours abut: an overlap would show as a darker seam.
        float chord = 2.0 * m_Radius * Math.Sin( m_Step * 0.5 );
        m_SegmentScale = chord / SEGMENT_LENGTH;
    }

    override protected bool HasAt(int idx)
    {
        return m_Segments.Contains( idx );
    }

    override protected void CollectLive(array<int> indices)
    {
        foreach ( int idx, Object seg : m_Segments )
            indices.Insert( idx );
    }

    override protected void DropAt(int idx)
    {
        Object old = m_Segments.Get( idx );
        if ( old )
            GetGame().ObjectDelete( old );
        m_Segments.Remove( idx );
    }

    override protected void DropAll()
    {
        foreach ( int idx, Object seg : m_Segments )
        {
            if ( seg )
                GetGame().ObjectDelete( seg );
        }
        m_Segments.Clear();
    }

    //! A failed create is stored as NULL and not retried, as before the arc was shared.
    override protected bool SpawnAt(int idx)
    {
        m_Segments.Set( idx, SpawnSegment( idx ) );
        return true;
    }

    protected Object SpawnSegment(int idx)
    {
        float angle = AngleAt( idx );
        float cx = m_Center[0];
        float cz = m_Center[2];
        float x = cx + Math.Cos( angle ) * m_Radius;
        float z = cz + Math.Sin( angle ) * m_Radius;
        float y = GetGame().SurfaceY( x, z );
        vector p = Vector( x, y, z );
        vector normal = GetGame().SurfaceGetNormal( x, z );

        //--- Tangent to the ring, flattened onto the terrain plane so the quad lies on the slope.
        vector tangent = Vector( -Math.Sin( angle ), 0, Math.Cos( angle ) );
        float along_up = vector.Dot( tangent, normal );
        vector dir = tangent - ( normal * along_up );
        dir.Normalize();

        Object seg = GetGame().CreateObjectEx( "BR_GroundSegment", p, ECE_LOCAL );
        if ( !seg )
            return NULL;

        vector mat[4];
        Math3D.DirectionAndUpMatrix( dir, normal, mat );
        mat[3] = p + ( normal * SURFACE_OFFSET );
        seg.SetTransform( mat );
        seg.SetScale( m_SegmentScale );

        EntityAI entity = EntityAI.Cast( seg );
        if ( entity )
            entity.SetObjectTexture( 0, m_Texture );

        return seg;
    }
}
#endif
