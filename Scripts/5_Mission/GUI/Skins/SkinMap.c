class SkinMap
{
    protected string s_DisplayName;
    
    protected string s_PreviewItemClassName;
    protected typename t_ItemBaseClass;
    protected string s_ShopFlag;

    ref array<string> skin_list;

    void SkinMap()
    {
        skin_list = new array<string>();

        s_DisplayName = "UNKNOWN";
        t_ItemBaseClass = Zucchini;
        s_PreviewItemClassName = "Zucchini";
        s_ShopFlag = "do_not_allow_to_be_visible"; //this prevents it from appearing in the shop window
    }
    void Init(string name, ref array<string> textures, string item_preview_classname, typename item_base_class, string flag)
    {
        skin_list.InsertAll( textures );
        s_DisplayName = name;
        t_ItemBaseClass = item_base_class;
        s_ShopFlag = flag;
        s_PreviewItemClassName = item_preview_classname;
    }
    string GetFlag()
    {
        return s_ShopFlag;
    }
    bool IsValidSkinForEntity(EntityAI item)
    {
        return item.IsInherited( GetClass() );
    }
    string GetName()
    {
        return s_DisplayName;
    }
    string GetClassName()
    {
        return s_PreviewItemClassName;
    }
    typename GetClass()
    {
        return t_ItemBaseClass;
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
        t_ItemBaseClass = TShirt_ColorBase;
        s_PreviewItemClassName = "TShirt_White";
        s_ShopFlag = shop_item;
        s_DisplayName = display_name;
    }
}
class DayZTSkinMap extends SkinMap
{
    void InitTee(string display_name, string ground_texture, string shirt_texture)
    {
        skin_list.Insert( DAYZ_TSHIRT_SKINS_PATH + ground_texture ); //ground
        skin_list.Insert( DAYZ_TSHIRT_SKINS_PATH + shirt_texture ); //male & female
        t_ItemBaseClass = TShirt_ColorBase;
        s_PreviewItemClassName = "TShirt_White";
        s_ShopFlag = ""; //blank allows anyone to use it
        s_DisplayName = display_name;
    }
}

class GunSkinMap extends SkinMap
{
    string s_BodyTexture;
    void GunSkinMap()
    {
        s_BodyTexture = "";
    }
    override string GetTexture(int index = 0)
    {
        return s_BodyTexture;
    }
}
class DayZBRGunSkinMap extends GunSkinMap
{
    void InitGun(string display_name, string body_texture, string preview_class, typename gun_class, string shop_item = "")
    {
        s_BodyTexture = BATTLEROYALE_WEAPON_SKINS_PATH + body_texture; //weapon texture
        t_ItemBaseClass = gun_class;
        s_PreviewItemClassName = preview_class;
        s_ShopFlag = shop_item;
        s_DisplayName = display_name;
    }
}