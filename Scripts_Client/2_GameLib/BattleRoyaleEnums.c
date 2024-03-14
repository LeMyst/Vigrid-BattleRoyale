//#ifdef JM_COT
enum BattleRoyaleCOTStateMachineRPC
{
    INVALID = 11000, //Do not move this

    Next,
    Pause,
    Resume,
    TestSpectator,
    AddFakePlayer,
    AddFakeGroup,
    SpawnAirdrop,
    SpawnHorde,
    SpawnChemicals,

    COUNT //Do not move this
}
//#endif