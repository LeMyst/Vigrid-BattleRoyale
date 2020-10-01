
class BattleRoyaleSpectatorMapEntityData
{
    vector position;
    vector direction;
    string name;

    void BattleRoyaleSpectatorMapEntityData( string displayname, vector pos, vector dir )
    {
        name = displayname;
        position = pos;
        direction = dir;
    }
}