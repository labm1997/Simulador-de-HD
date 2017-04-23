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

int main(){
	if(iniciar_disco() == 0){
		printf("Não há memória para o disco\n");
		return 0;
	}
	menu_mostrar("Menu Principal", menu_opcoes, menu_funcoes);
	
	return 0;
}
