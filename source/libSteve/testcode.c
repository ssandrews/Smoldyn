/* Steve Andrews */

#include <stdio.h>


// Test of Geo_SphereReflectSphere function
// gcc -Wall -O0 -g testcode.c Geometry.c math2.c -o testcode
#include "Geometry.h"

int main() {
	double a0[3],a1[3],b0[3],b1[3],a1p[3],b1p[3],ap[3],bp[3],radius,radius2,p;

	while(1) {
		printf("Enter 2D values for a0, a1, b0, b1: ");
		scanf("%lf %lf %lf %lf %lf %lf %lf %lf",&a0[0],&a0[1],&a1[0],&a1[1],&b0[0],&b0[1],&b1[0],&b1[1]);
		printf("Enter R: ");
		scanf("%lf",&radius);
		radius2=radius*radius;
		p=Geo_SphereReflectSphere(a0,a1,b0,b1,2,radius2,a1p,b1p);
		printf("p: %g\n",p);
		ap[0]=(1-p)*a0[0]+p*a1[0];
		ap[1]=(1-p)*a0[1]+p*a1[1];
		bp[0]=(1-p)*b0[0]+p*b1[0];
		bp[1]=(1-p)*b0[1]+p*b1[1];
		printf("A: (%g,%g) -> (%g,%g) -> (%g,%g)\n",a0[0],a0[1],ap[0],ap[1],a1p[0],a1p[1]);
		printf("B: (%g,%g) -> (%g,%g) -> (%g,%g)\n",b0[0],b0[1],bp[0],bp[1],b1p[0],b1p[1]);
		printf("\n"); }

	return 0; }


// Test of partial transmission values
// gcc -Wall -O0 -g testcode.c SurfaceParam.c random2.c math2.c SFMT/SFMT.c -o testcode
/*#include "SurfaceParam.h"
#include "random2.h"

int main() {
	double difc1,difc2,dt;
	double kap1,kap2,p1,p2;
	int er;

	randomize(-1);
	printf("Testing surfacetransmit function.\n");
	printf("Press control-c to quit.\n\n");
	printf("Enter difc1, difc2, dt: ");
	scanf("%lf %lf %lf",&difc1,&difc2,&dt);
	while(1) {
		printf("Enter kap1, kap2, p1, p2 (unknowns=-2): ");
		scanf("%lf %lf %lf %lf",&kap1,&kap2,&p1,&p2);
		er=surfacetransmit(&kap1,&kap2,&p1,&p2,difc1,difc2,dt);
		if(er) printf("** Error.");
		printf(" kap1=%g, kap2=%g, p1=%g, p2=%g\n",kap1,kap2,p1,p2);
		p1=p2=-2;
		er=surfacetransmit(&kap1,&kap2,&p1,&p2,difc1,difc2,dt);
		if(er) printf("** Error.\n");
		printf(" check: p1=%g, p2=%g\n",p1,p2);
		printf("\n"); }
	return 0; }
*/

// Test of string logic expansion
// gcc -Wall -O0 -g testcode.c string2.c random2.c math2.c SFMT/SFMT.c -o testcode
/*
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
*/

	
