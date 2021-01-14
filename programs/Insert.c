main() 
{ 
	int a[4]={-14,6,28};
	int number,temp1,temp2,end,i,j; 
	printf("original array is:\n"); 
	for(i=0;i<3;i++) 
		printf("%5d",a[i]); 
	printf("\n"); 
	printf("insert a new number:"); 
	scanf("%d",&number); 
	end=a[2]; 
	if(number>=end) 
		a[3]=number; 
	else 
	{
		for(i=0;i<3;i++) 
		{ 
			if(a[i]>number) 
			{
				temp1=a[i]; 
				a[i]=number; 
				for(j=i+1;j<4;j++)
				{
					temp2=a[j]; 
					a[j]=temp1; 
					temp1=temp2;
				} 
				break;
			}
		} 
	} 
	for(i=0;i<4;i++) 
		printf("%6d",a[i]); 
	printf("\n");
}