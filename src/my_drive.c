#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/disco.h"

/**
 * Função que mostra o menu
 * Recebe o título do menu, as opções do menu e um array
 * contendo as funções para cada opção
 * Se o retorno dessas funções for 0, voltamos ao menu, caso contrário
 * fechamos o menu.
 */
void menu_mostrar(char *titulo, char **opcoes, int (**funcoes)()){
	unsigned int i, opcao;
	int retorno;
	char leitura[maximoLeitura], erro = 0;
	while(1){
		system("clear"); i=0;
		
		printf("%s\n\n", titulo);
		while(opcoes[i][0] != '\0') {
			printf("%u - %s\n", i+1, opcoes[i]);
			i++;
		}
		
		if(erro == 1) printf("%sOpção inválida%s\n", COR_VERMELHO, COR_NORMAL);
		else if(erro == 2) printf("%sA função não faz nada%s\n", COR_VERMELHO, COR_NORMAL);
		
		fgets(leitura, maximoLeitura, stdin);
		if(sscanf(leitura, "%u", &opcao) && opcao > 0 && opcao <= i){
			erro=0;
			retorno = (*funcoes[opcao-1])();
			if(retorno == -1) erro = 2;
			else if(retorno != 0) return;
		}
		else erro=1;
	}
}

int menu_escreverArquivo(){
	char nomeArquivo[fileNameLength], leitura[2];
	FILE *arquivo;
	while(1){
		printf("\rDigite o nome do arquivo a ser transferido [* para voltar]: ");
		if(fgets(nomeArquivo, fileNameLength, stdin)!=NULL && *nomeArquivo != '\n'){
			if(nomeArquivo[0] == '*') return 0;
			strtok(nomeArquivo, "\n");
			printf("Abrindo: %s\n", nomeArquivo);
			arquivo = fopen(nomeArquivo, "r");
			if(arquivo == NULL) {
				printf("Arquivo inválido\n");
			}
			else {
				printf("********* INÍCIO DO ARQUIVO *********\n\n");
				while(!feof(arquivo)) printf("%c", fgetc(arquivo));
				printf("\n\n%s********** FIM DO ARQUIVO **********\n\n", COR_NORMAL);
				while(1){
					printf("\rÉ este o arquivo que quer inserir? [S/n] ");
					fgets(leitura, 2, stdin);
					if(leitura[0] == 's' || leitura[0] == 'S'){
						
						return;
					}
					else if(leitura[0] == 'n' || leitura[0] == 'N'){
						break;
					}
				}
			}
		}
	}
}

int menu_lerArquivo(){
	return -1;
}

int menu_apagarArquivo(){
	printf("ESSA FUNÇÃO NÃO FAZ NADA\n");
	return -1;
}

int menu_mostrarFAT(){
	return -1;
}

int menu_sair(){
	return -1;
}

/**
 * Opções do menu
 */
char *menu_opcoes[20] = {
	"Escrever Arquivo",
	"Ler Arquivo",
	"Apagar Arquivo", 
	"Mostrar Tabela FAT", 
	"Sair", 
	"\0"
};
/**
 * Funções do menu atreladas a cada opção
 */
int (*menu_funcoes[5])() = {
	menu_escreverArquivo,
	menu_lerArquivo,
	menu_apagarArquivo,
	menu_mostrarFAT,
	menu_sair
};

int main(){
	
	menu_mostrar("Menu Principal", menu_opcoes, menu_funcoes);
	
	return 0;
}
