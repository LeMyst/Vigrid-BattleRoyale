class SkinMap
{
    protected string s_DisplayName; 

    protected string s_ItemConfigName;
    protected typename t_ItemBaseClass;
    protected string s_ShopFlag;


    void SkinMap(string name, string item_config_name, typename item_base_class, string flag = "")
    {
        s_DisplayName = name;
        s_ItemConfigName = item_config_name;
        t_ItemBaseClass = item_base_class;
        s_ShopFlag = flag;
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
        return s_ItemConfigName;
    }
    typename GetClass()
    {
        return t_ItemBaseClass;
    }
    bool IsClothing()
    {
        return GetClass().IsInherited( Clothing );
    }
/*
    void ApplyTo(EntityAI item)
    {
        int i;
        ref array<string> values = GetTextures();
        for(i = 0; i < values.Count(); i++)
        {
            item.SetObjectTexture(i, values.Get(i));
        }
        values = GetMaterials();
        for(i = 0; i < values.Count(); i++)
        {
            item.SetObjectMaterial(i, values.Get(i));
        }
    }
*/
    ref array<string> GetTextures()
    {
        ref array<string> result = new array<string>;
        GetGame().ConfigGetTextArray("CfgVehicles " + s_ItemConfigName + " hiddenSelectionsTextures",result);
        return result;
    }
    ref array<string> GetMaterials()
    {
        ref array<string> result = new array<string>;
        GetGame().ConfigGetTextArray("CfgVehicles " + s_ItemConfigName + " hiddenSelectionsMaterials",result);
        return result;
    }
}