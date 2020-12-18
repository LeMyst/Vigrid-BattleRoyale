/*
Example Usage


ref LootReader lr = new LootReader();
lr.ParseLoot("$CurrentDir:\\mpmissions\\BattleRoyale.ChernarusPlusGloom\mapgroupproto.xml"); //Note that the path cannot yet be dynamic (broken function by dayz)

string object_class_name = ...;
object_class_name.ToLower();

array<vector> local_coord_positions = lr.GetAllLootPositions(object_class_name);



*/


class LootReader
{
    static ref LootReader GetReader()
    {
        if(!m_Singleton)
        {
            m_Singleton = new LootReader;
        }
        return m_Singleton;
    }
    static ref LootReader m_Singleton;

    ref LootReaderXMLCallback _callback;

    void ReadAsync(string path)
    {
        CF_XML api = GetXMLApi();
        _callback = new LootReaderXMLCallback( this );
        api.Read(path, _callback);
    }

    bool IsReady()
    {
        if(_callback)
        {
            return _callback.IsComplete();
        }
        return false;
    }
    bool ContainsObject(string name)
    {
        if(_callback && IsReady())
        {
            return _callback.Contains( name );
        }
        return false;
    }

 //TODO: LOOT RAYCASTING #2
 //this method needs to return pos, height, and radius (perhaps we have it return ref BattleRoyaleLootPoint as an array?)
    ref array<vector> GetAllLootPositions(string name) 
    {
        ref array<vector> positions = new array<vector>();
        if(IsReady() && ContainsObject(name))
        {
            ref BattleRoyaleLootPosition loot_entry = _callback.Get(name);
            if(loot_entry)
            {
                ref array<ref BattleRoyaleLootContainer> containers = loot_entry.GetContainers();
                for(int i = 0; i < containers.Count(); i++)
                {
                    ref BattleRoyaleLootContainer container = containers.Get(i);
                    if(container)
                    {
                        ref array<ref BattleRoyaleLootPoint> points = container.GetPoints();
                        for(int j = 0; j < points.Count(); j++)
                        {
                            ref BattleRoyaleLootPoint point = points.Get(j);
                            if(point)
                            {
                                //TODO: we need to also return height and radius (perhaps we could return "BattleRoyaleLootPoint" as an object in here?)
                                vector position = point.GetPosition();
                                positions.Insert(position);
                            }
                            else
                            {
                                Error("Null point in: " + name);
                            }
                            
                        }
                    }
                    else
                    {
                        Error("Null container in: " + name);
                    }
                    
                }
            }
            else
            {
                Error("Loot Entry does not exist for: " + name)
            }
            
        }

        return positions;
    }
}
class LootReaderXMLCallback extends CF_XML_Callback
{
    protected ref LootReader p_LootReader;
    protected bool b_IsComplete;
    protected ref map<string, ref BattleRoyaleLootPosition> m_Entries;

    void LootReaderXMLCallback(ref LootReader reader)
    {
        p_LootReader = reader;
        b_IsComplete = false;
        m_Entries = new map<string, ref BattleRoyaleLootPosition>();
    }
    bool Contains(string name)
    {
        string check = name;
        check.ToLower();
        return m_Entries.Contains(check);
    }
    bool IsComplete()
    {
        return b_IsComplete;
    }
    ref BattleRoyaleLootPosition Get(string name)
    {
        string check = name;
        check.ToLower();
        return m_Entries.Get(check);
    }

    override void OnStart( ref CF_XML_Document document )
    {
        Print("XML Reading...");
        Print(document);
    }

    override void OnFailure( ref CF_XML_Document document )
    {
        b_IsComplete = true;
        Print("Failed Reading XML");
        Print(document);
    }

    override void OnSuccess( ref CF_XML_Document document )
    {
        CF_XML_Element ele = document.Get( 1 ).GetContent(); //read <prototype>
        for ( int i = 0; i < ele.Count(); ++i )
		{
			CF_XML_Tag tag = ele.Get( i );
			if ( tag.GetName() != "group" )
				continue; //this is triggered when we hit "defaults"

			ref BattleRoyaleLootPosition entry = new BattleRoyaleLootPosition();
            string name = tag.GetAttribute( "name" ).ValueAsString();
            name.ToLower();
			m_Entries.Insert(name, entry);
			
			array< CF_XML_Tag > containers = tag.GetContent().Get( "container" );
			array< CF_XML_Tag > usages = tag.GetContent().Get( "usage" );
            int j;
            for(j = 0; j < usages.Count(); ++j)
            {
                CF_XML_Tag usage = usages[j];
                name = usage.GetAttribute("name").ValueAsString();
                name.ToLower();
                entry.AddUsage(name);
            }

			for (j = 0; j < containers.Count(); ++j )
			{
                ref BattleRoyaleLootContainer container = new BattleRoyaleLootContainer();
                entry.AddContainer(container);

				array< CF_XML_Tag > points = containers[j].GetContent().Get( "point" );
				array< CF_XML_Tag > categories = containers[j].GetContent().Get( "category" );
				array< CF_XML_Tag > tags = containers[j].GetContent().Get( "tag" );
                int k;
                for(k = 0; k < categories.Count(); ++k)
                {
                    CF_XML_Tag category = categories[k];
                    name = category.GetAttribute("name").ValueAsString();
                    name.ToLower();
                    container.AddCategory(name);

                }
                for(k = 0; k < tags.Count(); ++k)
                {
                    CF_XML_Tag tag_att = tags[k];
                    name = tag_att.GetAttribute("name").ValueAsString();
                    name.ToLower();
                    container.AddTag(name);
                }
				for (k = 0; k < points.Count(); ++k )
				{
					vector point_pos = "0 0 0";
                    int flags = 0;
                    float height = 0;
                    float range = 0;
                    CF_XML_Attribute attrib = NULL;

					attrib = points[k].GetAttribute( "pos" );
					if ( attrib )
						point_pos = attrib.ValueAsVector();

					attrib = points[k].GetAttribute( "height" );
					if ( attrib )
						height = attrib.ValueAsFloat();

					attrib = points[k].GetAttribute( "range" );
					if ( attrib )
						range = attrib.ValueAsFloat();

					attrib = points[k].GetAttribute( "flags" );
					if ( attrib )
						flags = attrib.ValueAsInt();

					container.AddPoint( point_pos, range, height, flags );
				}
			}
		}

        
        b_IsComplete = true;
        
        // some debug logging
        Print("==== Loot XML Reading Complete! ====");
        Print("Found " + m_Entries.Count().ToString() + " entries!");
    }
}
class BattleRoyaleLootPosition
{
    protected ref array<string> a_Usages;
    protected ref array<ref BattleRoyaleLootContainer> a_Containers;

    void BattleRoyaleLootPosition()
    {
        a_Usages = new array<string>();
        a_Containers = new array<ref BattleRoyaleLootContainer>();
    }

    ref array<string> GetUsages()
    {
        return a_Usages;
    }
    ref array<ref BattleRoyaleLootContainer> GetContainers()
    {
        return a_Containers;
    }

    void AddUsage(string usage)
    {
        a_Usages.Insert(usage);
    }
    void AddContainer(ref BattleRoyaleLootContainer container)
    {
        a_Containers.Insert(container);
    }
}
class BattleRoyaleLootContainer
{
    protected ref array<string> a_Categories;
    protected ref array<string> a_Tags;
    protected ref array<ref BattleRoyaleLootPoint> a_Points;

    void BattleRoyaleLootContainer()
    {
        a_Categories = new array<string>();
        a_Tags = new array<string>();
        a_Points = new array<ref BattleRoyaleLootPoint>();
    }
    ref array<string> GetCategories()
    {
        return a_Categories;
    }
    ref array<string> GetTags()
    {
        return a_Tags;
    }
    ref array<ref BattleRoyaleLootPoint> GetPoints()
    {
        return a_Points;
    }
    void AddCategory(string cat)
    {
        a_Categories.Insert(cat);
    }
    void AddTag(string tag)
    {
        a_Tags.Insert(tag);
    }
    void AddPoint(vector pos, float range, float height, int flags = 32)
    {
        ref BattleRoyaleLootPoint point = new BattleRoyaleLootPoint( pos, range, height, flags );
        a_Points.Insert(point);
    }
}
class BattleRoyaleLootPoint
{
    protected vector v_Position;
    protected float f_Range;
    protected float f_Height;
    protected int i_Flags;

    void BattleRoyaleLootPoint(vector pos, float range, float height, int flags)
    {
        v_Position = pos;
        f_Range = range;
        f_Height = height;
        i_Flags = flags;
    }

    vector GetPosition()
    {
        return v_Position;
    }
    float GetRange()
    {
        return f_Range;
    }
    float GetHeight()
    {
        return f_Height;
    }
    int GetFlags()
    {
        return i_Flags;
    }
}