//#ifdef JM_COT
enum BattleRoyaleCOTStateMachineRPC
{
    INVALID = 11000, //Do not move this

    Next,
    Pause,
    Resume,
#ifdef SPECTATOR
    TestSpectator,
#endif
    AddFakePlayer,
    AddFakeGroup,
    SpawnAirdrop,
    SpawnHorde,
    SpawnChemicals,

    COUNT //Do not move this
}
//#endif


enum BattleRoyaleMatchMakingState
{
	INVALID = -1, //Do not move this

	None,
	Searching,
	Connecting,
	Failed,
	Success
}