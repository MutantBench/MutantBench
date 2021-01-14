// This is a mutant program.
// Author : ysma

public class QuickSort
{

    private  void quicksort( int[] data, int first, int last )
    {
        int lower = first + 1;
        int upper = last;
        swap( data, first, (first + last) / 2 );
        int bound = data[first];
        while (lower <= upper) {
            while (bound > data[lower]) {
                lower++;
            }
            while (bound < data[upper]) {
                upper--;
            }
            if (lower < upper) {
                swap( data, lower++, upper-- );
            } else {
                lower++;
            }
        }
        swap( data, upper, first );
        if (first < upper - 1) {
            quicksort( data, first, upper - 1 );
        }
        if (upper + 1 < last) {
            quicksort( data, upper + 1, last );
        }
    }

    public  void sort( int[] data )
    {
        if (data.length < 2) {
            return;
        }
        int max = 0;
        for (int i = 1; i < data.length; i++) {
            if (data[max] < data[i]) {
                max = i;
            }
        }
        swap( data, data.length - 1, max );
        quicksort( data, 0, data.length - 2 );
    }

    public  void swap( int[] data, int i, int j )
    {
        int tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
    }

}
