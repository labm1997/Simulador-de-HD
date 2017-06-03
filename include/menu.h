/**
 * Protótipos das funções
 */

#define maximoLeitura 101
#define COR_VERMELHO	"\x1B[31m"
#define COR_NORMAL	"\x1B[0m"
#define COR_VERDE	"\x1B[32m"
#define COR_AMARELO	"\x1B[33m"
#define leftVerboseSize 7

void menu_mostrar(char *, char **, int (**)());
int menu_escreverArquivo();
int menu_lerArquivo();
int menu_apagarArquivo();
int menu_mostrarFAT();
int menu_sair();

int print_leftVerbose(const char *, const char *);

/* Variável de controle */
char modoTeste;

