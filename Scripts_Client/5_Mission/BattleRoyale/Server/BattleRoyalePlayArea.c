class BattleRoyalePlayArea
{
    float f_Radius;
    vector v_Center;

    void BattleRoyalePlayArea(vector center, float radius)
    {
        SetCenter(center);
        SetRadius(radius);
    }

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

    vector GetCenter()
    {
        return v_Center;
    }

    bool IsAreaOverlap(BattleRoyalePlayArea other)
    {
        float x1 = this.GetCenter()[0];
        float z1 = this.GetCenter()[2];
        float r1 = this.GetRadius();

        float x2 = other.GetCenter()[0];
        float z2 = other.GetCenter()[2];
        float r2 = other.GetRadius();

        // Calculate the distance between the centers of the two circles
        int d = Math.Sqrt(Math.Pow((x2 - x1), 2) + Math.Pow((z2 - z1), 2));

        // Check if one circle is completely inside the other
        if(((d + r2) <= r1) || ((d + r1) <= r2))
            return true;

        // Check if the circles intersect or partially overlap
        if(d < (r1 + r2))
            return true;

        // If none of the above conditions are true, the circles do not overlap
        return false;
    }
}