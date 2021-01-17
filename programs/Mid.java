public class Mid {
	public static int main( int a, int b, int c ) {
		int mid;
		if (a < b) {
			if (c < b) {
				if (a < c) {
					mid = c;
				} else {
					mid = a;
				}
			} else {
				mid = b;
			}
		} else {
			if (c > b) {
				if (a > c) {
					mid = c;
				} else {
					mid = a;
				}
			} else {
				mid = b;
			}
		}
		return mid;
	}
}
