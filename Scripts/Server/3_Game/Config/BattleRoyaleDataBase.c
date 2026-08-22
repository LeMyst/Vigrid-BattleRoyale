#ifdef SERVER
class BattleRoyaleDataBase
{
	// Path to the config file from profile folder
	string GetProfilePath()
	{
		return "";
	}

	// Path to the config file from mission folder
	string GetMissionPath()
	{
		return "";
	}

	void Load() {}  // Load the config from the file
	void LoadMission() {}  // Load the config from the mission file
	void Save() {}  // Save the config to the file
	void Upgrade() {}  // Upgrade the config to the latest version if needed

	//--- Clamp settings that are internally inconsistent, so a misconfiguration degrades into a
	//--- playable match instead of halting boot. Distinct from Upgrade(), which migrates between
	//--- versions: Upgrade runs inside Load(), before the mission pass, while Validate runs after
	//--- BOTH passes so a mission override is checked too.
	//---
	//--- An implementation MUST NOT call Save(). BattleRoyaleConfig.Load() already re-saves before
	//--- the mission pass, so persisting a clamp here would overwrite the admin's intent in their
	//--- profile JSON permanently - the clamp is meant to be in memory, for this boot only.
	void Validate() {}

	//--- ⚠️ THE MISSION PASS CLEARS EVERY `ref array` THE MISSION FILE DOES NOT MENTION, AND THESE
	//--- ARE THE TOOLS FOR SURVIVING IT.
	//---
	//--- LoadMission() deserializes into the SAME instance the profile pass filled, and the documented
	//--- model - "not a merge; only keys present in the mission JSON get overwritten" - holds for
	//--- scalars and is FALSE for arrays: an absent array key leaves the field empty rather than
	//--- untouched, exactly like the initialiser that does not survive deserialization.
	//---
	//--- Measured 2026-08-19 on a mission pois_settings.json carrying only two keys: with the file
	//--- present a guard keyed on one of the omitted arrays reported 0 hits and walked all 306 POIs;
	//--- with the identical file renamed away, 229 hits. The profile JSON on disk held the correct
	//--- values the whole time, which is what makes this so hard to see from the outside - the setting
	//--- reads correctly and the only symptom is a feature that quietly does nothing.
	//---
	//--- The pattern in a LoadMission() is: copy each array with one of these BEFORE the deserialize,
	//--- then restore it afterwards if and only if the mission pass left it empty. A mission that
	//--- really specifies the array still overrides it; a mission that says nothing no longer disables
	//--- it.
	//---
	//--- ⚠️ AND THE COPY MUST BE OF THE CONTENTS, NOT THE REFERENCE. `array<string> kept = field;`
	//--- compiles, reads correctly and does nothing at all: the deserializer clears the existing array
	//--- IN PLACE rather than assigning a fresh one, so both names denote the same object and the
	//--- "snapshot" is emptied along with the field. That mistake cost a build here, and it had
	//--- already silently disabled the admins_steamid64 mission-lock in BattleRoyaleGameData.
	ref array<string> CopyStrings(array<string> source)
	{
		ref array<string> copy = new array<string>;
		if (!source)
			return copy;

		int i;
		for (i = 0; i < source.Count(); i++)
		{
			string value = source.Get(i);
			copy.Insert(value);
		}

		return copy;
	}

	ref array<int> CopyInts(array<int> source)
	{
		ref array<int> copy = new array<int>;
		if (!source)
			return copy;

		int i;
		for (i = 0; i < source.Count(); i++)
		{
			int value = source.Get(i);
			copy.Insert(value);
		}

		return copy;
	}

	ref array<float> CopyFloats(array<float> source)
	{
		ref array<float> copy = new array<float>;
		if (!source)
			return copy;

		int i;
		for (i = 0; i < source.Count(); i++)
		{
			float value = source.Get(i);
			copy.Insert(value);
		}

		return copy;
	}
};
