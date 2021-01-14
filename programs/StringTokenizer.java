// This is a mutant program.
// Author : ysma




import java.lang.*;


public class StringTokenizer implements java.util.Enumeration<Object>
{

    private int currentPosition;

    private int newPosition;

    private int maxPosition;

    private java.lang.String str;

    private java.lang.String delimiters;

    private boolean retDelims;

    private boolean delimsChanged;

    private int maxDelimCodePoint;

    private boolean hasSurrogates = false;

    private int[] delimiterCodePoints;

    private  void setMaxDelimCodePoint()
    {
        if (delimiters == null) {
            maxDelimCodePoint = 0;
            return;
        }
        int m = 0;
        int c;
        int count = 0;
        for (int i = 0; i < delimiters.length(); i += Character.charCount( c )) {
            c = delimiters.charAt( i );
            if (c >= Character.MIN_HIGH_SURROGATE && c <= Character.MAX_LOW_SURROGATE) {
                c = delimiters.codePointAt( i );
                hasSurrogates = true;
            }
            if (m < c) {
                m = c;
            }
            count++;
        }
        maxDelimCodePoint = m;
        if (hasSurrogates) {
            delimiterCodePoints = new int[count];
            for (int i = 0, j = 0; i < count; i++, j += Character.charCount( c )) {
                c = delimiters.codePointAt( j );
                delimiterCodePoints[i] = c;
            }
        }
    }

    public StringTokenizer( java.lang.String str, java.lang.String delim, boolean returnDelims )
    {
        currentPosition = 0;
        newPosition = -1;
        delimsChanged = false;
        this.str = str;
        maxPosition = str.length();
        delimiters = delim;
        retDelims = returnDelims;
        setMaxDelimCodePoint();
    }

    public StringTokenizer( java.lang.String str, java.lang.String delim )
    {
        this( str, delim, false );
    }

    public StringTokenizer( java.lang.String str )
    {
        this( str, " \t\n\r\f", false );
    }

    private  int skipDelimiters( int startPos )
    {
        if (delimiters == null) {
            throw new java.lang.NullPointerException();
        }
        int position = startPos;
        while (!retDelims && position < maxPosition) {
            if (!hasSurrogates) {
                char c = str.charAt( position );
                if (c > maxDelimCodePoint || delimiters.indexOf( c ) < 0) {
                    break;
                }
                position++;
            } else {
                int c = str.codePointAt( position );
                if (c > maxDelimCodePoint || !isDelimiter( c )) {
                    break;
                }
                position += Character.charCount( c );
            }
        }
        return position;
    }

    private  int scanToken( int startPos )
    {
        int position = startPos;
        while (position < maxPosition) {
            if (!hasSurrogates) {
                char c = str.charAt( position );
                if (c <= maxDelimCodePoint && delimiters.indexOf( c ) >= 0) {
                    break;
                }
                position++;
            } else {
                int c = str.codePointAt( position );
                if (c <= maxDelimCodePoint && isDelimiter( c )) {
                    break;
                }
                position += Character.charCount( c );
            }
        }
        if (retDelims && startPos == position) {
            if (!hasSurrogates) {
                char c = str.charAt( position );
                if (c <= maxDelimCodePoint && delimiters.indexOf( c ) >= 0) {
                    position++;
                }
            } else {
                int c = str.codePointAt( position );
                if (c <= maxDelimCodePoint && isDelimiter( c )) {
                    position += Character.charCount( c );
                }
            }
        }
        return position;
    }

    private  boolean isDelimiter( int codePoint )
    {
        for (int i = 0; i < delimiterCodePoints.length; i++) {
            if (delimiterCodePoints[i] == codePoint) {
                return true;
            }
        }
        return false;
    }

    public  boolean hasMoreTokens()
    {
        newPosition = skipDelimiters( currentPosition );
        return newPosition < maxPosition;
    }

    public  java.lang.String nextToken()
    {
        currentPosition = newPosition >= 0 && !delimsChanged ? newPosition : skipDelimiters( currentPosition );
        delimsChanged = false;
        newPosition = -1;
        if (currentPosition >= maxPosition) {
            throw new java.util.NoSuchElementException();
        }
        int start = currentPosition;
        currentPosition = scanToken( currentPosition );
        return str.substring( start, currentPosition );
    }

    public  java.lang.String nextToken( java.lang.String delim )
    {
        delimiters = delim;
        delimsChanged = true;
        setMaxDelimCodePoint();
        return nextToken();
    }

    public  boolean hasMoreElements()
    {
        return hasMoreTokens();
    }

    public  java.lang.Object nextElement()
    {
        return nextToken();
    }

    public  int countTokens()
    {
        int count = 0;
        int currpos = currentPosition;
        while (currpos < maxPosition) {
            currpos = skipDelimiters( currpos );
            if (currpos >= maxPosition) {
                break;
            }
            currpos = scanToken( currpos );
            count++;
        }
        return count;
    }

}
