/* Steven Andrews, 6/08.
See documentation called parse_doc.doc
Copyright 2008 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __parse_h
#define __parse_h

typedef struct ParseFileStruct {
	char *froot;								// root of file name
	char *fname;								// complete file name, including root
	FILE *fptr;									// pointer to open file
	int lctr;										// line counter
	char *line;									// the line being parsed
	char *linecopy;							// copy of the line being parsed
	int incomment;							// flag for block comments
	struct ParseFileStruct *prevfile;	// previous file in linked list
	int maxdef;									// allocated size of define lists
	int ndef;										// current length of define lists
	char **defkey;							// search keys for defines
	char **defreplace;					// replacement text for defines
	int *defgbl;								// flag for if define is global
	int inifdef;								// counter for depth in ifdefines
	} *ParseFilePtr;

ParseFilePtr Parse_AllocFilePtr(const char *fileroot,const char *filename);
void Parse_FreeFilePtr(ParseFilePtr pfp);
int Parse_ExpandDefine(ParseFilePtr pfp,int maxdef);
int Parse_AddDefine(ParseFilePtr pfp,const char *key,const char *replace,int global);
int Parse_RemoveDefine(ParseFilePtr pfp,char *key);
void Parse_DisplayDefine(ParseFilePtr pfp);
int Parse_DoDefine(ParseFilePtr pfp);

int Parse_CmdLineArg(int *argcptr,char **argv,ParseFilePtr pfp);
ParseFilePtr Parse_Start(const char *fileroot,const char *filename,char *erstr);
int Parse_ReadLine(ParseFilePtr *pfpptr,char *word,char **line2ptr,char *erstr);
int Parse_ReadFailure(ParseFilePtr pfp,char *erstr);

#endif
