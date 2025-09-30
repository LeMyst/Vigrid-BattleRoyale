#ifdef Vehicle3PP
modded class CarScript
{
	protected bool m_Allow3PPCamera;
	protected bool m_DriverOnly;

	override void EEInit()
	{
		if (GetGame().IsDedicatedServer())
		{
			array<string> ForcedWhitelist = {
				"OffroadHatchback",
				"CivilianSedan",
				"Hatchback_02",
				"Sedan_02",
				"Truck_01_Covered",
				"Offroad_02"
			};

			array<string> vehicles = GetVehicle3PPConfig().GetWhitelist();

			// Ensure forced vehicles are always in the whitelist
			foreach (string forced_vehicle : ForcedWhitelist)
			{
				if (vehicles.Find(forced_vehicle) == -1)
					vehicles.Insert(forced_vehicle);
			}

			m_DriverOnly = GetVehicle3PPConfig().IsDriverOnly();

			// if no vehicles are specified, 3PP for all
			if (!vehicles || vehicles.Count() < 1)
			{
				m_Allow3PPCamera = true;
			}
			else
			{
				foreach (string vehicle : vehicles)
				{
					if (IsInherited(vehicle.ToType()) || vehicle == GetType() || KindOf(vehicle))
					{
						m_Allow3PPCamera = true;
						break;
					}
				}
			}

			SetSynchDirty();
		}

		super.EEInit();
	}
}
#endif