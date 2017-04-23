/**
 * Estruturas que simulam a parte física do disco
 */

#define blockSize 512
#define sectorByTrack 60
#define tracksByCylinder 5

typedef struct block{
	unsigned char bytes_s[blockSize];
} block;

typedef struct sector_array{
	block sector[sectorByTrack];
} sector_array;

typedef struct track_array{
	sector_array track[tracksByCylinder];
} track_array;

/**
 * Estruturas do sistema de arquivos usando a tabela FAT
 */

#define fileNameLength 100

typedef struct fatlist_s{
	char file_name[fileNameLength];
	unsigned int first_sector;
} fatlist;

typedef struct fatent_s{
	unsigned int used;
	unsigned int eof;
	unsigned int next;
} fatent;

/**
 * Protótipos das funções
 */

#define maximoLeitura 101
#define COR_VERMELHO	"\x1B[31m"
#define COR_NORMAL	"\x1B[0m"

void menu_mostrar(char *, char **, int (**funcoes)());
