class BR_Zone: House
{
	void BR_Zone()
	{
		Print("BR_Zone::BR_Zone");
	}

	void ~BR_Zone()
	{
		Print("BR_Zone::~BR_Zone");
	}

	string GetZoneName()
	{
		return "BR_Zone";
	}
}