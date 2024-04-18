#include "stdafx.h"

#include <cstdlib>
#include <iostream>

#include "burger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

using namespace std;

static Word LineNum;               /* Line being executed from the script */
static char Delimiters[] = " \t\n";        /* Token delimiters */
static char NumDelimiters[] = " ,\t\n";    /* Value delimiters */
static char InputLine[256];        /* Input line from script */
typedef int Boolean;         /* True if I should swap the endian */
Boolean SwapEndian;
#define TRUE  1	
#define FALSE 0	
#define SwapULong(val) val = ( (((val) >> 24) & 0x000000FF) | (((val) >> 8) & 0x0000FF00) | (((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )
#define Swap2Bytes(val) ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )

#define CommandCount 10    /* Number of commands */
static char *Commands[] = {"SPRITEBEGIN","SPRITEEND","XY","PREFIX","LOAD", "ADDOFFSET", "NEXTLINE", "NEXTGROUP", "MIRRORED", "MIRROR_OFF"};

int Prefix;
int CoordX, CoordY, temp;
FILE *RezultFile, *tmpfp, *PrefixTmp, *OffsetsTmp;
int CountFrames;
int OffsetsTable[1000];
char *FileName;
char tmpName[10]="_";
char *TextPtr;  /* Pointer to string token */
Boolean Enemy, SkipFile, SKIP, OnlyOnce, MIRROR_OFF;
char **FILENAMES = new char*[100];
Word CCBFlags;
int GroupNumber, GroupCut; // ��� ������������ ������ ������, ������� � ������� ��� �� ����� ���������� �����.
//strcpy(strings[NUMBER], "�����"); //����������� �������� ��������

// ��������� ������ ��� ������� �������� ������ � ���������� 0x4000.
// ������ ������ ������� (������ ��������) - ������ ������, ������������ ����� ��������� 40.
// �������� � ������� - ������� ������.
int OFFARRAY[8][8] = {
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0}
};
int OffarrayLine=0; // ��� ����� ���������� � ������
//int NextGroup=0;	// ��� ������������ ��������� ������ �������� (����� �������� 0x4000)

struct {
		short prefix;
		int Offset;
	} Offset;

struct {
	int Data;
	} Dta;

void PrintError(char *Error)
{
	printf("# Error in Line %d, %s\n",LineNum,Error);
}






// ����� ������� �������� ��� �����.
void CalcOffsetsEnemy(void) {
int tmp, i, j, k, eight, d;
Word OffsetValue, temp, sum, TableSize, OffVal, ScrFileSize, OffsetOrd;
int m[1000], ArraySum[1000], Pos[50000], Arr[50000], OffValArr[32], OffPairs[63];
int CountLines=0;

//����� ������ ��������, �������� �� ������ OFFARRAY.
// ���� ��, �� � ������� �������� �������� ������� �������������� �������� �� ����.
// � �� ����� ��������� �������� ������� ��������.
// ������� OFFARRAY. ����� ������ ��� �����.
#if 1
for (j=0;j<8;j++) {
	for (i=0;i<8;i++) {
	cout << "OFFARRAY["<< j << "][" << i << "]=" << OFFARRAY[j][i] <<"\n";
	}
}
// ������� ����� OffsetsTable. ����� ������ ��� �����.
for (i=0;i<40;i++) {
//	cout << "Frame size["<< i << "]=" << OffsetsTable[i] << "\n";
}
#endif;

#if 0
for (j=0;j<8;j++) {
	for (i=0;i<8;i++) {
///		if (OFFARRAY[j][i] != 0) {
		tmp = OFFARRAY[j][i];
			if (tmp!=0)	{
				cout << "-----OFFARRAY is not empty!\n" ;
				break;
//			goto Contin;
		}
	}
}
#endif;

// ��������� ������ ������� ��������. 
// ������ = 1 ������ ������������ ������� OFFARRAY * �� ���-�� �������� ��������� � ��� * 4 (���� ������ 8*4).
// � ��� ��� �������� ������ ������� OFFARRAY.
// ���� ���������� ������� ������ * 4.

// ���: (���������� ���� ����������� ������ * 4) + (���������� �������� ����� ������� OFFARRAY * 4).
for (i=0;i<8;i++) {
	if (OFFARRAY[i][0] != 0) {
		CountLines++;
		cout << "---CountLines=" << CountLines << "\n" ;
		}
	}
//TableSize = CountFrames*4 + CountLines*4; 
TableSize = CountLines*4 + CountLines*4*8 + (CountFrames-CountLines*8)*4;
cout << "---TableSize=" << TableSize << "\n" ;

// ������ ��������� ���� ��� ������ ����������� ��������.
OffsetsTmp=fopen("OffsetsTmpEnemy","wb");
PrefixTmp=fopen("PrefixTmp","rb");

// ������� ������ OffValArr.
//for (i=0; i<sizeof(OffValArr); i++) {
//	OffValArr[i] = 0;
//	}


// ��� ������� ����� ���������� � �������� 40. �������� �������� = ��-�� �������� 40-�� * �� ���-�� ��� ��������� (������ 8?).
// ����� ��������� �������� �������. 
// 1 ������ � ��������� ������� �� 8 = CountLines*4 + ��� ������� ������� (��� ����� ����� ��� ��������������� 8-��, �.�. �� �� ������ ����� ��������), �.�. + CountFrames*4.
// 2 ������ � ��������� ������� �� 8 = CountLines*4 + ��� ������� �������, �.�. + CountFrames*4 + ��� ����������� ������ 8-��, �.�. 4*8.
for (i=0;i<CountLines;i++) {
	OffVal = CountLines*4 + (CountFrames-CountLines*8)*4 + i*32;
	temp = 0x40;
	fwrite(&temp,2,1,OffsetsTmp);
	OffValArr[i] = OffVal;
	temp = Swap2Bytes(OffVal);
	fwrite(&temp,2,1,OffsetsTmp); 
	cout << "-OffVal[" << i << "] 0x4000 = " << OffValArr[i] << "\n" ;
	}


// ����� ������� �������. �.�. ����� ������������� ����� ���������������, �� ������� = �������� ������ ����� �����.
// �������� ������������ ������ ����� = ��������� ������ ����� �� ����� Scratch + 84 (����� ������� ��������).
// ��� ����� ��������� ���� Scratch � ����� ������ �������� ������ CountLines*8+1 � �� CountFrames.
// �������� ��������� �������� + 84 � ���� OffsetsTmpEnemy.


// ��������� ���� Scratch � ������� � ������� �������� ���� ������.
// ���� ����� ���������� SkipFile == TRUE, �� ������ �������� CCB ��������� ��-������.

tmpfp = fopen("Scratch","rb");
fseek(tmpfp,0,SEEK_END);
ScrFileSize = ftell(tmpfp);
cout << "ScrFileSize=" << ScrFileSize << "\n" ;
rewind(tmpfp);

OffsetOrd = 0;
k=0;
d=0;
for (i=0; i<ScrFileSize/4; i++) {
Cont:
		fread(&Dta.Data,4,1,tmpfp);
		temp = Dta.Data;
		OffsetOrd += 4;
//		if (temp == 0x0006E647) {
		if (temp == CCBFlags) {
//				cin.get();
				Pos[i] = OffsetOrd-8; // -8, ������ ��� �������� �����: ���������� � 47E60600...
				Arr[k] = Pos[i];		// ����� ��� ������� � ��������� ������.
//				if (k==5) {
//					d++;
//				}

				if ((SKIP == TRUE) && (GroupCut > 0) && ((k==4)||(k==12)||(k==20)||(k==28)||(k==36)||(k==44)||(k==52)||(k==60))) {
				//if ((SKIP == TRUE) && ((k==4)||(k==12)||(k==20)||(k==28)||(k==36)||(k==44)||(k==52)||(k==60))) {
//				if ((SKIP == TRUE) && ((d==1)||(k==2)||(k==3)||(k==4)||(k==5)||(k==6)||(k==7)||(k==8))) {
					//if GroupArr[GroupCut*8]
					///	if ((SKIP == TRUE) && (!(k % 8))) {
							Arr[k+1] = Arr[k-1];
							cout << "ARR 6 = " << Arr[k+1] << "\n";
							Arr[k+2] = Arr[k-2];
							cout << "ARR 7 = " << Arr[k+2] << "\n";
							Arr[k+3] = Arr[k-3];
							cout << "ARR 8 = " << Arr[k+3] << "\n\n";
							k += 3;
							GroupCut--;
//	d++;
//							goto Cont;
					}
#if 0
					// ����� ��������, ���������� �� �� ����� ����� � �������������� ���������� ���������� �������� Arr[]
					// ���� ����� ����� 6,
					if ((SKIP == TRUE) && ((k==5)||(k==13)||(k==21)||(k==29)||(k==37)||(k==45)||(k==53)||(k==61))) {
						cout << "ARR 6 = " << Arr[k] << "\n";
						Arr[k] = Arr[k-2];
			//			k++;
						i--;
						//break;
						}
					// ���� ����� ����� 7,
					if ((SKIP == TRUE) && ((k==6)||(k==14)||(k==22)||(k==30)||(k==38)||(k==46)||(k==54)||(k==62))) {
						cout << "ARR 7 = " << Arr[k] << "\n";
						Arr[k] = Arr[k-4];
			//			k++;
						i--;
						//break;
						}
					// ���� ����� ����� 8,
					if ((SKIP == TRUE) && ((k==7)||(k==15)||(k==23)||(k==31)||(k==39)||(k==47)||(k==55)||(k==63))) {
						cout << "ARR 8 = " << Arr[k] << "\n";
						Arr[k] = Arr[k-6];
			//			k++;
						i--;
						//break;
						}

#endif;
				k++;
//				cout << "Frame found! Offset= " << Pos[i] << "\n";
				} 
}
fclose(tmpfp);


// ��� �����. ��� ������ �������� CCB �� ����� Scratch.
for (i=0; i<CountFrames; i++) {
	cout << "Offset (Arr[i])= " << Arr[i] << "\n";
}


/*----------------------------------------------------------------------------------------------------------*/
// ����� ������� �������, ���� ���������� ������������� ����� - SkipFile == TRUE;
// ����������� ������� �������� OffsetsTable[i]?
// ����� � �������� 6,7,8 � ������� 8-�� ����� ������ �����������, � ���������� 0�80
#if 0
// ���� SkipFile = TRUE; �������� ��������� ������ ����� = i=(CountLines*5) -- i<CountFrames-CountLines*3;
if (SKIP==TRUE) {
	for (i=(CountLines*5); i<(CountFrames-CountLines*3); i++) {
		OffVal = Arr[i]+TableSize;
		cout << "OffVal (SKIPed)= " << OffVal << "\n";						// ��� �����
		// ����� ��������� ������ ���� ��������� �������� � ���� OffsetsTmp
		// �������� ������� �� ����� PrefixTmp. ������ ���������� ����� � ������ ����� ��������������� 8-�.
		fseek(PrefixTmp,i*2,SEEK_SET);////////////////////////////////////////////////////////////////////////////////////////////
		fread(&Dta.Data,2,1,PrefixTmp);

		fwrite(&Dta.Data,2,1,OffsetsTmp);

		temp = Swap2Bytes(OffVal);
		fwrite(&temp,2,1,OffsetsTmp);
	}
 goto PrefixesPLUS; // ������������� ����� �������� �������� �������� ��������� ������ (��� SKIP)
}
#endif;
/*----------------------------------------------------------------------------------------------------------*/



// ���������� �������� ������ ��� ��������� ������.
for (i=(CountLines*8); i<CountFrames; i++) {
	OffVal = Arr[i]+TableSize;

	cout << "OffValOrdinary= " << OffVal << "\n";
	// ������� �������� � ���� OffsetsTmpEnemy.
// �������� ������� �� ����� PrefixTmp. ������ ���������� ����� � ������ ����� ��������������� 8-�.
	// �������� 07.02.2017
	fseek(PrefixTmp,i*2,SEEK_SET);////////////////////////////////////////////////////////////////////////////////////////////
	fread(&Dta.Data,2,1,PrefixTmp);

		if (OffVal>0xFFFF) { // ���� �������� ������ 2-� ����, ����� 3 ����� ������ 2-�. �� ��� ���� �������� ��� �����������.
			cout << "Ordinary offset is greater than 0xFFFF. Frame #" << i << "\n";	
			cout << "so Prefix value is not allowed!\n\n";
				temp = SwapULong(OffVal);
				fwrite(&temp,4,1,OffsetsTmp);
		} else {

///	cout << "Dta.Data= " << Dta.Data << "\n";
	fwrite(&Dta.Data,2,1,OffsetsTmp);
///	temp = 0;
///	fwrite(&temp,2,1,OffsetsTmp);
	temp = Swap2Bytes(OffVal);
	fwrite(&temp,2,1,OffsetsTmp);
	}
}



/*----------------------------------------------------------------------------------------------------------*/
PrefixesPLUS:

#if 0
// ����� ������� �������� ������� � 8-���. �� ����� ��������� ������ Arr[] ����������� ���������� �������

// �������� ������ OffPairs.
for (i=0; i<sizeof(OffPairs); i++) {
	OffPairs[i] = 0;
	}

if (SKIP==TRUE) {
	tmpfp = fopen("Scratch","rb");
	eight = 0;

	for (i=0; i<CountLines*8; i++) {
		// ������ �������������� ����������, ������� ����������� ������ 8.
		if ((i==8)||(i==16)||(i==24)||(i==32)||(i==40)||(i==48)||(i==56)||(i==64)||(i==72)) {
			eight++;	
			}
		OffVal = Arr[i]+TableSize-OffValArr[eight]; // �������� �������� ��� ������������ ������� � 0�4000

			// ���� ����� ����� 6,
			if ((i==5)||(i==13)||(i==21)||(i==29)||(i==37)||(i==45)||(i==53)||(i==61)) {
				OffVal = 0x8888;
//				OffVal = Arr[i-2]+TableSize;	
			}
			// ���� ����� ����� 7,
			if ((i==6)||(i==14)||(i==22)||(i==30)||(i==38)||(i==46)||(i==54)||(i==62)) {
				OffVal = 0x8888;
//				OffVal = Arr[i-4]+TableSize;	
			}
			// ���� ����� ����� 8,
			if ((i==7)||(i==15)||(i==23)||(i==31)||(i==38)||(i==47)||(i==55)||(i==63)) {
				OffVal = 0x8888;
//				OffVal = Arr[i-6]+TableSize;	
			}

	// ������� �������� � ���� OffsetsTmpEnemy.
// �������� ������� �� ����� PrefixTmp. ������ ���������� ����� � ������ ����� ��������������� 8-�.
		fseek(PrefixTmp,i*2,SEEK_SET);/////////////////////////////////////////////////////////////////////////////////////////
		fread(&Dta.Data,2,1,PrefixTmp);
		fwrite(&Dta.Data,2,1,OffsetsTmp);

		temp = Swap2Bytes(OffVal);
		fwrite(&temp,2,1,OffsetsTmp);
}
goto Away;
}
#endif;
/*----------------------------------------------------------------------------------------------------------*/



// ����� �������� �������� ������� �� 8-���.
// ��������� ���� Scratch � ����� ������ �������� � ������ 1 ... CountLines*8, ��������� � ��� 84 (����� ������� ��������), ������ �� ��� ��������������� �� ������ ��������� ������ 0�4000.
tmpfp = fopen("Scratch","rb");
eight = 0;

for (i=0; i<CountLines*8; i++) {
	// ������ �������������� ����������, ������� ����������� ������ 8.
	if ((i==8)||(i==16)||(i==24)||(i==32)||(i==40)||(i==48)||(i==56)||(i==64)||(i==72)) {
		eight++;	
	}
//	OffVal = Arr[i]+84-OffValArr[eight]+eight*32;
	OffVal = Arr[i]+TableSize-OffValArr[eight];		// �������� �������� ��� ������������ ������� � 0�4000

	// ������� �������� � ���� OffsetsTmpEnemy.
// �������� ������� �� ����� PrefixTmp. ������ ���������� ����� � ������ ����� ��������������� 8-�.
// �������� 07.02.2017
	fseek(PrefixTmp,i*2,SEEK_SET);/////////////////////////////////////////////////////////////////////////////////////////
	fread(&Dta.Data,2,1,PrefixTmp);
			if (OffVal>0xFFFF) { // ���� �������� ������ 2-� ����, ����� 3 ����� ������ 2-�. �� ��� ���� �������� ��� �����������.
				cout << "Offset is greater than 0xFFFF. Frame# " << i << "\n";	
				cout << "so Prefix value is not allowed!\n\n";
			//	fwrite(&Dta.Data,1,1,OffsetsTmp);
				temp = SwapULong(OffVal);
				fwrite(&temp,4,1,OffsetsTmp);
			} else {
				fwrite(&Dta.Data,2,1,OffsetsTmp);
				///	temp = 0;
				///	fwrite(&temp,2,1,OffsetsTmp);
				temp = Swap2Bytes(OffVal);
				fwrite(&temp,2,1,OffsetsTmp);
			}
}

Away:
fclose(tmpfp);
fclose(OffsetsTmp);
fclose(PrefixTmp);
}



// ������� ��� �������� ��� ����� ��������.
// �������� ������: ������� ���� ������ � ��������������� ��������.
void CalcOffsets(void) {
int i, j, OffsetValue, temp, sum;
int m[1000], ArraySum[1000];

for (i=0;i<1000;i++) {
	ArraySum[i]=0;
	}
// ������� ������ ������ �� ������� � ��������� ������ ������. 
for (i=1;i<=CountFrames;i++) {
	cout << "Frame #" << i << " Size= " << OffsetsTable[i] << "\n";
}
// ������ ��������� ���� ��� ������ ����������� ��������.
OffsetsTmp=fopen("OffsetsTmp","wb");

// ������� ����� � ������ ��������� � ���, �������:
// ������ ������ ������� �� ����� PrefixTmp, ����� � �������� ���� OffsetsTmp,
// ����� ����������� �������� � ������� ����� �������.
// ������ � ����� ��������� ������� ������ �� ��������� ���������. 
// � ��� CountFrames ���.

// ������� ������������� �������� ���� ��������
sum=0;

// ��������� �������� ������ ����� CountFrames*4
ArraySum[0]=CountFrames*4;

for (i=1;i<=CountFrames;i++) {
	sum = sum+OffsetsTable[i];
//	cout << "\n----------OffsetsTable Summ #" << i << "= " << sum << "\n";
	ArraySum[i] = sum+CountFrames*4+i*4;
	}



PrefixTmp=fopen("PrefixTmp","rb");

for (i=0;i<CountFrames;i++) {
	OffsetValue = ArraySum[i];
	cout << "OffsetValue #" << i << "= " << OffsetValue << "\n";

	///// ���� �������� ������� ������� �������:
		if (OffsetValue>0xFFFF) { // ���� �������� ������ 2-� ����, ����� 4 ����� ������ 2-�. �� ��� ���� �������� ��� �����������.
			cout << "Offset is greater than 0xFFFF. Frame #" << i;	
///			cout << "so Prefix value is not allowed!\n\n";
				temp = SwapULong(OffsetValue);
				fwrite(&temp,4,1,OffsetsTmp);
				goto SkipPref;
				} 
		else 
		{

	fread(&Dta.Data,2,1,PrefixTmp);
//	cout << "\n--Prefix #" << i << "= " << Dta.Data << "\n";
	temp=Dta.Data;
	fwrite(&temp,2,1,OffsetsTmp);

	// ����� ������������ ������:
	temp=Swap2Bytes(OffsetValue);
	fwrite(&temp,2,1,OffsetsTmp);
SkipPref:
	if (OffsetValue>0xFFFF) { cout << ". Prefix is disabled!\n"; }
	}
}
// ��� ����� ��� �������� �����, ���� ����� ������������� ����� ������� ������� �������� ��� ������.
// ��������� ���.
Contin:

fclose(OffsetsTmp);
fclose(PrefixTmp);
}




void SpriteBegin(void)
{
int j, i;
// ���������� ��� � ������� �������� ���� �������
//char *TextPtr;

CountFrames=0; // �������� ������� ������.
OffarrayLine=0; // �������� ������� ����� ������� �������� ������.
Enemy = FALSE; // ������ �� �������. �.�., ���� �� �� �����, ����� ��.
SkipFile = FALSE; // ���� ���� ��� ������������, ����� ����� ���������, � �� ����� ����� ������ �� �������.
SKIP = FALSE; // ���� SkipFile ���� �� �����������.
GroupNumber = 0;
GroupCut = 9; // ������ ��� ����, ����� �������� ��� �������������. � ����� ������, �� ��� ������������� ������������� �����������.
// ��������� ��������� ����. ���� ����� ������ ��� ����� �������.
// ����� ����� ��� ���������.
tmpfp=fopen("Scratch","wb");
PrefixTmp=fopen("PrefixTmp","wb");

TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Needed filename for SPRITEBEGIN command.");
		return;
	}

RezultFile = fopen(TextPtr,"wb");
fclose(RezultFile);

cout << "\nNew Sprite file created: " << TextPtr << "\n";
//cout << "\n---FileName =" << FileName << "\n";
// ������� OFFARRAY. ��� �����, ���� �� ��� ������������ ������ ����� ������.
#if 1
for (j=0;j<8;j++) {
	for (i=0;i<8;i++) {
	OFFARRAY[j][i] = 0;
//	cout << "OFFARRAY-ZERO["<< j << "][" << i << "]=" << OFFARRAY[j][i] <<"\n";
	}
}
#endif;
}




void SpriteEnd(void)
{
int tmp, FileSize, i;

struct {
	int Data;
	} Dta;
//char *TextPtr;


TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Needed filename for SPRITEEND command.");
		return;
	}

// ������� ���������� ������. �� ����� ����� �������� ����� ������� ��������.
cout << "\nFrames count = "<< CountFrames <<"\n";

// ��������� ���� � ���������� ������� � ������������.
fclose(tmpfp);
fclose(PrefixTmp);

// �������� ��� ������. ������� ��������� ������� ��������.
// ���� ������ �������, �� ��������� ��� ���� ������ ������� ��������.

if (Enemy==TRUE) {
	CalcOffsetsEnemy();
}	else {
	CalcOffsets();
	}

// ������ ����� ���������� ���� Scratch � ���� ������� ��������.
RezultFile = fopen(TextPtr,"wb");
if (Enemy==TRUE) {
	OffsetsTmp=fopen("OffsetsTmpEnemy","rb");
} else {
	OffsetsTmp=fopen("OffsetsTmp","rb");
	}
tmpfp=fopen("Scratch","rb");

fseek(OffsetsTmp,0,SEEK_END);
FileSize = ftell(OffsetsTmp);
rewind(OffsetsTmp);
//cout << "\n---FileName =" << TextPtr << "\n";

// ������ ������� � ����� � �������������� ���� �������.
for (i=0; i<FileSize; i++) {
	fread(&Dta.Data,1,1,OffsetsTmp);
	tmp=Dta.Data;
	fwrite(&tmp,1,1,RezultFile);
}


// ������ ���� Scratch � ����� � �������������� ���� �������.
fseek(tmpfp,0,SEEK_END);
FileSize = ftell(tmpfp);
rewind(tmpfp);

for (i=0; i<FileSize; i++) {
	fread(&Dta.Data,1,1,tmpfp);
	tmp=Dta.Data;
	fwrite(&tmp,1,1,RezultFile);
}

fclose(OffsetsTmp);
fclose(tmpfp);
fclose(RezultFile);
//cout << "\nGroups N = "<< GroupNumber <<"\n";
cout << "\nSprite file creation finished. \n\n";
SKIP=FALSE;
}




void SetCoordinates(void)
{
//char *TextPtr;

TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough params for X,Y");
		return;
	}

CoordX = atoi(TextPtr);
TextPtr = strtok(0,NumDelimiters);
CoordY = atoi(TextPtr);

cout << "\n";
////*/cout << "\nFrame coordinate X:" << CoordX << "\n";
////*/cout << "Frame coordinate Y:" << CoordY << "\n";
}




void SetPrefix(void)
{
TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough params for PREFIX");
		return;
	}
Prefix = atoi(TextPtr);
////*/cout << "Prefix for the frame:" << Prefix << "\n";
// ����� ������� � �������� ����.
fwrite(&Prefix,2,1,PrefixTmp);
}



/*----------------------------------------------------------------------------------------------------------*/
// ��� �������� ����� ����� ��������� ���������� ����� ������, �� ������ ��������.
// � ���������� ��� �� ����� ����������� � Scratch, �� ����� �������������� � ������� ��������
// � ������ ���������.

// ����� ������� ������������ ������������� ���� ������, �� ��������� ��������� � �������� ������.
// ��� ���� ������, �������� � ������ 8-�� ������� �������� ����� ��������� 0�4000 ����������� ��-������.

// ��� ����� ������ ��� ����� ����� � ������. � ��� �������� ���������� ����� ��������� ���� ������ �� �������
// �������. ����� ��������� �������.
/*----------------------------------------------------------------------------------------------------------*/
void LoadFile(Word Config)
{
FILE *infile;
//char *TextPtr;
int i, FileSize, data;

struct {
		Word Data;
	} Data;

TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough params for LOAD");
		return;
	}

infile = fopen(TextPtr,"rb");
	if (!infile) {
		cout << "File " << TextPtr << " was not found!\n";
		//return;
		exit;
	//	Environment.Exit(0);
		} else

cout << "Loading file: " << TextPtr << "\n";
CountFrames++; // ������� ����. ����� ��� ����������� ����� ������� ��������.

if (SkipFile == TRUE) { // ���� ���� ������ �� �����,
	goto SkipWriting;
}

//����� �� ��������� ���� ���������� � �����.
temp=Swap2Bytes(CoordX);
fwrite(&temp,2,1,tmpfp);
temp=Swap2Bytes(CoordY);
fwrite(&temp,2,1,tmpfp);


fseek(infile,0,SEEK_END);
FileSize = ftell(infile);
////*/cout << "Frame FileSize=" << FileSize << "\n";
rewind(infile);

// ������� ������ ����� � ������ �������� ������.
OffsetsTable[CountFrames] = FileSize;
///CountFrames++; // ������� ����. ����� ��� ����������� ����� ������� ��������.
//cout << "OffsetsTable[i]=" << OffsetsTable[CountFrames] << "\n";


// ������ ���� ���� ����� � ����� ��� �� ��������� ����.
for (i=0;i<FileSize;i++) {
	// ������� ���������� ����� CCB, ��� ����� ���� �������, ������� �������� ������ ���������� � ���������� CCBFlags.
#if 1
		if (OnlyOnce == TRUE) {
			fread(&Data.Data,4,1,infile);
			CCBFlags = Data.Data;
			cout << "CCBFlags=" << CCBFlags << "\n";
			rewind(infile);		// ������������ ���� �� ������.
			OnlyOnce = FALSE; // ������� ����, ������ ���������� ����� �� �����.
//			cin.get();
			}
#endif;
	fread(&Data.Data,1,1,infile);
	data=Data.Data;
	fwrite(&data,1,1,tmpfp);
	}

SkipWriting:
fclose(infile);
SkipFile = FALSE; // ���� ���� ��� ������������, ����� ����� ���������, � �� ����� ����� ������ �� �������.
//fclose(tmpfp);
}



void AddOffset(void) {
//������� ��� ��������� ����������� ������� �������� ��� ������.
TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough params for ADDOFFSET");
		return;
	}

OFFARRAY[atoi(TextPtr)][OffarrayLine] = OffsetsTable[CountFrames];
	if (OffarrayLine == 0) {
		GroupNumber++;
	}
////*/cout << "OFFARRAY["<< TextPtr << "][" << OffarrayLine << "]=" << OffsetsTable[CountFrames] <<"\n";
}


void NewLine(void) {
	OffarrayLine++;
	}

void NewGroup(void) {
	OffarrayLine=0;
	// ��� �������� ������ ����� ������, ��� ������� �������� ����� ���������� �� � ������ ���������� �����,
	// � 
	}


void NotSaveFile(void) {
SKIP = TRUE; 
cout << "Following file is a mirrored copy.\n";
}


void ReadScript(FILE* fp)
{
//	char *TextPtr;  /* Pointer to string token */
	Word i;         /* Index */

	while (fgets(InputLine,sizeof(InputLine),fp)) { /* Get a string */
		++LineNum;          /* Adjust the line # */
		TextPtr = strtok(InputLine,Delimiters); /* Get the first token */
		if (!TextPtr) {
			continue;
		}
		i = 0;      /* Check for the first command */
		if (isalnum(TextPtr[0])) {  /* Comment? */
			do {
				if (!strcmp(TextPtr,Commands[i])) { /* Match? */
					switch (i) {        /* Execute the command */
					case 0:
						SpriteBegin();  
						break;
					case 1:
						SpriteEnd();  
						break;
					case 2:
						SetCoordinates();    
						break;
					case 3:
						SetPrefix();	
						break;
					case 4:
						LoadFile(0);	
						break;
					case 5:
						AddOffset();
						Enemy = TRUE;
						break;
					case 6:
						NewLine();
						break;
					case 7:
						NewGroup();
						break;
					case 8:
						NotSaveFile();
						MIRROR_OFF = FALSE;
						SkipFile = TRUE;
						break;
					case 9: 
						// ���� � ������� ���� ������� ������������� ��������� ������� ������ MIRRORED
						// �� ����� ���������� ����� � ����� ������.
						MIRROR_OFF = TRUE;
						GroupCut = GroupNumber;
					}
					break;      /* Don't parse anymore! */
				}
			} while (++i<CommandCount); /* Keep checking */
		}
		if (i==CommandCount) {      /* Didn't find it? */
			printf("# Command %s not implemented\n",TextPtr);
		}
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	FILE *fp;               /* Input file */
argc=2;
	if (argc!=2) {          /* Gotta have input and output arguments */
		printf("# Cel2Sprite app.\n"
			"# This program will create a Sprites files using a script\n"
			"# Usage: Cel2Sprite Infile\n");
		return 1;
	}

    fp = fopen("Cels.txt","r");    /* Read the ASCII script */
	if (!fp) {
		printf("# Can't open Cels.txt file %s.\n",argv[1]);    /* Oh oh */
//		free(Buffer);
		return 1;
	}

// ����� �������� ����������� ���� ������ ����� � �������, ������ ����������� ////*/
OnlyOnce = TRUE; // ��� ����������� ������ CCB. 
// ������ ���� �������.
	ReadScript(fp);
	fclose(fp);     /* Close the file */


// ������:
remove("Scratch");
remove("PrefixTmp");
remove("OffsetsTmp");
remove("OffsetsTmpEnemy");
	
	cout << "\nPress the Enter key to continue ...";
    cin.get();
    return 0;
}