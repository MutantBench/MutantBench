#include <stdio.h>
#include <math.h>
int main(a, b, c)
unsigned int a;
unsigned int b;
unsigned int c;
{	
	int t;
	if(a>b)
	{
		t=a;
		a=b;
		b=t;
	}
	if(a>c)
	{
		t=a;
		a=b;
		c=t;
	}  
	if(b>c)
	{
		t=b;
		b=c;
		c=t;
	}
	if(a+b<=c)
	    return 0;
	else if(a==b&&b==c)
	    return 3;
	else if(a==b||b==c)
	    return 2;
	else
	    return 1;
}
