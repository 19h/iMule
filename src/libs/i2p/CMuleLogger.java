import gnu.gcj.RawData;

public class CMuleLogger
{
    int m_logId;
    RawData m_buffer;
    int m_size;
    static final int DUMPSIZE = 2000000;

    public CMuleLogger(int id) 
	{
		m_logId = id ;
		init() ;
	}
	
    private native void init() ;
    protected native void finalize() ;

    public native void log(String message);
    public native void logCritical(String message);
    public native void store(byte [] buffer, int start, int len);
    public native void dump();
}
