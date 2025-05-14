class Basic_Zone: House
{
	void Basic_Zone()
	{
		Print("Basic_Zone::Basic_Zone");
//		SetEventMask(EntityEvent.POSTFRAME);
	}

	void ~Basic_Zone()
	{
		Print("Basic_Zone::~Basic_Zone");
	}

//	override void EOnPostFrame(IEntity other, int extra)
//	{
//		Print("Basic_Zone::EOnPostFrame");
//		BattleRoyaleUtils.Trace("Basic_Zone::EOnPostFrame");
//
//		vector transform[4];
//		GetTransform(transform);
//		BattleRoyaleUtils.Trace("Zone transform: " + transform[0] + " " + transform[1] + " " + transform[2]);
//		transform[0] = transform[0] * 4;
//		transform[1] = transform[1] * 4;
//		transform[2] = transform[2] * 4;
//		BattleRoyaleUtils.Trace("Zone transform: " + transform[0] + " " + transform[1] + " " + transform[2]);
//
//		SetTransform(transform);
//		Update();
//	}

	string GetZoneName()
	{
		return "Basic_Zone";
	}
}

class Basic_Zone_int: House
{
	void Basic_Zone_int()
	{
		Print("Basic_Zone_int::Basic_Zone_int");
	}

	void ~Basic_Zone_int()
	{
		Print("Basic_Zone_int::~Basic_Zone_int");
	}

	string GetZoneName()
	{
		return "Basic_Zone_int";
	}
}

class Basic_Zone_ext: House
{
	void Basic_Zone_ext()
	{
		Print("Basic_Zone_ext::Basic_Zone_ext");
	}

	void ~Basic_Zone_ext()
	{
		Print("Basic_Zone_ext::~Basic_Zone_ext");
	}

	string GetZoneName()
	{
		return "Basic_Zone_ext";
	}
}