#include "header.h"


#define MAKELIST 0
#define MAKETREE 1
#define NODE -1

typedef unsigned long long int UINT64;

struct _stat64 size;


typedef struct Tree
{
	int symbol;
	UINT64 count;
	struct Tree* next, * left, * right;
} Tree;


void deleteNode(Tree* root) {
	if (root) {
		deleteNode(root->left);
		deleteNode(root->right);
		free(root);
	}
}

/*функция подсчёта частоты встречаемости для каждого символа*/
void createFrequencyArr(FILE* file, UINT64* arr, UINT64 size)
{
	for (UINT64 i = 0; i < size; i++)
	{
		arr[fgetc(file)]++;
	}
	return;
}


void insert(Tree** head, UINT64 count, int symbol, char mode)
{
	Tree* tmp = (Tree*)malloc(sizeof(Tree));
	Tree* cur = NULL;
	int flag = 0;
	if (mode == 0)//добавление в список
	{
		//если список пуст
		if (*head == NULL)
		{
			tmp->left = tmp->right = NULL;
			tmp->symbol = symbol;
			tmp->count = count;
			tmp->next = *head;
			*head = tmp;
			return;
		}
		if (count <= (*head)->count) /* вставить перед первым */
		{
			tmp->count = count;
			tmp->symbol = symbol;
			tmp->left = tmp->right = NULL;
			tmp->next = (*head);
			*head = tmp;
		}
		else
		{
			cur = (*head);
			/* ищем позицию для вставки */
			while ((flag == 0) && (cur->next != NULL))
			{
				if ((cur->count < count) && (cur->next->count >= count)) flag = 1;
				cur = cur->next;
			}
			if (flag == 0) /* позиция не найдена - вставить в конец */
			{
				tmp->count = count;
				tmp->symbol = symbol;
				tmp->left = tmp->right = NULL;
				tmp->next = NULL;
				cur->next = tmp;
			}
			else /* позиция в середине найдена */
			{
				tmp->count = count;
				tmp->symbol = symbol;
				tmp->left = tmp->right = NULL;
				tmp->next = cur->next;
				cur->next = tmp;
			}
		}
	}

	else //добавление в дерево (==1)
	{
		if (count <= (*head)->count) /* вставить перед первым */
		{
			tmp->count = (*head)->count + (*head)->next->count;
			tmp->symbol = NODE;
			tmp->left = *head;
			tmp->right = (*head)->next;
			tmp->next = (*head);
			*head = tmp;
		}
		else
		{
			cur = (*head);
			/* ищем позицию для вставки */
			while ((flag == 0) && (cur->next != NULL))
			{
				if ((cur->count < count) && (cur->next->count >= count)) flag = 1;
				cur = cur->next;
			}
			if (flag == 0) /* позиция не найдена - вставить в конец */
			{
				tmp->count = (*head)->count + (*head)->next->count;
				tmp->symbol = NODE;
				tmp->left = *head;
				tmp->right = (*head)->next;
				tmp->next = NULL;
				cur->next = tmp;
				(*head) = (*head)->next->next;
			}
			else /* позиция в середине найдена */
			{
				tmp->count = (*head)->count + (*head)->next->count;
				tmp->symbol = NODE;
				tmp->left = *head;
				tmp->right = (*head)->next;
				tmp->next = cur->next;
				cur->next = tmp;
				(*head) = (*head)->next->next;
			}
		}
	}
}


/*Создание дерева Хаффмана*/
void makeHuffmanTree(Tree** head)
{
	UINT64 count;
	while ((*head)->next)
	{
		count = (*head)->count + (*head)->next->count;
		insert(head, count, NODE, MAKETREE);
	}
}


/*Построение неравномерного кода с помощью дерева Хаффмана*/
void CodeTable(Tree* root, char codes[256][256], char vrm[256])
{
	char tmp[256];
	int i = 0;
	strcpy(tmp, vrm);

	if (root->symbol >= 0)
	{
		strcpy(codes[root->symbol], tmp);
		return;
	}
	if (root->left)
		CodeTable(root->left, codes, strcat(tmp, "0"));
	while (tmp[i])
		i++;

	tmp[--i] = '\0';
	if (root->right)
		CodeTable(root->right, codes, strcat(tmp, "1"));
}



/* Это самая трешовая функция найденная на просторах стэковерфлоу,
	но она рабоатет*/
void CharToString(char* SymBuf, char c)
{
	for (int i = 0; i < 8; i++)
	{
		SymBuf[i] += !!(c & (1 << (7 - i)));
	}
}

/*
Функция записи бит
pos - позиция в буфере(0 до 7)
buffer - накапливаемый до байта набор битов
value - массив символов, который мы добавляем
*/

UINT64 writeBits(FILE* file, int* position, unsigned char* buffer, char* value)
{
	int written = 0;//для подсчёта размера (в байтах)
	int bit;
	while (*value)
	{
		bit = *value - '0';
		if (bit)
		{
			// little-endian (7 - )
			*buffer = (*buffer | 1 << (7 - (*position)));
		}
		(*position)++;
		if ((*position) == 8)
		{
			fwrite(buffer, sizeof(char), 1, file);
			written++;
			(*position) = 0;
			*buffer = 0;
		}
		value++;
	}
	return written;
}

/*Функция записи дерева в файл
0-если узел, 1 - если лист, за 1 всегда следует код символа
*/

void WriteTree(Tree* root, unsigned char* buffer, int* position, FILE* outputFile)
{
	char SymBuf[] = { "00000000" };

	if (root->symbol == -1)
	{
		writeBits(outputFile, position, buffer, "0");
		WriteTree(root->left, buffer, position, outputFile);
		WriteTree(root->right, buffer, position, outputFile);
		return;
	}

	writeBits(outputFile, position, buffer, "1");
	CharToString(SymBuf, (char)(root->symbol));
	writeBits(outputFile, position, buffer, SymBuf);
}

/*
void writeHeader(char* name, UINT64 size) {
}*/

/*Кодирование содержимого файла*/
UINT64 writeData(char codes[256][256], int* position, unsigned char* buffer, FILE* inputFile, FILE* outputFile, UINT64 size)
{
	int c;
	UINT64 writtenData = 0;
	UINT64 count = 0;
	while (count != size) {

		c = fgetc(inputFile);
		writtenData += writeBits(outputFile, position, buffer, codes[c]);
		count++;
	}
	return writtenData;
}

char read_bit(FILE* in)
{
	static unsigned char buf = 0, counter = 0;
	if (!counter)
	{
		fread(&buf, sizeof(char), 1, in);
	}
	counter++;
	int bit = buf >> 7;
	buf <<= 1;
	if (counter == 8)
		counter = 0;
	return bit;
}

unsigned char read_char(FILE* in)
{
	unsigned char c = 0;
	for (int i = 0; i < sizeof(char) * 8; i++)
	{
		c <<= 1;
		c |= read_bit(in);
	}
	return c;
}

/*Восстановление дерева из файла*/
Tree* createNode(FILE* inputFile)
{
	unsigned char c;
	Tree* tmp = (Tree*)malloc(sizeof(Tree));
	char bit = read_bit(inputFile);
	if (bit == 0)
	{
		tmp->symbol = -1;
		tmp->left = createNode(inputFile);
		tmp->right = createNode(inputFile);
		return tmp;
	}
	if (bit == 1)
	{
		c = read_char(inputFile);
		tmp->symbol = c;
		tmp->left = tmp->right = NULL;
		return tmp;
	}

}


void printTree(Tree* root) {
	if (root == NULL)
	{
		//printf("Tree is empty");
		return;
	}
	//printf("%d ", root->count);
	if (root->symbol < 0) printf("%d\n", root->symbol);
	else printf("%c\n", root->symbol);
	printTree(root->left);
	printTree(root->right);
}


void encode(FILE* inputFile, FILE* outputFile, UINT64 fileSize)
{
	UINT64 arr[256] = { 0 },//массив для хранения count
		placeBeforeTree = 0,
		writtenData = 0;


	Tree* head = NULL;

	char vrm[256] = { '\0' };
	char codes[256][256] = { '\0' };

	int pos = 0;
	unsigned char bufferTmp = 0; // Буфер в который будем писать
	char posInWRTree;

	createFrequencyArr(inputFile, arr, size.st_size);

	/*Вставка элементов в список*/
	for (int i = 0; i < 256; i++)
	{
		if (arr[i] != 0)
		{
			insert(&head, arr[i], i, MAKELIST);
		}
	}

	makeHuffmanTree(&head);


	CodeTable(head, codes, vrm);// Построение кодов для символов

	//сдвиг для записи размера закодированной части (без дерева и чего-либо)
	placeBeforeTree = _ftelli64_nolock(outputFile);
	_fseeki64_nolock(outputFile, sizeof(UINT64), SEEK_SET);

	// Записываем дерево
	WriteTree(head, &bufferTmp, &pos, outputFile);
	_fseeki64_nolock(inputFile, 0, SEEK_SET);//сдвиг в начало инпут файла для кодирования

	posInWRTree = pos;//запоминаем позицию в буфере при записи дерева в файл
	writtenData = writeData(codes, &pos, &bufferTmp, inputFile, outputFile, size.st_size);
	writtenData *= 8;

	if (pos != 0)//дозапись последнего байта
	{
		fwrite(&bufferTmp, sizeof(char), 1, outputFile);
		bufferTmp = 0;
		writtenData += pos;
		pos = 0;
	}
	writtenData -= posInWRTree;

	_fseeki64_nolock(outputFile, placeBeforeTree, SEEK_SET);

	//запись размера закодированной части
	fwrite(&writtenData, sizeof(UINT64), 1, outputFile);
	fcloseall;
	deleteNode(head);
}


void decode(FILE* inputFile, FILE* outputFile)
{
	UINT64 dataSize = 0;

	fread(&dataSize, sizeof(UINT64), 1, inputFile);
	Tree* root = createNode(inputFile);
	Tree* tmp = root;
	unsigned char c;

	/*Декодирование с помощью прохода по дереву
	 (чаще всего именно это используется)*/
	for (UINT64 i = 0; i < dataSize; i++)
	{
		c = read_bit(inputFile);
		if (c == 0)
		{
			tmp = tmp->left;
		}
		else
		{
			tmp = tmp->right;
		}
		if (tmp->symbol != -1)
		{
			fprintf(outputFile, "%c", tmp->symbol);
			tmp = root;
		}
	}
	deleteNode(root); root = NULL;
	fcloseall;
}

/* Да это unsafe, можно по факту любой файл засунуть через например .jpg.arc ,
	но просто как формальность из ТЗ*/
int rightFileFormat(char* file)
{
	int n = strlen(file);
	if (file[n] != 'c' && file[n - 1] != 'r' && file[n - 2] != 'a')
		return 0;
	return 1;
}


int main(int argc, char **argv)
{	
	char* locale = setlocale(LC_ALL, "");
	
	
	FILE *inputFile, *outputFile;
	

	if ((!strcmp(argv[1], "--file")) && (!strcmp(argv[3], "--create"))) {
		_stat64(argv[2], &size);

		if ( (!(inputFile = fopen(argv[2], "rb"))) && (!rightFileFormat(argv[2])))
			OPEN_ERR
		if ((!(outputFile = fopen(argv[4], "wb"))))
			CREATE_FILE_ERR

		if (size.st_size != 0)
			encode(inputFile, outputFile, size.st_size);
		else 
			printf("[ERORR]:Файл пуст");
	}

	if ((!strcmp(argv[1], "--file")) && (!strcmp(argv[3], "--extract")))
	{
		if (!(inputFile = fopen(argv[2], "rb")))
			OPEN_ERR
			if (!(outputFile = fopen(argv[4], "wb")))
				CREATE_FILE_ERR
				decode(inputFile, outputFile);
	}
	
	
	
}