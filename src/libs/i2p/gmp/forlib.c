#include <stdio.h>
#include <gmp.h>

void nativeModPowFunctions()
{
	double r ;
	int i;
	mpz_t mbase;
	mpz_t mexp;
	mpz_t mmod;
	
	mpz_init2 ( mbase, 123 ); //preallocate the size
	mpz_import ( mbase, 1, 1, 2, 1, 0, NULL );
	i = mpz_sizeinbase ( mbase, 2 );
	mpz_export ( ( void* ) NULL, NULL, 1, 1, 1, 0, mbase );
	mpz_powm ( mmod, mbase, mexp, mmod );
	r = mpz_get_d ( mbase );
	
	mpz_clear ( mbase );
}

