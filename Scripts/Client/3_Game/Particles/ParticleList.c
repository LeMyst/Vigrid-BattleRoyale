modded class ParticleList
{
    //--- Blue motes rising off the current zone edge (BattleRoyaleZoneMotes). Colour is baked into
    //--- zone_mote.edds: vanilla's particle materials ignore the tint. Two copies until one is proven
    //--- to load: the material referenced by plain path, and by {GUID}path with .meta files.
    static const int BR_ZONE_EDGE_MOTES = RegisterParticle( "Vigrid-BattleRoyale/Data/particles/", "zone_edge_motes" );
    static const int BR_ZONE_EDGE_MOTES_GUID = RegisterParticle( "Vigrid-BattleRoyale/Data/particles/", "zone_edge_motes_guid" );
}
