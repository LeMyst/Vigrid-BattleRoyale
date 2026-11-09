#ifndef SERVER
/**
 *  Something laid at even steps along a zone circle, client-side only, of which only the arc within
 *  DRAW_RADIUS of an anchor (the camera) exists. Re-cut when the anchor moves RECUT_DISTANCE or the
 *  circle changes, so a 2 km circle never puts 12 km of things in the world.
 *
 *  Subclasses own the things themselves: SpawnAt / DropAt / DropAll / HasAt / CollectLive.
 */
class BattleRoyaleRingArc
{
    static const float DRAW_RADIUS = 250.0;
    static const float RECUT_DISTANCE = 10.0;

    //--- Set by the subclass constructor: a base constructor cannot take arguments from it.
    protected float m_Spacing = 2.0;    // metres between two indices along the circle
    protected int m_SpawnBudget = 10;   // per Update, so a lock does not hitch one frame
    protected string m_Tag = "[RingArc]";

    protected vector m_Center;
    protected float m_Radius;
    protected bool m_HasCircle;

    protected int m_Count;
    protected float m_Step;

    //--- Wanted index window [m_First, m_Last], unwrapped (may be negative or >= m_Count).
    protected int m_First;
    protected int m_Last;
    protected bool m_Pending;

    protected vector m_LastAnchor;
    protected bool m_HasAnchor;

    void BattleRoyaleRingArc()
    {
        m_Last = -1;
    }

    //! Set the circle. Cheap when nothing changed, so it can be called every frame.
    void SetCircle(vector center, float radius)
    {
        if ( m_HasCircle && m_Center == center && m_Radius == radius )
            return;

        ResetArc();
        m_Center = center;
        m_Radius = radius;
        m_HasCircle = true;

        m_Count = Math.Ceil( Math.PI2 * radius / m_Spacing );
        if ( m_Count < 8 )
            m_Count = 8;
        m_Step = Math.PI2 / m_Count;

        OnCircleChanged();
        m_HasAnchor = false;

        string circle_text = center.ToString() + " r=" + radius;
        BattleRoyaleUtils.Debug( m_Tag + " circle " + circle_text + " count=" + m_Count );
    }

    //! No circle any more (cleared by the server, or the match reset).
    void Clear()
    {
        if ( m_HasCircle )
            BattleRoyaleUtils.Debug( m_Tag + " circle cleared" );
        ResetArc();
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
                BattleRoyaleUtils.Trace( m_Tag + " arc ready: " + GetLiveCount() + " item(s)" );
        }
    }

    int GetLiveCount()
    {
        array<int> live = new array<int>();
        CollectLive( live );
        return live.Count();
    }

    //--- Subclass hooks.
    protected void OnCircleChanged() {}
    //! false = could not create it now; the rest of the arc waits for the next re-cut.
    protected bool SpawnAt(int idx) { return true; }
    protected void DropAt(int idx) {}
    protected void DropAll() {}
    protected bool HasAt(int idx) { return false; }
    protected void CollectLive(array<int> indices) {}

    //! Angle of index idx: the middle of its step.
    protected float AngleAt(int idx)
    {
        return m_Step * ( idx + 0.5 );
    }

    //! Which indices lie within DRAW_RADIUS of the anchor, in 2D.
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
        array<int> live = new array<int>();
        CollectLive( live );
        foreach ( int idx : live )
        {
            if ( !InWindow( idx ) )
                DropAt( idx );
        }
    }

    protected void SpawnMissing()
    {
        int budget = m_SpawnBudget;
        for ( int k = m_First; k <= m_Last; k++ )
        {
            int idx = WrapIndex( k );
            if ( HasAt( idx ) )
                continue;

            if ( budget <= 0 )
                return;
            budget--;

            if ( !SpawnAt( idx ) )
            {
                BattleRoyaleUtils.Debug( m_Tag + " spawn failed at index " + idx + ", waiting for the next re-cut" );
                break;
            }
        }

        m_Pending = false;
    }

    protected void ResetArc()
    {
        DropAll();
        m_First = 0;
        m_Last = -1;
        m_Pending = false;
    }
}
#endif
