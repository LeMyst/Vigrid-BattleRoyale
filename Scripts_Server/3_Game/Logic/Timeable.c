class Timeable
{
    protected ref array<ref Timer> m_Timers;

    void Timeable()
    {
        m_Timers = new array<ref Timer>;
    }

    void ~Timeable()
    {
        StopTimers();

        delete m_Timers;
    }

    ref Timer AddTimer(float duration, Managed obj, string fn_name, Param params = NULL, bool loop = false)
    {
        ref Timer m_Timer = new Timer;
        m_Timer.Run( duration, obj, fn_name, params, loop );
        if(loop)
        {
            m_Timers.Insert( m_Timer );
        }
        return m_Timer;
    }

    bool RemoveTimer(ref Timer m_Timer)
    {
        int index = m_Timers.Find(m_Timer);
        if(index == -1)
            return false;

        m_Timers.Remove(index);
        return true;
    }

    void StopTimers()
    {
        for(int i = 0; i < m_Timers.Count(); i++)
        {
            if(m_Timers[i].IsRunning())
                m_Timers[i].Stop();
        }
    }
}
