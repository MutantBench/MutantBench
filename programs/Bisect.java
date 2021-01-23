// This is a mutant program.
// Author : ysma

public class Bisect
{

    double mEpsilon;

    double mNumber;

    double mResult;

    public Bisect()
    {
    }

    public  void setEpsilon( double epsilon )
    {
        this.mEpsilon = epsilon;
    }

    public  double sqrt( double N )
    {
        double x = N;
        double M = N;
        double m = 1;
        double r = x;
        double diff = x * x - N;
        while (Math.abs(diff) > mEpsilon) {
            if (diff < 0) {
                m = x;
                x = (M + x) / 2;
            } else {
                if (diff > 0) {
                    M = x;
                    x = (m + x) / 2;
                }
            }
            diff = x * x - N;
        }
        r = x;
        mResult = r;
        return r;
    }

}
