modded class ActionConstructor
{
	override void ConstructActions(out array<ref ActionBase> actions, out TIntArray suactions, out TIntArray cactions, out TIntArray iactions)
	{
		//init default dayz actions
		super.ConstructActions(actions,suactions,cactions,iactions);
		//add dayzbr actions
		actions.Insert(new ActionDrinkMonsta);
	}
}