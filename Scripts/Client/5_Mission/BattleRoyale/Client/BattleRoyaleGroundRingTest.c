#ifdef DIAG_DEVELOPER
#ifndef SERVER
/**
 *  Diag probe: draw the zone edge ON the ground with flat quads, as local client objects.
 *
 *  Measured 2026-11-09 (1.30 diag):
 *    - Engine decals are a dead end: script has no handle on the terrain entity, CreateDecal(NULL)
 *      is an access violation, CreateLandMarkDecal(NULL) returns NULL, and on a real object
 *      CreateDecal rejects rvmats ("Unable to load material. Error no. 2").
 *    - Flat quads on SurfaceY + terrain normal follow the slope; 5 cm up removed the z-fighting.
 *    - Rocks ARE IsRock(), but a flat quad cannot follow a rock's curve, and RaycastRV's
 *      contact_dir is the RAY direction, not a normal. Terrain only, for now.
 *
 *  Model "Band" is BR_GroundSegment (Models/Shapes, Tools/make_ground_segment.py): 2 m x 1 m,
 *  recoloured live through its "camo" hidden selection.
 */
class BattleRoyaleGroundRingTest
{
    static const float SURFACE_OFFSET = 0.05;
    static const int MODEL_BAND = 2;

    protected ref array<Object> m_Segments = new array<Object>();

    static string GetModelP3D(int index)
    {
        if ( index == 1 )
            return "DZ\\data\\data\\particleeffects\\excretion\\vomit_blood_ground.p3d";
        return "DZ\\gear\\cooking\\fireplace_spot_snow.p3d";
    }

    //! RGB part of the procedural colour. Order MUST match the "Ground Ring Color" diag item list.
    static string GetColorRGB(int index)
    {
        switch ( index )
        {
            case 1: return "1,0.45,0";
            case 2: return "1,0.85,0.1";
            case 3: return "1,1,1";
            case 4: return "0.05,0.25,1";
            case 5: return "0.3,0.6,1";
            case 6: return "0,0.85,1";
            case 7: return "0.6,0.15,0.9";
            case 8: return "0,0,0";
        }
        return "0.85,0.05,0.05";
    }

    static string GetColorTexture(int index, float alpha)
    {
        string rgb = GetColorRGB( index );
        return "#(argb,8,8,3)color(" + rgb + "," + alpha + ",ca)";
    }

    void Spawn(vector center, float radius, int model_index, float scale, int color_index, float alpha, bool flip)
    {
        Clear();

        float cx = center[0];
        float cz = center[2];
        Print( "[GroundRing] spawn center=" + center.ToString() + " radius=" + radius );
        string flip_text = "no";
        if ( flip )
            flip_text = "yes";
        Print( "[GroundRing] model=" + model_index + " scale=" + scale );
        Print( "[GroundRing] color=" + color_index + " flip=" + flip_text );

        //--- Band segments are 2 m long, so they abut at 2 * scale; the vanilla blobs overlap a little.
        float spacing = 0.8 * scale;
        if ( model_index == MODEL_BAND )
            spacing = 2.0 * scale;

        int count = Math.Ceil( Math.PI2 * radius / spacing );
        if ( count < 24 )
            count = 24;

        string texture = GetColorTexture( color_index, alpha );
        Print( "[GroundRing] texture=" + texture );
        int failed = 0;
        for ( int i = 0; i < count; i++ )
        {
            //--- Centre each segment on its chord, so neighbours meet rather than overlap.
            float angle = Math.PI2 * ( i + 0.5 ) / count;
            float x = cx + Math.Cos( angle ) * radius;
            float z = cz + Math.Sin( angle ) * radius;
            float y = GetGame().SurfaceY( x, z );
            vector p = Vector( x, y, z );
            vector normal = GetGame().SurfaceGetNormal( x, z );

            //--- Tangent to the ring, flattened onto the terrain plane so the quad lies on the slope.
            vector tangent = Vector( -Math.Sin( angle ), 0, Math.Cos( angle ) );
            float along_up = vector.Dot( tangent, normal );
            vector dir = tangent - ( normal * along_up );
            dir.Normalize();

            vector up = normal;
            if ( flip )
                up = normal * -1.0;

            Object seg = CreateSegment( model_index, p );
            if ( !seg )
            {
                failed++;
                continue;
            }

            vector mat[4];
            Math3D.DirectionAndUpMatrix( dir, up, mat );
            mat[3] = p + ( normal * SURFACE_OFFSET );
            seg.SetTransform( mat );
            seg.SetScale( scale );

            if ( model_index == MODEL_BAND )
                ApplyTexture( seg, texture );

            m_Segments.Insert( seg );
        }

        Print( "[GroundRing] segments: created=" + m_Segments.Count() + " failed=" + failed );
    }

    //! Recolour the ring already on the ground - no respawn, so colours can be compared in place.
    void ApplyColor(int color_index, float alpha)
    {
        string texture = GetColorTexture( color_index, alpha );
        int applied = 0;
        foreach ( Object seg : m_Segments )
        {
            if ( ApplyTexture( seg, texture ) )
                applied++;
        }
        Print( "[GroundRing] " + texture + " applied to " + applied + " segment(s)" );
    }

    void Clear()
    {
        foreach ( Object seg : m_Segments )
        {
            if ( seg )
                GetGame().ObjectDelete( seg );
        }
        m_Segments.Clear();
    }

    protected Object CreateSegment(int model_index, vector p)
    {
        if ( model_index == MODEL_BAND )
            return GetGame().CreateObjectEx( "BR_GroundSegment", p, ECE_LOCAL );

        return GetGame().CreateStaticObjectUsingP3D( GetModelP3D( model_index ), p, "0 0 0", 1.0, true );
    }

    protected bool ApplyTexture(Object seg, string texture)
    {
        EntityAI entity = EntityAI.Cast( seg );
        if ( !entity )
            return false;

        entity.SetObjectTexture( 0, texture );
        return true;
    }
}
#endif
#endif
