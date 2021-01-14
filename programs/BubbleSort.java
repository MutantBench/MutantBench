// This is a mutant program.
// Author : ysma

public class BubbleSort
{

    public  void sort( int[] data )
    {
        for (int i = 0; i < data.length - 1; i++) {
            for (int j = data.length - 1; j > i; --j) {
                if (data[j] < data[j - 1]) {
                    int temp = data[j];
                    data[j] = data[j - 1];
                    data[j - 1] = temp;
                }
            }
        }
    }

}
