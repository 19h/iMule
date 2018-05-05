/**
 *
 *                   wxStringWriter CLASS
 *
 */

import java.io.IOException ;
import java.io.Writer  ;
import gnu.gcj.*       ;

class WxStringWriter extends Writer
{
	private RawData _wxstring ;
	public WxStringWriter(RawData s)
	{
		_wxstring = s ;
	}
	
	private native void wxWrite(  String s );
	
	public void write(char[] cbuf, int off, int len)
			throws IOException
	{
		String s = new String(cbuf, off, len);
		wxWrite(s);
		
	}
	
	public void flush()
			throws IOException {}

	public void close()
			throws IOException {}
}
