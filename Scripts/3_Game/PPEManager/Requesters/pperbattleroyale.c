class PPERequester_BattleRoyale extends PPERequester_GameplayBase
{
	override protected void OnStart(Param par = null)
	{
		super.OnStart();

        // Red tint
		SetTargetValueColor(PostProcessEffectType.Glow,PPEGlow.PARAM_COLORIZATIONCOLOR,{0.0,0.3,0.3,0.0},PPEGlow.L_23_GLASSES,PPOperators.SUBSTRACT);
	}
}
