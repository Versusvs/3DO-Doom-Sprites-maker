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
int GroupNumber, GroupCut; // для отслеживания номера группы, начиная с которой уже не нужно пропускать кадры.
//strcpy(strings[NUMBER], "слово"); //присваиваем значение элементу

// Двумерный массив для таблицы оффсетов вражин с префиксами 0x4000.
// Каждая строка массива (первое значение) - группа кадров, объединенных одним префиксом 40.
// Значения в строках - Размеры кадров.
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
int OffarrayLine=0; // Для счета параметров в строке
//int NextGroup=0;	// Для отслеживания изменения группы спрайтов (новое смещение 0x4000)

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






// Здесь считаем смещения для врага.
void CalcOffsetsEnemy(void) {
int tmp, i, j, k, eight, d;
Word OffsetValue, temp, sum, TableSize, OffVal, ScrFileSize, OffsetOrd;
int m[1000], ArraySum[1000], Pos[50000], Arr[50000], OffValArr[32], OffPairs[63];
int CountLines=0;

//Здесь делаем проверку, заполнен ли массив OFFARRAY.
// Если да, то в таблицу оффсетов попадают сначала сформированные значения из него.
// А уж затем вычисляем значения обычных оффсетов.
// Выведем OFFARRAY. Нужно только для теста.
#if 1
for (j=0;j<8;j++) {
	for (i=0;i<8;i++) {
	cout << "OFFARRAY["<< j << "][" << i << "]=" << OFFARRAY[j][i] <<"\n";
	}
}
// Выведем часть OffsetsTable. Нужно только для теста.
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

// Посчитаем размер таблицы оффсетов. 
// Размер = 1 строка заполненного массива OFFARRAY * на кол-во значимых элементов в нем * 4 (либо просто 8*4).
// и так все значимые строки массива OFFARRAY.
// Плюс количество обычных кадров * 4.

// ИЛИ: (Количество всех загруженных кадров * 4) + (количество значимых строк массива OFFARRAY * 4).
for (i=0;i<8;i++) {
	if (OFFARRAY[i][0] != 0) {
		CountLines++;
		cout << "---CountLines=" << CountLines << "\n" ;
		}
	}
//TableSize = CountFrames*4 + CountLines*4; 
TableSize = CountLines*4 + CountLines*4*8 + (CountFrames-CountLines*8)*4;
cout << "---TableSize=" << TableSize << "\n" ;

// Делаем временный файл для записи посчитанных оффсетов.
OffsetsTmp=fopen("OffsetsTmpEnemy","wb");
PrefixTmp=fopen("PrefixTmp","rb");

// Обнулим массив OffValArr.
//for (i=0; i<sizeof(OffValArr); i++) {
//	OffValArr[i] = 0;
//	}


// Все оффсеты строк начинаются с префикса 40. Значение смещения = зн-ие текущего 40-го * на кол-во его элементов (всегда 8?).
// Пишем начальные плюсовые оффсеты. 
// 1 оффсет к реальному первому из 8 = CountLines*4 + все обычные оффсеты (все кадры минус все сгруппированные 8-ки, т.к. их мы писать будем отдельно), т.е. + CountFrames*4.
// 2 оффсет к реальному первому из 8 = CountLines*4 + все обычные оффсеты, т.е. + CountFrames*4 + уже отсчитанная первая 8-ка, т.е. 4*8.
for (i=0;i<CountLines;i++) {
	OffVal = CountLines*4 + (CountFrames-CountLines*8)*4 + i*32;
	temp = 0x40;
	fwrite(&temp,2,1,OffsetsTmp);
	OffValArr[i] = OffVal;
	temp = Swap2Bytes(OffVal);
	fwrite(&temp,2,1,OffsetsTmp); 
	cout << "-OffVal[" << i << "] 0x4000 = " << OffValArr[i] << "\n" ;
	}


// Пишем обычные оффсеты. Т.к. кадры располагаются после сгруппированных, то оффсеты = оффсетам кадров после групп.
// Реальное расположение начала кадра = положение начала кадра из файла Scratch + 84 (длина таблицы оффсетов).
// Нам нужно прочитать файл Scratch и взять оттуда смещения кадров CountLines*8+1 и до CountFrames.
// Записать найденные смещения + 84 в файл OffsetsTmpEnemy.


// Открываем файл Scratch с кадрами и находим смещения всех кадров.
// Если кадры пропускали SkipFile == TRUE, то массив смещений CCB заполняем по-своему.

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
				Pos[i] = OffsetOrd-8; // -8, потому что отнимаем шапку: координаты и 47E60600...
				Arr[k] = Pos[i];		// Пишем все оффсеты в отдельный массив.
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
					// Здесь отследим, пропускали ли мы кадры ранее и соответственно сформируем правильное значение Arr[]
					// Если номер файла 6,
					if ((SKIP == TRUE) && ((k==5)||(k==13)||(k==21)||(k==29)||(k==37)||(k==45)||(k==53)||(k==61))) {
						cout << "ARR 6 = " << Arr[k] << "\n";
						Arr[k] = Arr[k-2];
			//			k++;
						i--;
						//break;
						}
					// Если номер файла 7,
					if ((SKIP == TRUE) && ((k==6)||(k==14)||(k==22)||(k==30)||(k==38)||(k==46)||(k==54)||(k==62))) {
						cout << "ARR 7 = " << Arr[k] << "\n";
						Arr[k] = Arr[k-4];
			//			k++;
						i--;
						//break;
						}
					// Если номер файла 8,
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


// Для теста. Это массив смещений CCB из файла Scratch.
for (i=0; i<CountFrames; i++) {
	cout << "Offset (Arr[i])= " << Arr[i] << "\n";
}


/*----------------------------------------------------------------------------------------------------------*/
// Здесь считаем оффсеты, если попадались несохраняемые кадры - SkipFile == TRUE;
// Отслеживаем нулевые значения OffsetsTable[i]?
// Кадры с номерами 6,7,8 в группах 8-ок будут ВСЕГДА зеркальными, с префиксами 0х80
#if 0
// Если SkipFile = TRUE; Смещения отдельных кадров будут = i=(CountLines*5) -- i<CountFrames-CountLines*3;
if (SKIP==TRUE) {
	for (i=(CountLines*5); i<(CountFrames-CountLines*3); i++) {
		OffVal = Arr[i]+TableSize;
		cout << "OffVal (SKIPed)= " << OffVal << "\n";						// Для теста
		// Здесь прописать запись этих найденных смещений в файл OffsetsTmp
		// Префиксы возьмем из файла PrefixTmp. Только обратиться нужно к кадрам после сгруппированных 8-к.
		fseek(PrefixTmp,i*2,SEEK_SET);////////////////////////////////////////////////////////////////////////////////////////////
		fread(&Dta.Data,2,1,PrefixTmp);

		fwrite(&Dta.Data,2,1,OffsetsTmp);

		temp = Swap2Bytes(OffVal);
		fwrite(&temp,2,1,OffsetsTmp);
	}
 goto PrefixesPLUS; // Перескакиваем через алгоритм подсчета смещений отдельных кадров (без SKIP)
}
#endif;
/*----------------------------------------------------------------------------------------------------------*/



// Определяем смещения нужных нам отдельных кадров.
for (i=(CountLines*8); i<CountFrames; i++) {
	OffVal = Arr[i]+TableSize;

	cout << "OffValOrdinary= " << OffVal << "\n";
	// Запишем смещения в файл OffsetsTmpEnemy.
// Префиксы возьмем из файла PrefixTmp. Только обратиться нужно к кадрам после сгруппированных 8-к.
	// Изменено 07.02.2017
	fseek(PrefixTmp,i*2,SEEK_SET);////////////////////////////////////////////////////////////////////////////////////////////
	fread(&Dta.Data,2,1,PrefixTmp);

		if (OffVal>0xFFFF) { // Если Смещение больше 2-х байт, пишем 3 байта вместо 2-х. Но при этом префиксы уже недопустимы.
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
// Здесь считаем плюсовые оффсеты к 8-кам. Но лучше заполнять массив Arr[] правильными значениями заранее

// Обнуляем массив OffPairs.
for (i=0; i<sizeof(OffPairs); i++) {
	OffPairs[i] = 0;
	}

if (SKIP==TRUE) {
	tmpfp = fopen("Scratch","rb");
	eight = 0;

	for (i=0; i<CountLines*8; i++) {
		// Вводим дополнительную переменную, которая отслеживает каждую 8.
		if ((i==8)||(i==16)||(i==24)||(i==32)||(i==40)||(i==48)||(i==56)||(i==64)||(i==72)) {
			eight++;	
			}
		OffVal = Arr[i]+TableSize-OffValArr[eight]; // отнимаем значение уже посчитанного оффсета с 0х4000

			// Если номер файла 6,
			if ((i==5)||(i==13)||(i==21)||(i==29)||(i==37)||(i==45)||(i==53)||(i==61)) {
				OffVal = 0x8888;
//				OffVal = Arr[i-2]+TableSize;	
			}
			// Если номер файла 7,
			if ((i==6)||(i==14)||(i==22)||(i==30)||(i==38)||(i==46)||(i==54)||(i==62)) {
				OffVal = 0x8888;
//				OffVal = Arr[i-4]+TableSize;	
			}
			// Если номер файла 8,
			if ((i==7)||(i==15)||(i==23)||(i==31)||(i==38)||(i==47)||(i==55)||(i==63)) {
				OffVal = 0x8888;
//				OffVal = Arr[i-6]+TableSize;	
			}

	// Запишем смещения в файл OffsetsTmpEnemy.
// Префиксы возьмем из файла PrefixTmp. Только обратиться нужно к кадрам после сгруппированных 8-к.
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



// Пишем реальные плюсовые оффсеты по 8-кам.
// Прочитать файл Scratch и взять оттуда смещения к кадрам 1 ... CountLines*8, прибавить к ним 84 (длина таблицы оффсетов), отнять от них соответствующий их группе начальный оффсет 0х4000.
tmpfp = fopen("Scratch","rb");
eight = 0;

for (i=0; i<CountLines*8; i++) {
	// Вводим дополнительную переменную, которая отслеживает каждую 8.
	if ((i==8)||(i==16)||(i==24)||(i==32)||(i==40)||(i==48)||(i==56)||(i==64)||(i==72)) {
		eight++;	
	}
//	OffVal = Arr[i]+84-OffValArr[eight]+eight*32;
	OffVal = Arr[i]+TableSize-OffValArr[eight];		// отнимаем значение уже посчитанного оффсета с 0х4000

	// Запишем смещения в файл OffsetsTmpEnemy.
// Префиксы возьмем из файла PrefixTmp. Только обратиться нужно к кадрам после сгруппированных 8-к.
// Изменено 07.02.2017
	fseek(PrefixTmp,i*2,SEEK_SET);/////////////////////////////////////////////////////////////////////////////////////////
	fread(&Dta.Data,2,1,PrefixTmp);
			if (OffVal>0xFFFF) { // Если Смещение больше 2-х байт, пишем 3 байта вместо 2-х. Но при этом префиксы уже недопустимы.
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



// Считаем все смещения для шапки оффсетов.
// Исходные данные: размеры всех кадров и соответствующие префиксы.
void CalcOffsets(void) {
int i, j, OffsetValue, temp, sum;
int m[1000], ArraySum[1000];

for (i=0;i<1000;i++) {
	ArraySum[i]=0;
	}
// Размеры кадров узнаем из массива с размерами файлов кадров. 
for (i=1;i<=CountFrames;i++) {
	cout << "Frame #" << i << " Size= " << OffsetsTable[i] << "\n";
}
// Делаем временный файл для записи посчитанных оффсетов.
OffsetsTmp=fopen("OffsetsTmp","wb");

// Оффсеты пишем с учетом префиксов к ним, поэтому:
// Читаем первый префикс из файла PrefixTmp, пишем в выходной файл OffsetsTmp,
// Пишем вычисленное смещение к первому кадру спрайта.
// Читаем и пишем следующий префикс вместе со следующим смещением. 
// И так CountFrames раз.

// Находим промежуточные значения сумм размеров
sum=0;

// Начальное смещение всегда будет CountFrames*4
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

	///// Если значение оффсета слишком большое:
		if (OffsetValue>0xFFFF) { // Если Смещение больше 2-х байт, пишем 4 байта вместо 2-х. Но при этом префиксы уже недопустимы.
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

	// Пишем рассчитанный оффсет:
	temp=Swap2Bytes(OffsetValue);
	fwrite(&temp,2,1,OffsetsTmp);
SkipPref:
	if (OffsetValue>0xFFFF) { cout << ". Prefix is disabled!\n"; }
	}
}
// Эта метка для указания места, куда нужно переместиться после расчета таблицы оффсетов для вражин.
// Закрываем все.
Contin:

fclose(OffsetsTmp);
fclose(PrefixTmp);
}




void SpriteBegin(void)
{
int j, i;
// Генерируем имя и создаем выходной файл спрайта
//char *TextPtr;

CountFrames=0; // Обнуляем счетчик кадров.
OffarrayLine=0; // Обнуляем счетчик строк массива оффсетов вражин.
Enemy = FALSE; // Спрайт не вражины. Т.е., пока мы не знаем, какой он.
SkipFile = FALSE; // Этот флаг для отслеживания, какие файлы сохранять, а от каких брать только их размеры.
SKIP = FALSE; // Пока SkipFile флаг не использован.
GroupNumber = 0;
GroupCut = 9; // Только для того, чтобы параметр был положительным. В любом случае, он при необходимости принудительно назначается.
// Открываем временный файл. Туда будем писать все кадры спрайта.
// Потом берем его полностью.
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
// Обнулим OFFARRAY. Это нужно, если мы уже запаковывали спрайт врага раньше.
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

// Считаем количество файлов. От этого будет зависеть длина таблицы смещений.
cout << "\nFrames count = "<< CountFrames <<"\n";

// Закрываем файл с собранными кадрами и координатами.
fclose(tmpfp);
fclose(PrefixTmp);

// Собираем все вместе. Сначала формируем таблицу оффсетов.
// Если спрайт вражины, то формируем для него другую таблицу оффсетов.

if (Enemy==TRUE) {
	CalcOffsetsEnemy();
}	else {
	CalcOffsets();
	}

// Теперь нужно совместить файл Scratch и файл таблицы оффсетов.
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

// Читаем оффсеты и пишем в результирующий файл спрайта.
for (i=0; i<FileSize; i++) {
	fread(&Dta.Data,1,1,OffsetsTmp);
	tmp=Dta.Data;
	fwrite(&tmp,1,1,RezultFile);
}


// Читаем файл Scratch и пишем в результирующий файл спрайта.
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
// Пишем префикс в выходной файл.
fwrite(&Prefix,2,1,PrefixTmp);
}



/*----------------------------------------------------------------------------------------------------------*/
// При загрузке файла нужно отследить одинаковые имена файлов, но разные префиксы.
// В дальнейшем они не будут загружаться в Scratch, но будут присутствовать в таблице оффсетов
// с другим префиксом.

// Нужно ставить соответствие повторяющихся имен файлов, но различных префиксов в исходных данных.
// Для этих файлов, попавших в группы 8-ки таблица смещений после начальных 0х4000 формируется по-своему.

// Для этого каждое имя файла пишем в массив. И при загрузке следующего файла сканируем этот массив на наличие
// повтора. Затем проверяем префикс.
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
CountFrames++; // Плюсуем кадр. Нужно для определения длины таблицы смещений.

if (SkipFile == TRUE) { // Этот файл писать не нужно,
	goto SkipWriting;
}

//Пишем во временный файл координаты и кадры.
temp=Swap2Bytes(CoordX);
fwrite(&temp,2,1,tmpfp);
temp=Swap2Bytes(CoordY);
fwrite(&temp,2,1,tmpfp);


fseek(infile,0,SEEK_END);
FileSize = ftell(infile);
////*/cout << "Frame FileSize=" << FileSize << "\n";
rewind(infile);

// Запишем размер файла в массив размеров кадров.
OffsetsTable[CountFrames] = FileSize;
///CountFrames++; // Плюсуем кадр. Нужно для определения длины таблицы смещений.
//cout << "OffsetsTable[i]=" << OffsetsTable[CountFrames] << "\n";


// Читаем весь файл кадра и пишем его во временный файл.
for (i=0;i<FileSize;i++) {
	// Сначала определяем флаги CCB, они могут быть разными, поэтому значение флагов запихиваем в переменную CCBFlags.
#if 1
		if (OnlyOnce == TRUE) {
			fread(&Data.Data,4,1,infile);
			CCBFlags = Data.Data;
			cout << "CCBFlags=" << CCBFlags << "\n";
			rewind(infile);		// Перематываем файл на начало.
			OnlyOnce = FALSE; // Снимаем флаг, больше определять флаги не будем.
//			cin.get();
			}
#endif;
	fread(&Data.Data,1,1,infile);
	data=Data.Data;
	fwrite(&data,1,1,tmpfp);
	}

SkipWriting:
fclose(infile);
SkipFile = FALSE; // Этот флаг для отслеживания, какие файлы сохранять, а от каких брать только их размеры.
//fclose(tmpfp);
}



void AddOffset(void) {
//Функция для генерации собственной таблицы оффсетов для вражин.
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
	// При создании группы нужно учесть, что таблица оффсетов будет собираться не с учетом предыдущих групп,
	// а 
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
						// Если в скрипте была команда принудительно выключить пропуск файлов MIRRORED
						// Не будем пропускать файлы в новой группе.
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

// Чтобы включить отображение всех нужных строк в консоли, убрать комментарии ////*/
OnlyOnce = TRUE; // Для определения флагов CCB. 
// Читаем файл скрипта.
	ReadScript(fp);
	fclose(fp);     /* Close the file */


// Чистим:
remove("Scratch");
remove("PrefixTmp");
remove("OffsetsTmp");
remove("OffsetsTmpEnemy");
	
	cout << "\nPress the Enter key to continue ...";
    cin.get();
    return 0;
}