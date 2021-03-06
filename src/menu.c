#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/menu.h"
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
		!modoTeste && system("clear"); 
		i=0;
		
		printf("%s\n\n", titulo);
		while(opcoes[i][0] != '\0') {
			printf("%u - %s\n", i+1, opcoes[i]);
			i++;
		}
		
		if(erro == 1) printf("%sOpção inválida%s", COR_VERMELHO, COR_NORMAL);
		else if(erro == 2) printf("%sA função não faz nada%s\n", COR_VERMELHO, COR_NORMAL);
		
		printf("\n>> ");
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
	char nomeArquivo[maximoLeitura], leitura[2];
	FILE *arquivo;
	while(1){
		printf("\rDigite o nome do arquivo a ser transferido [* para voltar]: ");
		if(fgets(nomeArquivo, maximoLeitura, stdin)!=NULL && *nomeArquivo != '\n'){
			if(nomeArquivo[0] == '*') return 0;
			strtok(nomeArquivo, "\n");
			printf("Abrindo: %s\n", nomeArquivo);
			arquivo = fopen(nomeArquivo, "r");
			if(arquivo == NULL) printf("Arquivo inválido\n");
			else {
				/*printf("********* INÍCIO DO ARQUIVO *********\n\n");
				while(!feof(arquivo)) printf("%c%s", fgetc(arquivo), COR_NORMAL);
				printf("\n\n%s********** FIM DO ARQUIVO **********\n\n", COR_NORMAL);*/
				while(1){
					printf("\rÉ este o arquivo que quer inserir? [S/n] ");
					fgets(leitura, 2, stdin);
					if(leitura[0] == 's' || leitura[0] == 'S'){
						disco_escreverArquivo(arquivo, nomeArquivo);
						fclose(arquivo);
						break;
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
	char nomeArquivo[fileNameLength];
	while(1){
		printf("Digite o nome do arquivo [* para voltar]: ");
		if(fgets(nomeArquivo, fileNameLength, stdin) != NULL && *nomeArquivo != '\n'){
			if(nomeArquivo[0] == '*') return 0;
			strtok(nomeArquivo, "\n");
			disco_lerArquivo(nomeArquivo);
		}
	}
}

int menu_apagarArquivo(){
	char nomeArquivo[fileNameLength];
	while(1){
		printf("Digite o nome do arquivo a ser removido [* para voltar]: ");
		if(fgets(nomeArquivo, fileNameLength, stdin) != NULL && *nomeArquivo != '\n'){
			if(nomeArquivo[0] == '*') return 0;
			strtok(nomeArquivo, "\n");
			disco_removerArquivo(nomeArquivo);
		}
	}
}

int menu_mostrarFAT(){
	char leitura[5];
	disco_mostrarFAT();
	fgets(leitura, 5, stdin);
	return 0;
}

int menu_sair(){
	printf("Tchau!\n");
	return 1;
}

/* Mostra o texto a esquerda da verbose */
int print_leftVerbose(const char *text, const char *cor){
	int size = strlen(text), padding = (leftVerboseSize-size)/2, rest = (leftVerboseSize-size)%2, i;
	if(size > leftVerboseSize) return 0;
	printf("[%s", cor);
	for(i=0;i<padding;i++) printf(" ");
	printf("%s", text);
	for(i=0;i<padding+rest;i++) printf(" ");
	printf("%s]  ", COR_NORMAL);
	return 1;
}
