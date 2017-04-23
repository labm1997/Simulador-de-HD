#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/disco.h"

track_array cylinders[numCylinders];

fatlist_s *fatlist = NULL;
fatent_s **fatent = NULL;

unsigned int disco_setoresOcupados = 0;

int iniciar_disco(){
	unsigned int i=0;
	
	fatent = (fatent_s **)malloc(sizeof(fatent_s *)*(numCylinders*tracksByCylinder*sectorByTrack));
	if(fatent == NULL) return 0;
	
	for(i=0;i<numCylinders*tracksByCylinder*sectorByTrack;i++) {
		fatent[i] = (fatent_s *)malloc(sizeof(fatent_s));
		fatent[i]->used = LIVRE;
		fatent[i]->eof = LIVRE;
		fatent[i]->next = LIVRE;
		if(fatent[i] == NULL) return 0;
	}
	return 1;
}

void disco_escreverArquivo(FILE *arquivo, char *nome){
	unsigned int clustersNecessarios, espaco = numCylinders*tracksByCylinder*sectorByTrack, i, j, contaSetores = 0;
	int primeiro = -1, anterior, arquivoTamanho;
	fatlist_s *novo, *tmp;
	
	fseek(arquivo, 0, SEEK_END);
	arquivoTamanho = ftell(arquivo);
	
	printf("Inserindo arquivo de %d bytes\n", arquivoTamanho);
	clustersNecessarios = arquivoTamanho/(blockSize*clusterSize);
	if(arquivoTamanho%(blockSize*clusterSize)) clustersNecessarios++;
	
	printf("Precisaremos de %u cluster(s)\n", clustersNecessarios);
	printf("%u/%u do disco ocupado\n", disco_setoresOcupados, espaco);
	
	if(disco_setoresOcupados+clustersNecessarios*clusterSize>espaco) {
		printf("Não há espaço no disco, o disco tem %u clusters disponíveis\n", (espaco-disco_setoresOcupados)/clusterSize);
		return;
	}
	
	rewind(arquivo);
	printf("Buscando setores vazios\n");
	
	for(i=0;i<espaco;i++){
		if(arquivoTamanho <= 0) {
			fatent[anterior]->next = NULO;
			fatent[anterior]->eof = ULTIMOSETOR;
			break;
		}
		
		if(fatent[i]->used == LIVRE) {
			/* Verificamos se o novo setor está na mesma trilha do setor anterior */
			if(contaSetores && i/sectorByTrack != (i-1)/sectorByTrack) contaSetores = 0;
			else contaSetores++;
		}
		else contaSetores = 0;
		
		if(contaSetores == clusterSize){
			/* Inserimos no cluster, setor a setor */
			for(j=0;j<clusterSize;j++){
				unsigned int setor = (i-(clusterSize-1-j));
				if(arquivoTamanho > 0) {
					fread(cylinders[setor%numCylinders].track[setor%tracksByCylinder].sector[setor%sectorByTrack].bytes_s, arquivoTamanho%blockSize+1, 1, arquivo);
				}
				
				arquivoTamanho-=arquivoTamanho%blockSize+1;
				
				fatent[setor]->used = OCUPADO;
				fatent[setor]->next = NULO;
				fatent[setor]->eof = NULO;
				disco_setoresOcupados++;
		
				if(primeiro == -1) primeiro = setor;
				else fatent[anterior]->next = setor;
				anterior = setor;
			}
			contaSetores = 0;
		}
	}
	
	printf("Atualizando FAT\n");
	if(fatlist == NULL){
		fatlist = (fatlist_s *)malloc(sizeof(fatlist_s));
		strcpy(fatlist->file_name, nome);
		fatlist->next = NULL;
		fatlist->first_sector = primeiro;
	}
	else {
		novo = (fatlist_s *)malloc(sizeof(fatlist_s));
		strcpy(novo->file_name, nome);
		novo->next = NULL;
		novo->first_sector = primeiro;
		
		tmp = fatlist;
		while(tmp->next != NULL) tmp = tmp->next;
		tmp->next = novo;
	}
	
	printf("%u/%u do disco ocupado\n", disco_setoresOcupados, espaco);
	printf("Arquivo salvo\n");
	fclose(arquivo);
	return;
}



void disco_mostrarFAT(){
	fatlist_s *tmp;
	fatent_s *tmp_file;
	unsigned int tamanho, maiorNome = 5;
	unsigned int *tmp_file_setores;
	tmp = fatlist;
	
	if(tmp != NULL) {
		do {
			if(strlen(tmp->file_name)>maiorNome) maiorNome = strlen(tmp->file_name);
			tmp = tmp->next;
		} while(tmp != NULL);
		
		printf("NOME:");
		for(int i=0;i<maiorNome-5;i++) printf(" ");
		printf("\tTAMANHO EM DISCO\tLOCALIZAÇÃO\n");
		
		tmp = fatlist;
		do {
			int j=0;
			tmp_file_setores = (unsigned int *)malloc(sizeof(unsigned int));
			tmp_file_setores[0] = tmp->first_sector;
			
			tmp_file = fatent[tmp->first_sector];
			
			while(tmp_file->eof != ULTIMOSETOR) {
				tmp_file_setores = (unsigned int *)realloc(tmp_file_setores, (++j+1)*sizeof(unsigned int));
				tmp_file_setores[j] = tmp_file->next;
				tmp_file = fatent[tmp_file->next];
			}
			
			printf("%s\t%10u bytes\t", tmp->file_name, blockSize*(j+1));
			int k=0;
			while(k<=j) {
				printf("%u", tmp_file_setores[k]);
				if(k++!=j) printf(",");
			}
			printf("\n");
			tmp = tmp->next;
		} while(tmp != NULL);
	}
	else printf("A tabela FAT está vazia\n");
}
