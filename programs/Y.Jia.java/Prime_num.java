public class Prime_num {
	public static void main(String args[])
{
	int m,i,k,h=0,leap=1;
	System.out.print("\n");
	for(m=1;m<=5;m++)
	{
		k=(int)Math.sqrt(m+1);
		for(i=2;i<=k;i++)
		{
			if(m%i==0)
			{
				leap=0;
				break;
			}
		}
		if(leap!=0)
		{
			System.out.printf("%-4d",m);
			h++;
			if(h%10==0)
				System.out.printf("\n");
		}
		leap=1;
	}
	System.out.printf("\nThe total is %d",h);
}
}
