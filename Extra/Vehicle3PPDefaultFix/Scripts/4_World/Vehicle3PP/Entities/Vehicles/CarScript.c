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

			Vehicle3PPConfig v3pp_config = GetVehicle3PPConfig();
			array<string> vehicles;
			if (v3pp_config)
			{
				vehicles = v3pp_config.GetWhitelist();
				m_DriverOnly = v3pp_config.IsDriverOnly();
			}

			// if no vehicles are specified, 3PP for all
			if (!vehicles || vehicles.Count() < 1)
			{
				m_Allow3PPCamera = true;
			}
			else
			{
				// Forced only into a non-empty whitelist: injecting into an empty one would revoke
				// "3PP for all" from every other vehicle.
				foreach (string forced_vehicle : ForcedWhitelist)
				{
					if (vehicles.Find(forced_vehicle) == -1)
						vehicles.Insert(forced_vehicle);
				}

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