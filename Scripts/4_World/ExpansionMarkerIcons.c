/*

	I highly recommend NEVER reading this code. 
	
	If the marker system is changed, and this code breaks, re-fucking-do it. This abomination should never have existed in the first place.
	
	- kegan
	
	
	ps. if you want to add more icons than a circle, you'll need to expand this with a second json file.... good luck
*/
modded class ExpansionMarkerIcons
{
	protected static int circle_marker_index = -1;
	protected static autoptr array< ref ExpansionMarkerIcon > m_MarkersIconList_BR;
	
	
	static string GetCircleName()
	{
		SetupBRMarkers();
		return GetMarkerName(circle_marker_index);
	}
	static string GetCirclePath()
	{
		SetupBRMarkers();
		return GetMarkerName(circle_marker_index);
	}
	static string GetCircleIndex()
	{
		SetupBRMarkers();
		return GetMarkerName(circle_marker_index);
	}
	
	static void SetupBRMarkers()
	{
		if(circle_marker_index != -1)
			return;
		
		int c = Count(); //ensure circle marker index has a value
	}
	static void SetupExpansionMarkers()
	{
		if ( m_MarkersIconList_BR == NULL )
		{
			JsonFileLoader< ref array< ref ExpansionMarkerIcon > >.JsonLoadFile( "DayZExpansion/Scripts/Data/Markers.json", m_MarkersIconList_BR );
		}
	}
	
	
	override static int Count()
	{
		SetupExpansionMarkers();
		int base_value = m_MarkersIconList_BR.Count();
		
		circle_marker_index = base_value;
		
		return base_value + 1;
	}
	override static string GetMarkerPath(int index)
	{
		SetupExpansionMarkers();
		SetupBRMarkers();
		if(index == circle_marker_index)
		{
			return "BattleRoyale\\GUI\\icons\\marker\\marker_circle.paa";
		}
		else
		{
			if ( index < 0 )
			{
				return "";
			}

			if ( index >= m_MarkersIconList_BR.Count() )
			{
				return "";
			}

			return m_MarkersIconList_BR.Get( index ).Path;
		}
	}
	override static int GetMarkerIndex(string findPath)
	{
		Print("GetMarkerIndex");
		Print(findPath);
		
		SetupExpansionMarkers();
		SetupBRMarkers();
		if(findPath == "BattleRoyale\\GUI\\icons\\marker\\marker_circle.paa")
		{
			return circle_marker_index;
		}
		else
		{
			if (findPath != "")
			{
				
				for ( int i = 0; i < m_MarkersIconList_BR.Count(); ++i )
				{
					ExpansionMarkerIcon icon = m_MarkersIconList_BR.Get(i);
					int index;
					string iconPath = icon.Path;

					if (findPath == iconPath)
					{
						index = i;

						if ( index < 0 )
						{
							return -1;
						}

						if ( index >= m_MarkersIconList_BR.Count() )
						{
							return -1;
						}
					}				
				}
			}
			return index;
		}
	}
	override static string GetMarkerName( int index )
	{
		SetupExpansionMarkers();
		SetupBRMarkers();
		if(index == circle_marker_index)
		{
			return "Circle";
		}
		else
		{
			if ( index < 0 )
			{
				return "";
			}

			if ( index >= m_MarkersIconList_BR.Count() )
			{
				return "";
			}

			return m_MarkersIconList_BR.Get( index ).Name;
		}
	}
}