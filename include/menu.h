/**
 * Protótipos das funções
 */

#define maximoLeitura 101
#define COR_VERMELHO	"\x1B[31m"
#define COR_NORMAL	"\x1B[0m"

void menu_mostrar(char *, char **, int (**)());
int menu_escreverArquivo();
int menu_lerArquivo();
int menu_apagarArquivo();
int menu_mostrarFAT();
int menu_sair();

/* Variável de controle */
char menuApagar;

