/**
 * Estruturas que simulam a parte física do disco
 */

#define blockSize 512 /* em bytes */
#define clusterSize 4 /* Em setores */
#define sectorByTrack 60
#define tracksByCylinder 5
#define numCylinders 10

#define seekTime 4 /* Em ms */
#define delayTime 6 /* Em ms */
#define transferTime 12 /* Em ms por trilha */

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

#define fileNameLength 101 /* Com '\0' */

typedef enum disco_const{
	LIVRE,
	OCUPADO,
	ULTIMOSETOR,
	NULO
} disco_const;

typedef struct fatlist_s{
	char file_name[fileNameLength];
	unsigned int first_sector;
	struct fatlist_s *next;
} fatlist_s;

typedef struct fatent_s{
	disco_const used;
	disco_const eof;
	unsigned int next;
} fatent_s;

typedef enum{
	ERRO_SEMESPACO,
	ERRO_LOOPINFINITO,
	SUCESSO,
	CLUSTER_LIVRE,
	CLUSTER_OCUPADO,
	ERRO_SEMMEMORIA
} discoRet;

typedef struct coordenadas{
	unsigned int x, z, t;
} coordenadas;

discoRet iniciar_disco();
void disco_escreverArquivo(FILE *, char *);
void disco_lerArquivo(char *);
void disco_removerArquivo(char *);
void disco_mostrarFAT();

unsigned int getIndex(coordenadas *);
void getCoord(unsigned int, coordenadas *);
discoRet clusterLivre(coordenadas *);
discoRet encontraCluster(coordenadas *);
unsigned int gravarCluster(FILE *, coordenadas *, int *, unsigned int *, unsigned int *);
double calculaTempo(unsigned int);
unsigned int calculaTamanhoSetor(coordenadas *);
