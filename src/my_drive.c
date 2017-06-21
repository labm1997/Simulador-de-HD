#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/menu.h"
#include "../include/disco.h"

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

int main(int argc, char **argv){
	if(iniciar_disco() != SUCESSO){
		printf("Não há memória para o disco\n");
		return 0;
	}
	int i=0, limit;
	char fileName[fileNameLength];
	FILE *arquivo;
	
	/* Teste de estresse, carrega o mesmo arquivo muitas vezes */
	if(argc == 3){
		sscanf(argv[2], "%d", &limit);
		strncpy(fileName, argv[1], fileNameLength-1);
		printf("%s\n", fileName);
		arquivo = fopen(fileName, "r");
		if(arquivo != NULL) {
			modoTeste = 1;
			for(;i<limit;i++) {
				disco_escreverArquivo(arquivo, fileName);
				rewind(arquivo);
			}
			fclose(arquivo);			
		}
		else printf("Não foi possível realizar o teste, arquivo inválido\n");
	}
	else modoTeste = 0;
	
	menu_mostrar("Menu Principal", menu_opcoes, menu_funcoes);
	
	return 0;
}
