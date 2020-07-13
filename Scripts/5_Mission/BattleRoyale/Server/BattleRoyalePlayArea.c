class BattleRoyalePlayArea
{
    float f_Radius;
    vector v_Center;
    
    void SetRadius(float rad)
    {
        f_Radius = rad;
    }
    float GetRadius()
    {
        return f_Radius;
    }
    void SetCenter(vector center)
    {
        v_Center = center;
    }
    float GetCenter()
    {
        return v_Center;
    }
}