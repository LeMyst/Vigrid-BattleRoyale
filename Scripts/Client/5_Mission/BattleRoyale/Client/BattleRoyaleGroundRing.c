#ifndef SERVER
//--- Colours as on the map (VIGRID_MAP_COLOR_CURRENT_ZONE / _NEXT_ZONE), at the opacity picked in game:
//--- white reads far stronger than blue on terrain, hence half the alpha.
static const string BR_GROUND_RING_TEXTURE_CURRENT = "#(argb,8,8,3)color(0.05,0.25,1,0.5,ca)";
static const string BR_GROUND_RING_TEXTURE_NEXT = "#(argb,8,8,3)color(1,1,1,0.25,ca)";

/**
 *  One zone circle drawn on the terrain as a band of flat local quads (BR_GroundSegment).
 *
 *  Client-side only: local objects, nothing on the wire - the client already holds the circles.
 *  Only the arc within DRAW_RADIUS of the anchor (the camera) exists; it is re-cut when the
 *  anchor moves RECUT_DISTANCE or the circle changes. Engine decals cannot do this from script
 *  (no terrain entity reachable; measured 2026-11-09), hence the quads.
 */
class BattleRoyaleGroundRing
{
    static const float SEGMENT_LENGTH = 2.0;    // ground_segment.p3d is 2 m along +Z
    static const float SURFACE_OFFSET = 0.05;   // 2 cm z-fought, 5 cm did not
    static const float DRAW_RADIUS = 250.0;
    static const float RECUT_DISTANCE = 10.0;
    static const int SPAWN_BUDGET = 150;        // per Update, so a lock does not hitch one frame

    protected string m_Texture;
    protected vector m_Center;
    protected float m_Radius;
    protected bool m_HasCircle;

    protected int m_Count;
    protected float m_Step;
    protected float m_SegmentScale;

    //--- Wanted index window [m_First, m_Last], unwrapped (may be negative or >= m_Count).
    protected int m_First;
    protected int m_Last;
    protected bool m_Pending;

    protected vector m_LastAnchor;
    protected bool m_HasAnchor;

    protected ref map<int, Object> m_Segments = new map<int, Object>();

    void BattleRoyaleGroundRing(string texture)
    {
        m_Texture = texture;
    }

    void ~BattleRoyaleGroundRing()
    {
        Clear();
    }

    //! Set the circle to draw. Cheap when nothing changed, so it can be called every frame.
    void SetCircle(vector center, float radius)
    {
        if ( m_HasCircle && m_Center == center && m_Radius == radius )
            return;

        DeleteAll();
        m_Center = center;
        m_Radius = radius;
        m_HasCircle = true;

        m_Count = Math.Ceil( Math.PI2 * radius / SEGMENT_LENGTH );
        if ( m_Count < 8 )
            m_Count = 8;
        m_Step = Math.PI2 / m_Count;

        //--- Chord length over model length, so neighbours abut: an overlap would show as a darker seam.
        float chord = 2.0 * radius * Math.Sin( m_Step * 0.5 );
        m_SegmentScale = chord / SEGMENT_LENGTH;

        m_HasAnchor = false;

        string circle_text = center.ToString() + " r=" + radius;
        BattleRoyaleUtils.Debug( "[GroundRing] circle " + circle_text + " segments=" + m_Count );
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

    //! No circle any more (cleared by the server, or the match reset).
    void Clear()
    {
        if ( m_HasCircle )
            BattleRoyaleUtils.Debug( "[GroundRing] circle cleared" );
        DeleteAll();
        m_HasCircle = false;
        m_HasAnchor = false;
    }

    void Update(vector anchor)
    {
        if ( !m_HasCircle )
            return;

        bool recut = !m_HasAnchor;
        if ( m_HasAnchor && vector.DistanceSq( anchor, m_LastAnchor ) > RECUT_DISTANCE * RECUT_DISTANCE )
            recut = true;

        if ( recut )
        {
            m_LastAnchor = anchor;
            m_HasAnchor = true;
            ComputeWindow( anchor );
            DropOutsideWindow();
            m_Pending = true;
        }

        if ( m_Pending )
        {
            SpawnMissing();
            if ( !m_Pending )
                BattleRoyaleUtils.Trace( "[GroundRing] arc ready: " + m_Segments.Count() + " segment(s)" );
        }
    }

    int GetSegmentCount()
    {
        return m_Segments.Count();
    }

    //! Which segment indices lie within DRAW_RADIUS of the anchor, in 2D.
    protected void ComputeWindow(vector anchor)
    {
        float cx = m_Center[0];
        float cz = m_Center[2];
        float ax = anchor[0];
        float az = anchor[2];
        float dx = ax - cx;
        float dz = az - cz;
        float d = Math.Sqrt( dx * dx + dz * dz );

        //--- Too far inside or outside: no point of the circle is within reach.
        if ( Math.AbsFloat( d - m_Radius ) > DRAW_RADIUS )
        {
            m_First = 0;
            m_Last = -1;
            return;
        }

        //--- The whole circle is within reach (small circle, or anchor near its centre).
        if ( d + m_Radius <= DRAW_RADIUS || d < 0.001 )
        {
            m_First = 0;
            m_Last = m_Count - 1;
            return;
        }

        //--- Law of cosines: half-angle of the arc whose points are exactly DRAW_RADIUS away.
        float cos_half = ( d * d + m_Radius * m_Radius - DRAW_RADIUS * DRAW_RADIUS ) / ( 2.0 * d * m_Radius );
        cos_half = Math.Clamp( cos_half, -1.0, 1.0 );
        float half = Math.Acos( cos_half );
        float mid = Math.Atan2( dz, dx );

        m_First = Math.Floor( ( mid - half ) / m_Step );
        m_Last = Math.Floor( ( mid + half ) / m_Step );
        if ( m_Last - m_First + 1 >= m_Count )
        {
            m_First = 0;
            m_Last = m_Count - 1;
        }
    }

    protected int WrapIndex(int k)
    {
        int idx = k % m_Count;
        if ( idx < 0 )
            idx = idx + m_Count;
        return idx;
    }

    protected bool InWindow(int idx)
    {
        if ( m_Last < m_First )
            return false;

        int offset = WrapIndex( idx - m_First );
        return offset <= m_Last - m_First;
    }

    protected void DropOutsideWindow()
    {
        array<int> drop = new array<int>();
        foreach ( int idx, Object seg : m_Segments )
        {
            if ( !InWindow( idx ) )
                drop.Insert( idx );
        }

        foreach ( int drop_idx : drop )
        {
            Object old = m_Segments.Get( drop_idx );
            if ( old )
                GetGame().ObjectDelete( old );
            m_Segments.Remove( drop_idx );
        }
    }

    protected void SpawnMissing()
    {
        int budget = SPAWN_BUDGET;
        for ( int k = m_First; k <= m_Last; k++ )
        {
            int idx = WrapIndex( k );
            if ( m_Segments.Contains( idx ) )
                continue;

            if ( budget <= 0 )
                return;
            budget--;

            Object seg = SpawnSegment( idx );
            m_Segments.Set( idx, seg );
        }

        m_Pending = false;
    }

    protected Object SpawnSegment(int idx)
    {
        float angle = m_Step * ( idx + 0.5 );
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

    protected void DeleteAll()
    {
        foreach ( int idx, Object seg : m_Segments )
        {
            if ( seg )
                GetGame().ObjectDelete( seg );
        }
        m_Segments.Clear();
        m_First = 0;
        m_Last = -1;
        m_Pending = false;
    }
}
#endif
