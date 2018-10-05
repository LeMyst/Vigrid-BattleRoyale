modded class GameInventory
{
	static bool CheckDropRequest (notnull Man requestingPlayer, EntityAI item)
	{
		return true;
	}
	static bool CheckTakeItemRequest (notnull Man requestingPlayer, EntityAI item, EntityAI target)
	{
		return true;
	}
	static bool CheckMoveToDstRequest (notnull Man requestingPlayer, EntityAI item, notnull InventoryLocation dst)
	{
		return true;
	}
	static bool CheckSwapItemsRequest (notnull Man requestingPlayer, EntityAI item1, EntityAI item2)
	{
		return true;
	}
}