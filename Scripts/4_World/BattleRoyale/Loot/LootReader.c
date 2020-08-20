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
    protected ref array<ref LootGroup> a_Groups;
    protected ref LootDefaults p_Defaults;

    //--- optimized collections
    protected ref map<string, ref array<ref LootGroup>> m_WithName;
    protected ref map<string, ref array<ref LootGroup>> m_WithTag;
    protected ref map<string, ref array<ref LootGroup>> m_WithCategory;
    protected ref map<string, ref array<ref LootGroup>> m_WithUsage;
    protected ref map<int, ref array<ref LootGroup>> m_WithLootmax;
    
    
    ref LootGroup GetGroup(int idx)
    {
        return a_Groups.Get(idx);
    }
    ref array<ref LootGroup> GetGroupsWithName(string name)
    {
        return m_WithName.Get(name);
    }
    ref array<ref LootGroup> GetGroupsWithTag(string tag)
    {
        return m_WithTag.Get(tag);
    }
    ref array<ref LootGroup> GetGroupsWithCategory(string category)
    {
        return m_WithCategory.Get(category);
    }
    ref array<ref LootGroup> GetGroupsWithUsage(string usage)
    {
        return m_WithUsage.Get(usage);
    }
    ref array<ref LootGroup> GetGroupsWithLootmax(int lootmax)
    {
        return m_WithLootmax.Get(lootmax);
    }

    array<vector> GetAllLootPositions(string group_name)
    {
        array<vector> positions = new array<vector>();
        ref array<ref LootGroup> matching_groups = GetGroupsWithName(name);

        for(int i = 0; i < matching_groups.Count(); i++)
        {
            ref LootGroup group = matching_groups.Get(i);
            ref array<ref LootContainer> containers = group.GetContainers();
            
            for(int j = 0; j < containers.Count(); j++)
            {
                ref LootContainer container = containers.Get(j);
                ref array<ref LootPoint> points = container.GetPoints();

                for(int k = 0; k < points.Count(); k++)
                {
                    ref LootPoint point = points.Get(k);
                    vector pos = point.GetPos();

                    positions.Insert(pos);

                }

            }
        }
        return positions;
    }

    void LootReader()
    {
        a_Groups = new array<ref LootGroup>();
    }
    void ~LootReader()
    {

    }

    //$CurrentDir:\\mpmissions\\BattleRoyale.ChernarusPlusGloom for example
    void ParseLoot(string mission_path)
    {
        //TODO: read from disk and call ParseXML

        FileHandle file;
        string line;

        TStringArray contents = new TStringArray();

        file = OpenFile(path, FileMode.READ);
        while (FGets(file, line) > 0)
        {
            line.Trim();
            if (line != string.Empty)
            {
                contents.Insert(line);
                line = string.Empty;
            }
        }

        CloseFile(file);

        string all_lines = "";
        for(int i = 0; i < contents.Count(); i++)
        {
            all_lines += contents.Get(i) + "\n";

        }
        
        if(!ParseXML(all_lines))
        {
            Error("FAILED TO PARSE LOOT!");
        }
    }
    bool ParseXML(string all_lines)
    {
        //Example input:  all lines read from Mission/mapgroupproto.xml
        int index;
        int end_index;

        //1. preprocess out comments & extra tags
        string new_lines = all_lines.Replace("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>","").Trim();
        do {
            index = new_lines.IndexOf("<!--");
            if(index == -1)
                break;

            end_index = new_lines.IndexOf("-->");
            if(end_index == -1)
                break;

            if(index != 0)
            {
                new_lines = new_lines.Substring(0, index) + new_lines.Substring(end_index + 3, new_lines.Length() - (end_index + 3));
            }
            else
            {
                new_lines = new_lines.Substring(end_index + 3, new_lines.Length() - (end_index + 3));
            }
        } while(true);
        new_lines.TrimInPlace();
        

        //2. preprocess all groups into seperate strings starting with <group> and ending with </group>
        int defaults_start = new_lines.IndexOf("<defaults>");
        int defaults_end = new_lines.IndexOf("</defaults>");
        string defaults_content = new_lines.Substring(defaults_start, (defaults_end - defaults_start) + 11).Trim();
        
        p_Defaults = new LootDefaults();
        if(!p_Defaults.ParseXML(defaults_content))
        {
            Error("[LootReader] Failed to parse defaults. Some entries may be missing!");
        }

        //3. pass group data into LootGroup::ParseXML
        bool result = true;
        index = defaults_end;
        do {
            index = new_lines.IndexOfFrom(index, "<group");
            index_end = new_lines.IndexOfFrom(index, "</group>");
            if(index == -1 || index_end == -1)
            {
                break;
            }

            string group_content = new_lines.Substring(index, (index_end - index) + 8).Trim();
            ref LootGroup p_Group = new LootGroup();
            bool success = p_Group.ParseXML(group_content);
            result = result && success;
            if(success)
            {
                AddGroup(p_Group);
            }
            else
            {
                Error("[LootReader] Failed to parse group!");
            }
            

        } while(true);

        //TODO: sort groups into efficient values
    }
    void AddGroup(ref p_Group)
    {
        a_Groups.Insert(p_Group);

        string name = p_Group.GetName();
        name.ToLower();
        int Lootmax = p_Group.GetLootmax(); 

        if(Lootmax == -1)
        {
            Lootmax = p_Defaults.GetLootmax("group");
        }

        //create keys if not exist
        if(!m_WithName.Contains(name))
        {
            m_WithName.Insert(name, new array<ref LootGroup>());
        }


        //TODO: make sure that this does handle references properly
        m_WithName.Get(name).Insert(p_Group);

    }
}
class LootDefaults
{
    ref array<ref LootDefault> a_Defaults;
    ref map<string, ref LootDefault> m_ByName;


    int GetLootmax(string property_name)
    {
        if(!m_ByName.Contains(property_name))
        {
            Error("[LootDefaults] Could not find default with name `" + property_name + "`");
            return -1;
        }
        return m_ByName.Get(property_name).GetValue("lootmax").ToInt();
    }


    void LootDefaults()
    {
        a_Defaults = new array<ref LootDefault>();
    }

    bool ParseXML(string defaults_lines)
    {
        defaults_lines.Replace("<defaults>","");
        defaults_lines.Replace("</defaults>","");
        defaults_lines.TrimInPlace();

        bool result = true;

        TStringArray defaults = new TStringArray;
        defaults_content.Split("\n", defaults);
        for(int i = 0; i < defaults.Count(); i++)
        {
            string default_entry = defaults.Trim();
            if(default_entry != "")
            {
                ref LootDefault default = new LootDefault();
                bool success = default.ParseXML(default_entry);
                result = result && success;
                if(success)
                {
                    a_Defaults.Insert(default);
                    m_ByName.Insert(default.GetName(), default); //used for quick name lookup
                }
                else
                {
                    Error("[LootDefaults] Failed to parse default entry `" + default_entry + "`");
                }
            }
        }

        return result;
    }
}
class LootDefault
{
    protected ref XmlTag tag_parser;

    string GetName()
    {
        if(!tag_parser)
        {
            Error("[LootDefault] Attempt to Get Name before Parsing!");
            return "<no:name>";
        }
        string range_value = tag_parser.GetProperty("name");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr;
    }
    string GetValue(string property)
    {
        if(!tag_parser)
        {
            Error("[LootDefault] Attempt to Get `" + property + "` before Parsing!");
            return "";
        }
        if(!tag_parser.HasProperty(property))
        {
            Error("[LootDefault] Property `" + property + "` does not exist!");
            return "";
        }
        string range_value = tag_parser.GetProperty(property);
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr;
    }

    bool ParseXML(string default_entry)
    {
        tag_parser = new XmlTag();
        return tag_parser.ParseTag(default_entry);
    }
}
class LootGroup
{
    protected ref XmlTag tag_parser;
    protected ref array<string> a_Usage;
    protected ref array<ref LootContainer> a_Containers;

    int GetLootmax()
    {
        if(!tag_parser)
        {
            Error("[LootGroup] Attempt to Get Lootmax before Parsing!");
            return -1;
        }
        if(!tag_parser.HasProperty("lootmax"))
        {
            return -1; //use default
        }
        string range_value = tag_parser.GetProperty("lootmax");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr.ToInt();
    }
    string GetName()
    {
        if(!tag_parser)
        {
            Error("[LootGroup] Attempt to Get Name before Parsing!");
            return "<no:name>";
        }
        string range_value = tag_parser.GetProperty("name");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr;
    }
    ref array<ref LootContainer> GetContainers()
    {
        return a_Containers;
    }
    ref array<string> GetUsages()
    {
        return a_Usage;
    }


    void LootGroup()
    {
        a_Usage = new array<string>();
        a_Containers = new array<ref LootContainer>;
    }

    bool ParseXML(string xml_lines)
    {
        bool result = true;

        string header = xml_lines.Substring(0, xml_lines.IndexOf("\n"));
        tag_parser = new XmlTag();
        if(!tag_parser.ParseTag(header))
        {
            Error("[LootGroup] Failed to parse tag header!");
            result = false;
        }

        //extract usage
        int start = xml_lines.IndexOf("\n");
        int end;
        do {
            start = xml_lines.IndexOfFrom(start, "<usage");
            end = xml_lines.IndexOfFrom(start, "/>");
            if(start == -1 || end == -1)
            {
                break;
            }
            string usage_content = xml_lines.Substring(start, (end - start) + 2).Trim();
            XmlTag temp_parser = new XmlTag();
            if(!temp_parser.ParseTag(usage_content))
            {
                Error("[LootGroup] Failed to parse usage tag `" + usage_content + "`");
                result = false;
            }
            else
            {
                string name = temp_parser.GetProperty("name");
                name.ToLower();
                if(!a_Usage.Contains(name))
                {
                    a_Usage.Insert(name);
                }
            }
        } while(true);

        start = xml_lines.IndexOf("\n");
        do {
            start = xml_lines.IndexOfFrom(start, "<container");
            end = xml_lines.IndexOfFrom(start, "</container>");
            if(start == -1 || end == -1)
            {
                break;
            }
            string container_content = xml_lines.Substring(start, (end - start) + 12).Trim();
            ref LootContainer p_Container = new LootContainer();
            bool success = p_Container.ParseXML(container_content);
            if(!success)
            {
                Error("[LootGroup] Failed to parse container tag.");
                result = false; 
            }
            else
            {
                a_Containers.Insert(p_Container);
            }
            
        } while(true);


        return result;

        /*
            Example XML lines:

            <group name="Land_Shed_W6">
                    <usage name="Industrial" />
                    <usage name="Farm" />
                    <container name="lootFloor" lootmax="6">
                            <category name="tools" />
                            <category name="containers" />
                            <tag name="ground" />
                            <tag name="floor" />
                            <point pos="0.919720 -1.320877 -1.997921" range="0.739502" height="1.841980" />
                            <point pos="0.813784 -1.320877 2.119166" range="0.741943" height="1.854857" />
                            <point pos="0.880842 -1.320877 0.076158" range="1.199951" height="1.998627" />
                            <point pos="-1.558618 -1.320877 1.429411" range="1.199951" height="2.001556" />
                            <point pos="-1.450122 -1.320877 -1.138185" range="1.199951" height="1.999008" />
                            <point pos="2.996397 -1.320877 1.551793" range="1.199951" height="1.999466" />
                            <point pos="2.996995 -1.320877 -1.401179" range="1.199951" height="1.989624" />
                    </container>
            </group>
        */
    }
}
class LootContainer
{
    protected ref XmlTag tag_parser;
    protected ref array<string> a_Categories;
    protected ref array<string> a_Tags;
    protected ref array<ref LootPoint> a_LootPoints;

    string GetName()
    {
        if(!tag_parser)
        {
            Error("[LootContainer] Attempt to Get Name before Parsing!");
            return "<no:name>";
        }
        string range_value = tag_parser.GetProperty("name");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr;
    }
    int GetLootmax()
    {
        if(!tag_parser)
        {
            Error("[LootContainer] Attempt to Get Lootmax before Parsing!");
            return -1;
        }
        if(!tag_parser.HasProperty("lootmax"))
        {
            return -1; //use default
        }
        string range_value = tag_parser.GetProperty("lootmax");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr.ToInt();
    }
    
    ref array<string> GetCategories()
    {
        return a_Categories;
    }
    ref array<string> GetTags()
    {
        return a_Tags;
    }
    ref array<ref LootPoint> GetPoints()
    {
        return a_LootPoints;
    }

    void LootContainer()
    {
        a_Categories = new array<string>();
        a_Tags = new array<string>();
        a_LootPoints = new array<ref LootPoint>();
    }   

    bool ParseXML(string xml_lines)
    {
        bool result = true;

        string header = xml_lines.Substring(0, xml_lines.IndexOf("\n"));
        tag_parser = new XmlTag();
        if(!tag_parser.ParseTag(header))
        {
            Error("[LootContainer] Failed to parse tag header!");
            result = false;
        }

        int start = xml_lines.IndexOf("\n");
        int end;
        string name;
        XmlTag temp_parser;

        //parse categories
        {
            start = xml_lines.IndexOfFrom(start, "<category");
            end = xml_lines.IndexOfFrom(start, "/>");
            if(start == -1 || end == -1)
            {
                break;
            }
            string category_content = xml_lines.Substring(start, (end - start) + 2).Trim();
            temp_parser = new XmlTag();
            if(!temp_parser.ParseTag(category_content))
            {
                Error("[LootContainer] Failed to parse categroy tag `" + category_content + "`");
                result = false;
            }
            else
            {
                name = temp_parser.GetProperty("name");
                name.ToLower();
                if(!a_Categories.Contains(name))
                {
                    a_Categories.Insert(name);
                }
            }
        } while(true);

        start = xml_lines.IndexOf("\n");

        //parse tags
        {
            start = xml_lines.IndexOfFrom(start, "<tag");
            end = xml_lines.IndexOfFrom(start, "/>");
            if(start == -1 || end == -1)
            {
                break;
            }
            string tag_content = xml_lines.Substring(start, (end - start) + 2).Trim();
            temp_parser = new XmlTag();
            if(!temp_parser.ParseTag(tag_content))
            {
                Error("[LootContainer] Failed to parse tag tag `" + tag_content + "`");
                result = false;
            }
            else
            {
                name = temp_parser.GetProperty("name");
                name.ToLower();
                if(!a_Tags.Contains(name))
                {
                    a_Tags.Insert(name);
                }
            }
        } while(true);

        //parse points
        start = xml_lines.IndexOf("\n");
        do {
            start = xml_lines.IndexOfFrom(start, "<container");
            end = xml_lines.IndexOfFrom(start, "</container>");
            if(start == -1 || end == -1)
            {
                break;
            }
            string point_content = xml_lines.Substring(start, (end - start) + 12).Trim();
            ref LootPoint p_LootPoint = new LootPoint();
            bool success = p_LootPoint.ParseXML(container_content);
            if(!success)
            {
                Error("[LootContainer] Failed to parse point tag.");
                result = false; 
            }
            else
            {
                a_LootPoints.Insert(p_LootPoint);
            }
            
        } while(true);

        return result;
        /*
            Example XML lines:

            <container name="lootFloor" lootmax="6">
                    <category name="tools" />
                    <category name="containers" />
                    <tag name="ground" />
                    <tag name="floor" />
                    <point pos="0.919720 -1.320877 -1.997921" range="0.739502" height="1.841980" />
                    <point pos="0.813784 -1.320877 2.119166" range="0.741943" height="1.854857" />
                    <point pos="0.880842 -1.320877 0.076158" range="1.199951" height="1.998627" />
                    <point pos="-1.558618 -1.320877 1.429411" range="1.199951" height="2.001556" />
                    <point pos="-1.450122 -1.320877 -1.138185" range="1.199951" height="1.999008" />
                    <point pos="2.996397 -1.320877 1.551793" range="1.199951" height="1.999466" />
                    <point pos="2.996995 -1.320877 -1.401179" range="1.199951" height="1.989624" />
            </container>

        */
    }

    
}
class LootPoint
{
    protected ref XmlTag tag_parser;

    vector GetPos()
    {
        if(!tag_parser)
        {
            Error("[LootPoint] Attempt to Get Position before Parsing!");
            return "0 0 0";
        }
        string range_value = tag_parser.GetProperty("pos");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr.ToVector();
    }
    float GetRange()
    {
        if(!tag_parser)
        {
            Error("[LootPoint] Attempt to Get Range before Parsing!");
            return 0;
        }
        string range_value = tag_parser.GetProperty("range");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr.ToFloat();
    }
    float GetHeight()
    {
        if(!tag_parser)
        {
            Error("[LootPoint] Attempt to Get Height before Parsing!");
            return 0;
        }
        string range_value = tag_parser.GetProperty("height");
        string substr = range_value.Substring(1, range_value.Length() - 2);
        return substr.ToFloat();
    }

    bool ParseXML(string xml_line)
    {
        /* 
            Example XML line:
            <point pos="0.919720 -1.320877 -1.997921" range="0.739502" height="1.841980" />
        */
        xml_line.ToLower();
        tag_parser = new XmlTag();
        tag_parser.ParseTag(xml_line);

        if(tag_parser.GetName() != "point")
        {
            Error("[LootPoint] Tag Name Invalid | Expected `point` | Got `" + tag_parser.s_TagName + "`");
            return false;
        }
        if(!tag_parser.HasProperty("pos"))
        {
            Error("[LootPoint] Tag missing property `pos`");
            return false;
        }
        if(!tag_parser.HasProperty("range"))
        {
            Error("[LootPoint] Tag missing property `range`");
            return false;
        }
        if(!tag_parser.HasProperty("range"))
        {
            Error("[LootPoint] Tag missing property `range`");
            return false;
        }

        return true;
    }
}


class XmlTag
{
    protected string s_TagName;
    protected string s_TagContent;
    protected ref map<string, string> m_Tags;
    protected ref array<string> a_Flags;

    string GetName()
    {
        return s_TagName;
    }
    string GetContent()
    {
        return s_TagContent;
    }
    string GetProperty(string property_name)
    {
        return m_Tags.Get(property_name);
    }
    bool HasProperty(string property_name)
    {
        return m_Tags.Contains(property_name);
    }
    bool HasFlag(string flag_name)
    {
        return a_Flags.Contains(flag_name);
    }

    void XmlTag()
    {
        m_Tags = new map<string, string>();
        a_Flags = new array<string>();
    }


    //TODO: catch errors in this method and return false
    bool ParseTag(string data)
    {
        string trimmed = data.Trim();

        s_TagContent = "";
        s_TagName = "";
        m_Tags.Clear();
        a_Flags.Clear();

        bool is_in_tag = false;
        bool is_in_body = false;
        bool is_end_tag = false;
        bool is_in_string = false;
        bool is_escaped = false;
        bool was_escaped = false;

        bool skip_char = false;
        
        string tag_buffer = "";
        bool is_tag_nammed = false;
        string tag_name = "";

        bool is_property_named = false;
        string property_name = "";
        string property_value = "";

        for(int i = 0; i < trimmed.Length(); i++)
        {
            string character = trimmed.Substring(i, 1);
            skip_char = false;
            was_escaped = is_escaped;
            
            //are we opening an xml tag?
            if(character == "<")
            {
                if(!is_in_string && !is_in_tag)
                {
                    is_in_tag = true;
                    is_in_body = false;
                    skip_char = true;
                    is_tag_nammed = false;
                }
            }
            //end tag character (expecting > afterwards)
            if(character == "/")
            {
                if(!is_in_string && is_in_tag)
                {
                    is_end_tag = true;
                    skip_char = true;
                }
            }
            if(character == ">")
            {
                if(!is_in_string && is_in_tag)
                {
                    is_in_tag = false;
                    if(!is_end_tag)
                    {
                        is_in_body = true;
                    }
                    skip_char = true;
                }
            }
            if(character == "\\")
            {
                if(is_in_string)
                {
                    is_escaped = true;
                }
            }
            if(character == "\"")
            {
                if(is_in_string)
                {
                    if(!was_escaped)
                    {
                        is_in_string = false;
                    }
                }
                else
                {
                    is_in_string = true;
                }
            }
            if(is_escaped && was_escaped)
            {
                is_escaped = false;
            }

            if(skip_char)
                continue;

            if(is_in_body)
            {
                //no processing in body
                s_TagContent += character;
            }
            else
            {
                if(is_in_tag)
                {
                    if(!is_tag_nammed)
                    {
                        //tag naming is going on
                        if(character.Trim() != "")
                        {
                            tag_name += character;
                        }
                        else
                        {
                            s_TagName = tag_name;
                            is_tag_nammed = true;
                        }
                    }
                    else
                    {
                        //tag is named, extract properties
                        if(character.Trim() != "")
                        {
                            if(!is_property_named)
                            {
                                if(character != "=")
                                {
                                    property_name += character;
                                }
                                else
                                {
                                    is_property_named = true;
                                }
                            }
                            else
                            {
                                property_value += character;
                            }
                        }
                        else
                        {
                            //white space! property break!
                            if(is_property_named && property_value != "")
                            {
                                //END OF PROPERTY!
                                m_Tags.Insert(property_name, property_value);
                                //reset property read values
                                is_property_named = false;
                                property_name = "";
                                property_value = "";
                            }
                            if(!is_property_named && property_name != "")
                            {
                                //Example: <tag_name prop1 prop2=2/> (prop1 would trigger this)

                                //END OF FLAG! (unvalued property)
                                a_Flags.Insert(property_name);

                                //reset property read values
                                is_property_named = false;
                                property_name = "";
                                property_value = "";
                            }
                        }
                        
                    }
                    
                }
            }
            
        }

        if(s_TagName == "")
        {
            s_TagName = tag_name; //whatever we had gets pushed into the value
        }

        return true;
    }
}