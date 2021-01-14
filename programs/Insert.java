import java.util.*;

public class Insert {

    public static void main(int number) {
        int[] a = new int[] { -14, 6, 28, 0 };
        int mytemp1, mytemp2, end, i, j;
        System.out.println("original array is:\n");
        for (i = 0; i < 3; i++) {
            System.out.printf("%5d", a[i]);
        }
        System.out.printf("\n");
        System.out.printf("insert a new number:");
        end = a[2];
        if (number >= end) {
            a[3] = number;
        } else {
            for (i = 0; i < 3; i++) {
                if (a[i] > number) {
                    mytemp1 = a[i];
                    a[i] = number;
                    for (j = i + 1; j < 4; j++) {
                        mytemp2 = a[j];
                        a[j] = mytemp1;
                        mytemp1 = mytemp2;
                    }
                    break;
                }
            }
        }
        for (i = 0; i < 4; i++) {
            System.out.printf("%6d", a[i]);
        }
        System.out.printf("\n");
    }
}

