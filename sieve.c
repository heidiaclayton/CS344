#include <stdio.h>
#include <math.h>
#include <stdlib.h>


int* sieve(int n, int *size) {
	int *original, i, j, counter = 0;

	original = malloc(sizeof(int) * n);

	/*fill array*/	
	for (i = 1; i <= n; i++) {
		original[i - 1] = i;
	}

	/*mark composites and 1 with 0*/
	
	original[0] = 0;

	for (i = 2; i <= sqrt(n); i++) {
		for (j = i; j < n; j++) {
			if (original[j] != 0 && original[j] % i == 0) {
				original[j] = 0;
				counter++;
			}
		}
	}

	*size = n - counter - 1;

	return original;
}


int main() {
	int num, *primes, num_of_primes = 0, i;

	printf("What number do you want to primes for?: ");
	scanf("%d", &num);

	primes = sieve(num, &num_of_primes);


	printf("The number of primes before %d is: %d\n", num, num_of_primes);
	
	printf("The primes are: \n");
	
	for (i = 0; i < num; i++) {
		if (primes[i] != 0)
			printf("%d\n", primes[i]);
	}
	
	free(primes);

	return 0;
}
