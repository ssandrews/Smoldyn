/* Steve Andrews */

/**** Compiling and linking ****

Compile and link:

gcc -Wall -O0 -g testcode.c string2.c -o testcode
 
*/


#include <stdio.h>
#include <string.h>
#include "string2.h"

int main() {
	char pat[STRCHAR],str[STRCHAR],dest[STRCHAR],dest2[STRCHAR],temp[STRCHAR];
	int starextra,ismatch;

	printf("Testing logic expansion\n");
	while(1) {
		printf("Enter pattern: ");
		fgets(pat,STRCHAR,stdin);
		pat[strlen(pat)-1]='\0';
		printf("Enter string: ");
		fgets(str,STRCHAR,stdin);
		str[strlen(str)-1]='\0';
		printf("Enter destination: ");
		fgets(dest,STRCHAR,stdin);
		dest[strlen(dest)-1]='\0';
		printf("Enter extra star characters: ");
		scanf("%i",&starextra);
		fgets(temp,STRCHAR,stdin);

		printf("pat: '%s'\n",pat);
		printf("str: '%s'\n",str);
		printf("dest: '%s'\n",dest);

		printf("strwildcardmatch: %i\n",strwildcardmatch(pat,str));
		strcpy(dest2,dest);
		printf("strwildcardmatchandsub: %i, %s\n",strwildcardmatchandsub(pat,str,dest2,starextra),dest2);
		printf("strEnhWildcardMatch: %i\n",strEnhWildcardMatch(pat,str));
		printf("strEnhWildcardMatchandSub:\n");
		while((ismatch=strEnhWildcardMatchAndSub(pat,str,dest,dest2))>0) {
			printf(" %i, %s\n",ismatch,dest2); }
		if(ismatch<0) printf(" %i, %s\n",ismatch,dest2);
		printf("\n"); }


	return 0; }


	
