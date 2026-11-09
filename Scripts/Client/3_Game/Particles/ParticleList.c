modded class ParticleList
{
    //--- Blue motes rising off the current zone edge (BattleRoyaleZoneMotes). Plain-text .ptc in the Data PBO,
    //--- identical but for the motes' alpha peak: an additive emissive sprite washes out to white when too bright.
    static const int BR_ZONE_EDGE_MOTES_BRIGHT = RegisterParticle( "Vigrid-BattleRoyale/Data/particles/", "zone_edge_motes_bright" );
    static const int BR_ZONE_EDGE_MOTES_MID = RegisterParticle( "Vigrid-BattleRoyale/Data/particles/", "zone_edge_motes_mid" );
    static const int BR_ZONE_EDGE_MOTES_DIM = RegisterParticle( "Vigrid-BattleRoyale/Data/particles/", "zone_edge_motes_dim" );
}
