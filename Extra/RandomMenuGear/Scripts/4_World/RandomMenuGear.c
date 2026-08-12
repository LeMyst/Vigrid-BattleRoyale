/**
 *  Rolls a random outfit onto the main-menu intro character.
 *
 *  Gear is applied straight to the spawned object with GameInventory.CreateAttachmentEx(). It is
 *  deliberately NOT routed through MenuDefaultCharacterData.SetDefaultAttachment(): that map is
 *  serialized to the server on connect (MenuDefaultCharacterData.SerializeCharacterData) and
 *  written to the local character save, so writing into it would leak random menu gear into the
 *  player's real spawn loadout. Decorating the object only keeps this purely cosmetic.
 *
 *  Pool convention: an empty-string entry means "leave this slot bare", so a slot can roll empty
 *  some of the time. Vanilla's MenuDefaultCharacterData.GenerateRandomEquip uses the same trick.
 *
 *  Every classname below was verified against P:\dz\**\config.cpp. The weapon slots rely on
 *  Rifle_Base declaring inventorySlot[] = {"Shoulder","Melee"} (P:\dz\data\config.cpp), which is
 *  what makes a rifle render as a proper slung proxy on the static menu idle pose.
 */
class RandomMenuGear
{
	//--- Parallel arrays: s_Slots[i] is filled from s_Pools[i]. Ordered torso/legs/feet first, then
	//--- the layered items, then the weapons, so nothing is attached before the layer under it.
	protected static ref array<int> s_Slots;
	protected static ref array<ref array<string>> s_Pools;

	//--------------------------------------------------------------------------------------------
	// Apply
	//--------------------------------------------------------------------------------------------
	//! Clears every managed slot on the character and re-rolls it. Safe to call repeatedly.
	static void Apply(Man player)
	{
		if (!player)
			return;

		if (GetGame().IsDedicatedServer())
			return;

		GameInventory inv = player.GetInventory();
		if (!inv)
			return;

		BuildPools();

		int slotId;
		EntityAI current;
		array<string> pool;
		string type;

		for (int i = 0; i < s_Slots.Count(); ++i)
		{
			slotId = s_Slots.Get(i);
			pool = s_Pools.Get(i);

			//--- Clear whatever is there: leftovers from the character save AND from the last roll.
			current = inv.FindAttachment(slotId);
			if (current)
				GetGame().ObjectDelete(current);

			if (!pool)
				continue;

			if (pool.Count() == 0)
				continue;

			type = pool.GetRandomElement();
			if (type == "")
				continue;

			inv.CreateAttachmentEx(type, slotId);
		}
	}

	//--------------------------------------------------------------------------------------------
	// BuildPools
	//--------------------------------------------------------------------------------------------
	//! Lazy one-time init. InventorySlots constants are populated by the engine from CfgSlots, so
	//! they are only safe to read once the game is up - not at static-initializer time.
	protected static void BuildPools()
	{
		if (s_Slots)
			return;

		s_Slots = new array<int>;
		s_Pools = new array<ref array<string>>;

		ref array<string> body = {
			"TacticalShirt_Black",
			"M65Jacket_Black",
			"BomberJacket_Black",
			"Hoodie_Black",
			"Raincoat_Black",
			"GorkaEJacket_Summer"
		};
		AddSlot(InventorySlots.BODY, body);

		ref array<string> legs = {
			"CargoPants_Black",
			"GorkaPants_Summer",
			"Jeans_Blue",
			"CanvasPants_Beige"
		};
		AddSlot(InventorySlots.LEGS, legs);

		ref array<string> feet = {
			"CombatBoots_Black",
			"MilitaryBoots_Black",
			"AthleticShoes_Black",
			"JungleBoots_Black"
		};
		AddSlot(InventorySlots.FEET, feet);

		ref array<string> vest = {
			"PlateCarrierVest",
			"HighCapacityVest_Black",
			"PressVest_Blue",
			"SmershVest",
			""
		};
		AddSlot(InventorySlots.VEST, vest);

		ref array<string> back = {
			"AliceBag_Green",
			"MountainBag_Blue",
			"AssaultBag_Black",
			"HuntingBag",
			"TortillaBag"
		};
		AddSlot(InventorySlots.BACK, back);

		ref array<string> headgear = {
			"BaseballCap_Black",
			"Mich2001Helmet",
			"GorkaHelmet",
			"BoonieHat_Black",
			"Ushanka_Black",
			"TankerHelmet"
		};
		AddSlot(InventorySlots.HEADGEAR, headgear);

		//--- Two empty entries: a mask covers the face, so it should be the exception not the rule.
		ref array<string> mask = {
			"GasMask",
			"BalaclavaMask_Blackskull",
			"Balaclava3Holes_Black",
			"NioshFaceMask",
			"",
			""
		};
		AddSlot(InventorySlots.MASK, mask);

		ref array<string> eyewear = {
			"AviatorGlasses",
			"TacticalGoggles",
			"DesignerGlasses",
			""
		};
		AddSlot(InventorySlots.EYEWEAR, eyewear);

		ref array<string> gloves = {
			"TacticalGloves_Black",
			"WorkingGloves_Black"
		};
		AddSlot(InventorySlots.GLOVES, gloves);

		ref array<string> armband = {
			"Armband_Black",
			"Armband_Red",
			""
		};
		AddSlot(InventorySlots.ARMBAND, armband);

		//--- Slung across the back.
		ref array<string> shoulder = {
			"M4A1",
			"AKM",
			"Mosin9130",
			"SKS",
			"Winchester70",
			"FAL",
			"CZ527",
			"B95",
			"Repeater"
		};
		AddSlot(InventorySlots.SHOULDER, shoulder);

		ref array<string> melee = {
			"Machete",
			"FirefighterAxe",
			"BaseballBat",
			"Pickaxe",
			"SledgeHammer"
		};
		AddSlot(InventorySlots.MELEE, melee);
	}

	//--------------------------------------------------------------------------------------------
	// AddSlot
	//--------------------------------------------------------------------------------------------
	protected static void AddSlot(int slotId, array<string> types)
	{
		s_Slots.Insert(slotId);
		s_Pools.Insert(types);
	}
}
