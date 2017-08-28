/*************************************************************************************************/
/**
	BASIC.cpp

	Contains routines for tokenising/detokenising BBC BASIC programs.

	Modified from code by Thomas Harte.

	Copyright (C) Thomas Harte

	This file is part of BeebAsm.

	BeebAsm is free software: you can redistribute it and/or modify it under the terms of the GNU
	General Public License as published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	BeebAsm is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
	even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with BeebAsm, as
	COPYING.txt.  If not, see <http://www.gnu.org/licenses/>.
*/
/*************************************************************************************************/

#include "BASIC.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*

	KeyWord table - used by both the exporter and the importer to convert
	between the plain text version of a BASIC program and the format used
	internally by the BASIC ROM

	Information taken from pages 41-43 of the BASIC ROM User Guide, 
	BASIC 2 ROM assumed throughout

*/

struct KeyWord
{
	KeyWord(const char* name, Uint8 flags)
		: Name(name), Flags(flags), StrLen(0), Next(NULL)
	{}

	const char *Name;
	Uint8 Flags;
	unsigned int StrLen;
	struct KeyWord *Next;
};

struct KeyWord KeyWordTable[0x80] =
{
	/* 0x80 */
	KeyWord("AND",	0x00),		KeyWord("DIV",	0x00),		KeyWord("EOR",	0x00),		KeyWord("MOD",	0x00),
	KeyWord("OR",	0x00),		KeyWord("ERROR",0x04),		KeyWord("LINE",	0x00),		KeyWord("OFF",	0x00),

	/* 0x88 */
	KeyWord("STEP",	0x00),		KeyWord("SPC",	0x00),		KeyWord("TAB(",	0x00),		KeyWord("ELSE",	0x14),
	KeyWord("THEN",	0x14),		KeyWord("",		0x00),		KeyWord("OPENIN",0x00),		KeyWord("PTR",	0x43),

	/* 0x90 */
	KeyWord("PAGE",	0x43),		KeyWord("TIME",	0x43),		KeyWord("LOMEM",0x43),		KeyWord("HIMEM",0x43),
	KeyWord("ABS",	0x00),		KeyWord("ACS",	0x00),		KeyWord("ADVAL",0x00),		KeyWord("ASC",	0x00),

	/* 0x98 */
	KeyWord("ASN",	0x00),		KeyWord("ATN",	0x00),		KeyWord("BGET",	0x01),		KeyWord("COS",	0x00),
	KeyWord("COUNT",0x01),		KeyWord("DEG",	0x00),		KeyWord("ERL",	0x01),		KeyWord("ERR",	0x01),

	/* 0xa0 */
	KeyWord("EVAL",	0x00),		KeyWord("EXP",	0x00),		KeyWord("EXT",	0x01),		KeyWord("FALSE",0x01),
	KeyWord("FN",	0x08),		KeyWord("GET",	0x00),		KeyWord("INKEY",0x00),		KeyWord("INSTR(",0x00),

	/* 0xa8 */
	KeyWord("INT",	0x00),		KeyWord("LEN",	0x00),		KeyWord("LN",	0x00),		KeyWord("LOG",	0x00),
	KeyWord("NOT",	0x00),		KeyWord("OPENUP",0x00),		KeyWord("OPENOUT",0x00),	KeyWord("PI",	0x01),

	/* 0xb0 */
	KeyWord("POINT(",0x00),		KeyWord("POS",	0x01),		KeyWord("RAD",	0x00),		KeyWord("RND",	0x01),
	KeyWord("SGN",	0x00),		KeyWord("SIN",	0x00),		KeyWord("SQR",	0x00),		KeyWord("TAN",	0x00),

	/* 0xb8 */
	KeyWord("TO",	0x00),		KeyWord("TRUE",	0x01),		KeyWord("USR",	0x00),		KeyWord("VAL",	0x00),
	KeyWord("VPOS",	0x01),		KeyWord("CHR$",	0x00),		KeyWord("GET$",	0x00),		KeyWord("INKEY$",0x00),

	/* 0xc0 */
	KeyWord("LEFT$(",0x00),		KeyWord("MID$(",0x00),		KeyWord("RIGHT$(",0x00),	KeyWord("STR$",	0x00),
	KeyWord("STRING$(",0x00),	KeyWord("EOF",	0x01),		KeyWord("AUTO",	0x10),		KeyWord("DELETE",0x10),

	/* 0xc8 */
	KeyWord("LOAD",	0x02),		KeyWord("LIST",	0x10),		KeyWord("NEW",	0x01),		KeyWord("OLD",	0x01),
	KeyWord("RENUMBER",0x10),	KeyWord("SAVE",	0x02),		KeyWord("",		0x00),		KeyWord("PTR",	0x00),

	/* 0xd0 */
	KeyWord("PAGE",	0x00),		KeyWord("TIME",	0x01),		KeyWord("LOMEM",0x00),		KeyWord("HIMEM",0x00),
	KeyWord("SOUND",0x02),		KeyWord("BPUT",	0x03),		KeyWord("CALL",	0x02),		KeyWord("CHAIN",0x02),

	/* 0xd8 */
	KeyWord("CLEAR",0x01),		KeyWord("CLOSE",0x03),		KeyWord("CLG",	0x01),		KeyWord("CLS",	0x01),
	KeyWord("DATA",	0x20),		KeyWord("DEF",	0x00),		KeyWord("DIM",	0x02),		KeyWord("DRAW",	0x02),

	/* 0xe0 */
	KeyWord("END",	0x01),		KeyWord("ENDPROC",0x01),	KeyWord("ENVELOPE",0x02),	KeyWord("FOR",	0x02),
	KeyWord("GOSUB",0x12),		KeyWord("GOTO",	0x12),		KeyWord("GCOL",	0x02),		KeyWord("IF",	0x02),

	/* 0xe8 */
	KeyWord("INPUT",0x02),		KeyWord("LET",	0x04),		KeyWord("LOCAL",0x02),		KeyWord("MODE",	0x02),
	KeyWord("MOVE",	0x02),		KeyWord("NEXT",	0x02),		KeyWord("ON",	0x02),		KeyWord("VDU",	0x02),

	/* 0xf0 */
	KeyWord("PLOT",	0x02),		KeyWord("PRINT",0x02),		KeyWord("PROC",	0x0a),		KeyWord("READ",	0x02),
	KeyWord("REM",	0x20),		KeyWord("REPEAT",0x00),		KeyWord("REPORT",0x01),		KeyWord("RESTORE",0x12),

	/* 0xf8 */
	KeyWord("RETURN",0x01),		KeyWord("RUN",	0x01),		KeyWord("STOP",	0x01),		KeyWord("COLOUR",0x02),
	KeyWord("TRACE",0x12),		KeyWord("UNTIL",0x02),		KeyWord("WIDTH",0x02),		KeyWord("OSCLI",0x02)
};

KeyWord *QuickTable[26*26];

/*

	Setup function, to establish contents of QuickTable, store strlens, etc

*/

struct Abbreviation
{
	Abbreviation(const char* name, Uint8 token, int minabbrlen)
		: Name(name), Token(token), MinAbbrLen(minabbrlen)
	{}

	const char *Name;
	Uint8 Token;
	int MinAbbrLen;
};

struct Abbreviation AbbreviationTable[] =
{
	Abbreviation("ABS", 0x94, 3),
	Abbreviation("ACS", 0x95, 3),
	Abbreviation("ADVAL", 0x96, 2),
	Abbreviation("AND", 0x80, 1),
	Abbreviation("ASC", 0x97, 3),
	Abbreviation("ASN", 0x98, 3),
	Abbreviation("ATN", 0x99, 3),

	Abbreviation("BGET", 0x9a, 1),
	Abbreviation("BPUT", 0xd5, 2),

	Abbreviation("CALL", 0xd6, 2),
	Abbreviation("CHAIN", 0xd7, 2),
	Abbreviation("CHR$", 0xbd, 3),
	Abbreviation("CLEAR", 0xd8, 2),
	Abbreviation("CLG", 0xda, 3),
	Abbreviation("CLOSE", 0xd9, 3),
	Abbreviation("CLS", 0xdb, 3),
	Abbreviation("COLOR", 0xfb, 1),
	Abbreviation("COLOUR", 0xfb, 1),
	Abbreviation("COS", 0x9b, 3),
	Abbreviation("COUNT", 0x9c, 3),

	Abbreviation("DATA", 0xdc, 1),
	Abbreviation("DEF", 0xdd, 3),
	Abbreviation("DEG", 0x9d, 3),
	Abbreviation("DIM", 0xde, 3),
	Abbreviation("DIV", 0x81, 3),
	Abbreviation("DRAW", 0xdf, 2),

	Abbreviation("ELSE", 0x8b, 2),
	Abbreviation("ENDPROC", 0xe1, 1),
	Abbreviation("END", 0xe0, 3),
	Abbreviation("ENVELOPE", 0xe2, 3),
	Abbreviation("EOF", 0xc5, 3),
	Abbreviation("EOR", 0x82, 3),
	Abbreviation("ERL", 0x9e, 3),
	Abbreviation("ERR", 0x9f, 3),
	Abbreviation("ERROR", 0x85, 3),
	Abbreviation("EVAL", 0xa0, 2),
	Abbreviation("EXP", 0xa1, 3),
	Abbreviation("EXT", 0xa2, 3),

	Abbreviation("FALSE", 0xa3, 2),
	Abbreviation("FN", 0xa4, 2),
	Abbreviation("FOR", 0xe3, 1),

	Abbreviation("GCOL", 0xe6, 2),
	Abbreviation("GET", 0xa5, 3),
	Abbreviation("GET$", 0xbe, 2),
	Abbreviation("GOSUB", 0xe4, 3),
	Abbreviation("GOTO", 0xe5, 1),

	Abbreviation("HIMEM", 0x93, 1),
	Abbreviation("HIMEM", 0xd3, 1),

	Abbreviation("IF", 0xe7, 2),
	Abbreviation("INKEY", 0xa6, 5),
	Abbreviation("INKEY$", 0xbf, 3),
	Abbreviation("INPUT", 0xe8, 1),
	Abbreviation("INSTR(", 0xa7, 3),
	Abbreviation("INT", 0xa8, 3),

	Abbreviation("LEFT$(", 0xc0, 2),
	Abbreviation("LEN", 0xa9, 3),
	Abbreviation("LET", 0xe9, 3),
	Abbreviation("LINE", 0x86, 4),
	Abbreviation("LN", 0xaa, 2),
	Abbreviation("LOCAL", 0xea, 3),
	Abbreviation("LOG", 0xab, 3),
	Abbreviation("LOMEM", 0x92, 3),
	Abbreviation("LOMEM", 0xd2, 3),

	Abbreviation("MID$(", 0xc1, 1),
	Abbreviation("MOD", 0x83, 3),
	Abbreviation("MODE", 0xeb, 2),
	Abbreviation("MOVE", 0xec, 3),

	Abbreviation("NEXT", 0xed, 1),
	Abbreviation("NOT", 0xac, 3),

	Abbreviation("OFF", 0x87, 3),
	Abbreviation("ON", 0xee, 2),
	Abbreviation("OPENIN", 0x8e, 2),
	Abbreviation("OPENOUT", 0xae, 5),
	Abbreviation("OPENUP", 0xad, 6),
	Abbreviation("OR", 0x84, 2),
	Abbreviation("OSCLI", 0xff, 3),

	Abbreviation("PAGE", 0x90, 2),
	Abbreviation("PAGE", 0xd0, 2),
	Abbreviation("PI", 0xaf, 2),
	Abbreviation("PLOT", 0xf0, 2),
	Abbreviation("POINT(", 0xb0, 2),
	Abbreviation("POS", 0xb1, 3),
	Abbreviation("PRINT", 0xf1, 1),
	Abbreviation("PROC", 0xf2, 3),
	Abbreviation("PTR", 0x8f, 2),
	Abbreviation("PTR", 0xcf, 2),

	Abbreviation("RAD", 0xb2, 3),
	Abbreviation("READ", 0xf3, 3),
	Abbreviation("REM", 0xf4, 3),
	Abbreviation("REPEAT", 0xf5, 3),
	Abbreviation("REPORT", 0xf6, 4),
	Abbreviation("RESTORE", 0xf7, 3),
	Abbreviation("RETURN", 0xf8, 1),
	Abbreviation("RIGHT$(", 0xc2, 2),
	Abbreviation("RND", 0xb3, 3),
	Abbreviation("RUN", 0xf9, 3),

	Abbreviation("SGN", 0xb4, 3),
	Abbreviation("SIN", 0xb5, 3),
	Abbreviation("SOUND", 0xd4, 2),
	Abbreviation("SPC", 0x89, 3),
	Abbreviation("SQR", 0xb6, 3),
	Abbreviation("STEP", 0x88, 1),
	Abbreviation("STOP", 0xfa, 3),
	Abbreviation("STR$", 0xc3, 3),
	Abbreviation("STRING$(", 0xc4, 4),

	Abbreviation("TAB(", 0x8a, 4),
	Abbreviation("TAN", 0xb7, 1),
	Abbreviation("THEN", 0x8c, 2),
	Abbreviation("TIME", 0x91, 2),
	Abbreviation("TIME", 0xd1, 2),
	Abbreviation("TO", 0xb8, 2),
	Abbreviation("TRACE", 0xfc, 2),
	Abbreviation("TRUE", 0xb9, 4),

	Abbreviation("UNTIL", 0xfd, 1),
	Abbreviation("USR", 0xba, 3),

	Abbreviation("VAL", 0xbb, 3),
	Abbreviation("VDU", 0xef, 1),
	Abbreviation("VPOS", 0xbc, 2),

	Abbreviation("WIDTH", 0xfe, 1)
};

#define HashCode(str)	(str[0] < 'A' || str[0] > 'Z' || str[1] < 'A' || str[1] > 'Z') ? 0 : ((str[0] - 'A')*26 + (str[1] - 'A'))

void SetupBASICTables()
{
	/* set QuickTable to empty */
	int c = 26*26;
	while(c--)
		QuickTable[c] = NULL;

	/* go through tokens, store strlens & populate QuickTable */
	for(c = 0; c < 0x80; c++)
	{
		if((KeyWordTable[c].StrLen = strlen(KeyWordTable[c].Name)))
		{
			/* reject any symbols that have already appeared 0x40 places earlier in the table */
			if(c < 0x40 || strcmp(KeyWordTable[c].Name, KeyWordTable[c - 0x40].Name))
			{
				int Code = HashCode(KeyWordTable[c].Name);
				KeyWord **InsertPointer = &QuickTable[Code];
				while(*InsertPointer)
					InsertPointer = &(*InsertPointer)->Next;

				*InsertPointer = &KeyWordTable[c];
			}
		}
	}

	/*

		Go through QuickTable, sorting each branch by string length

		I'm an idiot, so I've used insertion sort!

	*/
	c = 26*26;
	while(c--)
		if(QuickTable[c] && QuickTable[c]->Next)
		{
			/* sort first by string length */
			KeyWord **Check = &QuickTable[c];
			unsigned int CurLength = (*Check)->StrLen;
			Check = &(*Check)->Next;
			while(*Check)
			{
				/* check if out of order */
				if((*Check)->StrLen > CurLength)
				{
					/* unlink */
					KeyWord *Takeout = *Check;
					*Check = (*Check)->Next;

					/* start at top of list, find correct insertion point */
					KeyWord **InsertPoint = &QuickTable[c];
					while((*InsertPoint)->StrLen >= Takeout->StrLen)
						InsertPoint = &(*InsertPoint)->Next;

					/* ...and insert */
					Takeout->Next = *InsertPoint;
					*InsertPoint = Takeout;
				}
				else
				{
					CurLength = (*Check)->StrLen;
					Check = &(*Check)->Next;
				}
			}
		}
}

/*

	Little function to return an error string

*/
const char *ErrorTable[] =
{
	"",
	"BASIC is not currently active",
	"Unable to open file for input",
	"Program too large",
	"Unable to open file for output",
	"Malformed BASIC program or not running BASIC",
	"BASIC program appears to run past the end of RAM"
};
char DynamicErrorText[256];

int ErrorNum;

const char *GetBASICError()
{
	return ErrorNum >= 0 ? ErrorTable[ErrorNum] : DynamicErrorText;
}

int GetBASICErrorNum()
{
	return ErrorNum;
}


/*

	Functions to export BASIC code, i.e. decode from tokenised form to plain text

*/

bool ExtractLine(FILE *output, Uint8 *Memory, Uint16 Addr, Uint8 LineL)
{
	int LineLength = static_cast<int>(LineL);

	while(LineLength >= 0)
	{
		Uint8 ThisByte = Memory[Addr]; Addr++; LineLength--;
		if(ThisByte >= 0x80)
		{
			if(ThisByte == 0x8d) // then we're about to see a tokenised line number
			{
				Uint16 LineNumber;

				// decode weirdo tokenised line number format
				LineNumber = Memory[Addr+1]&0x3f;
				LineNumber |= (Memory[Addr+2]&0x3f) << 8;
				LineNumber |= (Memory[Addr]&0x0c) << 12;
				LineNumber |= (Memory[Addr]&0x30) << 2;
				LineNumber ^= 0x4040;

				Addr += 3;
				LineLength -= 3;

				fprintf(output, "%d", LineNumber);
			}
			else //ordinary keyword
			{
				fputs(KeyWordTable[ThisByte - 0x80].Name, output);
				
				if(KeyWordTable[ThisByte - 0x80].Flags & 0x20)
				{
					//copy to end of line without interpreting tokens}
					while(LineLength >= 0)
					{
						fputc(Memory[Addr], output); Addr++; LineLength--;
					}
					return true;
				}
			}
		}
		else
		{
			switch(ThisByte)
			{
				default: fputc(ThisByte, output); break;
				case '"':
					/* copy string literal... */
					fputc('"', output);
					while(Memory[Addr] != '"' && LineLength >= 0)
					{
						fputc(Memory[Addr], output);
						Addr++; LineLength--;
					}
					if(Memory[Addr] == '"')
					{
						fputc(Memory[Addr], output);
						Addr++; LineLength--;
					}
				break;
			}
		}
	}

	return (LineLength == -1) ? true : false;
}

bool ExportBASIC(const char *Filename, Uint8 *Memory)
{
	ErrorNum = 0;
	FILE *output = fopen(Filename, "wt");

	if(!output)
	{
		ErrorNum = 4;
		return false;
	}

	/* get the value of PAGE‚ start reading BASIC code from there */
	Uint16 Addr = Memory[0x18] << 8;

	if(Addr >= 32768 - 4)
		ErrorNum = 6;

	while(!ErrorNum)
	{
		/* character here should be \r */
		if(Memory[Addr] != 0x0d)
		{
			ErrorNum = 5;
			break;
		}
		Addr++;

		/* get line number, check if we've hit the end of BASIC */
		Uint16 LineNumber;
		LineNumber = (Memory[Addr] << 8) | Memory[Addr+1];
		Addr += 2;
		if(LineNumber & 0x8000) // if we've hit the end of the program, exit
			break;

		Uint8 LineLength = Memory[Addr]; Addr++;

		if(Addr+LineLength >= 32768 - 4)
		{
			ErrorNum = 6;
			break;
		}

		/* print line number */
		fprintf(output, "%5d", LineNumber);

		/* detokenise, etc */
		if(!ExtractLine(output, Memory, Addr, LineLength - 4))
			break;

		/* add a newline */
		fputc('\n', output);

		/* should process line here, but chicken out */
		Addr += LineLength - 4;
	}

	fclose(output);

	return ErrorNum ? false : true;
}

/*

	Functions to import BASIC code, i.e. tokenise from plain text

*/

#define AlphaNumeric(v)\
						(\
							(v >= 'a' && v <= 'z') ||\
							(v >= 'A' && v <= 'Z') ||\
							(v >= '0' && v <= '9')\
						)

char IncomingBuffer[9];
Uint8 Token, NextChar;
int TokenLen; // for Token>=0x80 only
unsigned int IncomingPointer;
FILE *inputfile;
bool EndOfFile, NumberStart;
unsigned int NumberValue, NumberLength;
int CurLine;

Uint8 *Memory;
Uint16 Addr;

inline bool WriteByte(Uint8 value)
{
	if(Addr == 32768) {ErrorNum = 3; return false;}
	Memory[Addr++] = value;
	return true;
}

int my_fgetc(FILE *in)
{
	int r;
	do
	{
		r = fgetc(in);
	}
	while(r == '\r');
	if(r == '\n') CurLine++;
	return r;
}

void GetCharacter()
{
	if(IncomingPointer == 8)
	{
		/* shift, load into position [8] */
		int c = 0;
		while(c < 8)
		{
			IncomingBuffer[c] = IncomingBuffer[c+1];
			c++;
		}

		IncomingBuffer[8] = my_fgetc(inputfile);
	}
	else
	{
		IncomingBuffer[IncomingPointer] = my_fgetc(inputfile);
		IncomingPointer++;
	}

	if(feof(inputfile)) //if we've hit feof then the last char isn't anything
	{
		IncomingPointer--;
		if(!IncomingPointer)
		{
			EndOfFile = true;
			return;
		}
	}

	/* check for tokens, set flags accordingly. Be a bit dense about this for now! */
	Token = IncomingBuffer[0];
#if 0 // SFTODO
	int Code = HashCode(IncomingBuffer);
	KeyWord *CheckPtr = QuickTable[Code];

	while(CheckPtr)
	{
		if(IncomingPointer >= CheckPtr->StrLen && !strncmp(IncomingBuffer, CheckPtr->Name, CheckPtr->StrLen))
		{
			Token = (CheckPtr - KeyWordTable) + 0x80;
			TokenLen = CheckPtr->StrLen;
			NextChar = IncomingBuffer[CheckPtr->StrLen];
			break;
		}

		CheckPtr = CheckPtr->Next;
	}
#endif

	// TODO: This is a brute-force search of entire AbbreviationTable; we could at least partition
	// it by initial letter to speed this up.
	for (size_t AbbrNum = 0; AbbrNum < sizeof(AbbreviationTable)/sizeof(AbbreviationTable[0]); ++AbbrNum)
	{
		const char *Name = AbbreviationTable[AbbrNum].Name;
		if (IncomingPointer >= strlen(Name) && !strncmp(IncomingBuffer, Name, strlen(Name)))
		{
			Token = AbbreviationTable[AbbrNum].Token;
			TokenLen = strlen(Name);
			NextChar = IncomingBuffer[TokenLen];
			break;
		}
		char *DotPtr = reinterpret_cast<char *>(memchr(IncomingBuffer, '.', IncomingPointer));
		if (DotPtr)
		{
			int AbbreviationLen = DotPtr - IncomingBuffer;
			if (AbbreviationLen >= AbbreviationTable[AbbrNum].MinAbbrLen)
			{
				int i;
				for (i = 0; i < AbbreviationLen && IncomingBuffer[i] == Name[i]; ++i)
					;
				if (i == AbbreviationLen)
				{
					Token = AbbreviationTable[AbbrNum].Token;
					TokenLen = AbbreviationLen + 1; // +1 to include '.'
					NextChar = IncomingBuffer[TokenLen];
					break;
				}
			}
		}
	}

	/* check if this is a number start */
	NumberStart = false;
	if(Token >= '0' && Token <= '9')
	{
		NumberStart = true;
		char *end;
		NumberValue = strtol(IncomingBuffer, &end, 10);
		NumberLength = end - IncomingBuffer;
	}
}

void EatCharacters(int n)
{
	/* shift left n places, decrease IncomingPointer */
	int c = 0;
	while(c < 9-n)
	{
		IncomingBuffer[c] = IncomingBuffer[c+n];
		c++;
	}
	IncomingPointer -= n;
	IncomingBuffer[IncomingPointer] = '\0';
	while(n--)		/* this is a quick fix: it causes lots of unnecessary token searches... */
		GetCharacter();
}

bool CopyStringLiteral()
{
	// eat preceeding quote
	WriteByte(IncomingBuffer[0]);
	EatCharacters(1);

	// don't tokenise anything until another quote is hit, keep eye out for things that may have gone wrong
	while(!ErrorNum && !EndOfFile && IncomingBuffer[0] != '"' && IncomingBuffer[0] != '\n')
	{
		WriteByte(IncomingBuffer[0]);
		EatCharacters(1);
	}

	if(IncomingBuffer[0] != '"') // stopped going for some reason other than a close quote
	{
		ErrorNum = -1;
		sprintf(DynamicErrorText, "Malformed string literal on line %d", CurLine);
		return false;
	}

	// eat proceeding quote
	WriteByte(IncomingBuffer[0]);
	EatCharacters(1);

	return true;
}

bool DoLineNumberTokeniser()
{
	while(!ErrorNum && !EndOfFile)
	{
		if(NumberStart)
		{
			// tokenise line number
			Uint16 LineNumber = NumberValue ^ 0x4040;

			WriteByte(0x8d);

			WriteByte(((LineNumber&0xc0) >> 2) | ((LineNumber&0xc000) >> 12) | 0x40);
			WriteByte((LineNumber&0x3f) | 0x40);
			WriteByte(((LineNumber >> 8)&0x3f) | 0x40);

			EatCharacters(NumberLength);
		}
		else
			switch(Token)
			{
				// whitespace and commas do not cause this mode to exit
				case ' ':
				case ',':
					WriteByte(Token);
					EatCharacters(1);
				break;

				// hex numbers get through unscathed too
				case '&':
					WriteByte(Token);
					EatCharacters(1);

					while(
						!ErrorNum &&
						!EndOfFile &&
						(
							(IncomingBuffer[0] >= '0' && IncomingBuffer[0] <= '9') ||
							(IncomingBuffer[0] >= 'A' && IncomingBuffer[0] <= 'F')
						)
					)
					{
						WriteByte(IncomingBuffer[0]);
						EatCharacters(1);
					}
				break;

				/* grab strings without tokenising numbers */
				case '"':
					if(!CopyStringLiteral())
						return false;
				break;

				/* default action is to turn off line number tokenising and get back to normal */
				default: return true;
			}
	}
	return true;
}

bool EncodeLine()
{
	bool StartOfStatement = true;

	/* continue until we hit a '\n' or file ends */
	while(!EndOfFile && Token != '\n' && !ErrorNum)
	{
		/* even if this looks like a keyword, it really isn't if the conditional flag is set & the next char is alphanumeric*/
		if(
			Token >= 0x80 &&
			(KeyWordTable[Token - 0x80].Flags&1) &&
			AlphaNumeric(NextChar)
			)
			Token = IncomingBuffer[0];

		if(Token < 0x80)	//if not a keyword token
		{
			switch(Token)
			{
				default:	//default is dump character to memory
					WriteByte(Token);

					if(Token == ':') // a colon always switches the tokeniser back to "start of statement" mode
						StartOfStatement = true;

					// grab entire variables rather than allowing bits to be tokenised
					if
					(
						(Token >= 'a' && Token <= 'z') ||
						(Token >= 'A' && Token <= 'Z')
					)
					{
						StartOfStatement = false;
						EatCharacters(1);
						while(AlphaNumeric(IncomingBuffer[0]))
						{
							WriteByte(IncomingBuffer[0]);
							EatCharacters(1);
						}
					}
					else
						EatCharacters(1);

				break;
				case '*':
					WriteByte(Token);
					EatCharacters(1);

					if(StartOfStatement)
					{
						/* * at start of statement means don't tokenise rest of statement, other than string literals */
						// Bugfix RTW - * commands should not be terminated by colons
						while(!EndOfFile && !ErrorNum && /*IncomingBuffer[0] != ':' &&*/ IncomingBuffer[0] != '\n')
						{
							switch(IncomingBuffer[0])
							{
								default:
									WriteByte(IncomingBuffer[0]);
									EatCharacters(1);
								break;
								case '"':
									if(!CopyStringLiteral())
										return false;
								break;
							}
						}
					}
				break;
				case '"':
					if(!CopyStringLiteral())
						return false;
				break;
			}
		}
		else
		{
			Uint8 Flags = KeyWordTable[Token - 0x80].Flags; //make copy of flags, as we're about to throwaway Token

			WriteByte(Token);	//write token
			EatCharacters(TokenLen);

			/*
			
				Effect token flags
			
			*/
			if(Flags & 0x08)
			{
				/* FN or PROC, so duplicate next set of alphanumerics without thought */
				while(!ErrorNum && !EndOfFile && AlphaNumeric(IncomingBuffer[0]))
				{
					WriteByte(IncomingBuffer[0]);
					EatCharacters(1);
				}
			}

			if(Flags & 0x10)
			{
				/* tokenise line numbers for a bit */
				if(!DoLineNumberTokeniser())
					return false;
			}

			if(Flags & 0x20)
			{
				/* REM or DATA, so copy rest of line without tokenisation */
				while(!ErrorNum && !EndOfFile && IncomingBuffer[0] != '\n')
				{
					WriteByte(IncomingBuffer[0]);
					EatCharacters(1);
				}
			}

			if(
				(Flags & 0x40) && StartOfStatement
			)
			{
				/* pseudo-variable flag */
				Memory[Addr-1] += 0x40;	//adjust just-written token
			}

			/* check if we now go into middle of statement */
			if(Flags & 0x02)
				StartOfStatement = false;

			/* check if we now go into start of statement */
			if(Flags & 0x04)
				StartOfStatement = true;
		}
	}

	EatCharacters(1);	//either eat a '\n' or have no effect at all

	return true;
}

bool ImportBASIC(const char *Filename, Uint8 *Mem, int* Size)
{
	/* store memory target to global var */
	Memory = Mem;
	ErrorNum = 0;
	Addr = 0;

#if 0
	/* get the value of PAGE‚ insert BASIC code starting from there */
	Addr = Memory[0x18] << 8;

	/* validity check: does PAGE currently point to a 0x0d? */
	if(Memory[Addr] != 0x0d)
	{
		ErrorNum = 1;
		return false;
	}

	/* validity check: does TOP - 2 point to a 0x0d, 0xff? */
	Uint16 TOPAddr = Memory[0x12] | (Memory[0x13] << 8);
	if(
		(Memory[TOPAddr-2] != 0x0d) ||
		(Memory[TOPAddr-1] != 0xff)
	)
	{
		ErrorNum = 1;
		return false;
	}
#endif

	/* open file, reset variables */
	inputfile = fopen(Filename, "rt");
	IncomingPointer = 0;
	CurLine = 1;
	EndOfFile = false;

	if(!inputfile)
	{
		ErrorNum = 2;
		return false;
	}

	/* fill input buffer */
	int c = 8;
	while(c--)
		GetCharacter();

	/* initialise this to 0 for use with automatic line numbering */
	unsigned int LastLineNumber = 0;

	while(!EndOfFile && !ErrorNum)
	{
		/* get line number */
			/* skip white space and empty lines */
			while(Token == ' ' || Token == '\t' || Token == '\r' || Token == '\n')
				EatCharacters(1);
				
			/* end of file? */
			if(EndOfFile) break;

			/* now we may see a line number */
			if(NumberStart)
			{
				if (NumberValue <= LastLineNumber)
				{
					ErrorNum = -1;
					sprintf(DynamicErrorText, "Out of sequence line numbers (%d followed by %d) at line %d", LastLineNumber, NumberValue, CurLine);
					break;
				}
				LastLineNumber = NumberValue;
				EatCharacters(NumberLength);
			}
			else
			{
				/* auto-number the line instead */
				LastLineNumber += 1;
			}
			if(LastLineNumber >= 32768)
			{
				ErrorNum = -1;
				sprintf(DynamicErrorText, "Malformed line number at line %d", CurLine);
				break;
			}
			/* inject into memory */
			WriteByte(0x0d);
			WriteByte(LastLineNumber>> 8);
			WriteByte(LastLineNumber&0xff);

		/* read rest of line, record length */
		Uint16 LengthAddr = Addr; WriteByte(0);
		if(!EncodeLine())
			break;
		Memory[LengthAddr] = static_cast<Uint8>(Addr - LengthAddr + 3);
	}

	/* write "end of program" */
	WriteByte(0x0d);
	WriteByte(0xff);
	
#if 0
	/* write TOP */
	Memory[0x12] = Addr&0xff;
	Memory[0x13] = Addr >> 8;
#endif

	// Return size of tokenised code
	if (Size != NULL)
	{
		*Size = Addr;
	}

	fclose(inputfile);
	return ErrorNum ? false : true;
}
