import java.util.Scanner;
public class Bubble {
public static void main(int b, int c, int d, int e, int f)
{	int i,j,xyz,y;int[]a=new int[5];
	a[0] = b;
	a[1] = c;
	a[2] = d;
	a[3] = e;
	a[4] = f;
	for(i=0;i<5;i++)
	{
		for(j=i+1;j<5;j++)
		{
			if(a[i]<a[j])
			{
				xyz=a[i];
				a[i]=a[j];
				a[j]=xyz;
		} }
		System.out.printf("5d ",a[i]); }
}
}
