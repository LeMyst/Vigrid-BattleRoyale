class SkinMap
{
    protected string s_DisplayName;
    

    protected string s_ItemClass;
    protected string s_ShopFlag;

    ref array<string> skin_list;

    void SkinMap()
    {
        skin_list = new array<string>();

        s_DisplayName = "UNKNOWN";
        s_ItemClass = "Zuccini";
        s_ShopFlag = "do_not_allow_to_be_visible"; //this prevents it from appearing in the shop window
    }
    void Init(string name, ref array<string> textures, string item, string flag)
    {
        skin_list.InsertAll( textures );
        s_DisplayName = name;
        s_ItemClass = item;
        s_ShopFlag = flag;
    }
    string GetFlag()
    {
        return s_ShopFlag;
    }

    string GetName()
    {
        return s_DisplayName;
    }
    string GetClassName()
    {
        return s_ItemClass;
    }
    string GetTexture(int index = 0)
    {
        if(skin_list.Count() <= index)
        {
            Print(index);
            Print(skin_list);
            Error("Not enough skins for index");
            return "";
        }
        return skin_list[index];
    }
}


class DayZBRTSkinMap extends SkinMap
{
    void InitTee(string display_name, string ground_texture, string shirt_texture, string shop_item)
    {
        skin_list.Insert( BATTLEROYALE_TSHIRT_SKINS_PATH + ground_texture ); //ground
        skin_list.Insert( BATTLEROYALE_TSHIRT_SKINS_PATH + shirt_texture ); //male & female
        s_ItemClass = "TShirt_White";
        s_ShopFlag = shop_item;
        s_DisplayName = display_name;
    }
}
class DayZTSkinMap extends SkinMap
{
    void InitTee(string display_name, string ground_texture, string shirt_texture)
    {
        skin_list.Insert( ground_texture ); //ground
        skin_list.Insert( shirt_texture ); //male & female
        s_ItemClass = "TShirt_White";
        s_ShopFlag = ""; //blank allows anyone to use it
        s_DisplayName = display_name;
    }
}