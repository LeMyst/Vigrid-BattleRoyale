#ifdef SERVER
//! The stats API's finish body (BattleRoyaleApiReport.Finish): the last_match.json rows, every one of
//! them. Here rather than beside the other bodies in 3_Game because the row class is 4_World.
class BattleRoyaleApiFinishBody
{
    string token;
    ref array<ref BattleRoyaleLastMatchRow> rows;

    void BattleRoyaleApiFinishBody()
    {
        rows = new array<ref BattleRoyaleLastMatchRow>();
    }
}
#endif
