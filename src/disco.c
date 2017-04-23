#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/disco.h"

track_array cylinders[numCylinders];

fatlist_s *fatlist = NULL;
fatent_s **fatent = NULL;

unsigned int disco_clustersOcupados = 0;

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
	unsigned int clustersNecessarios, espaco = numCylinders*tracksByCylinder*sectorByTrack, i, j, setores[4] = {0,0,0,0}, contaSetores = 0;
	int primeiro = -1, anterior, arquivoTamanho;
	fatlist_s *novo, *tmp;
	
	fseek(arquivo, 0, SEEK_END);
	arquivoTamanho = ftell(arquivo);
	
	printf("Inserindo arquivo de %u bytes\n", arquivoTamanho);
	clustersNecessarios = arquivoTamanho/(blockSize*clusterSize);
	if(arquivoTamanho%(blockSize*clusterSize)) clustersNecessarios++;
	
	printf("Precisaremos de %u cluster(s)\n", clustersNecessarios);
	
	if(clusterSize*disco_clustersOcupados+clustersNecessarios>espaco) printf("Não há espaço no disco\n");
	
	rewind(arquivo);
	printf("Buscando setores vazios\n");
	
	for(i=0;i<espaco;i++){
		if(arquivoTamanho <= 0) {
			fatent[anterior]->next == NULO;
			fatent[anterior]->eof == ULTIMOSETOR;
			break;
		}
		
		if(fatent[i]->used == LIVRE) {
			/* Verificamos se o novo setor está na mesma trilha do setor anterior */
			if(contaSetores && i/sectorByTrack != (i-1)/sectorByTrack) contaSetores = 0;
			setores[contaSetores++] = i;
		}
		else contaSetores = 0;
		
		if(contaSetores == clusterSize){
			printf("Inserindo arquivo no cluster: ");
			/* Inserimos no cluster, setor a setor */
			for(j=0;j<clusterSize;j++){
				printf("%u ", setores[j]);
				if(arquivoTamanho > 0) {
					fread(cylinders[setores[j]%numCylinders].track[setores[j]%tracksByCylinder].sector[setores[j]%sectorByTrack].bytes_s, arquivoTamanho%blockSize+1, 1, arquivo);
				}
				
				arquivoTamanho-=arquivoTamanho%blockSize+1;
				
				fatent[setores[j]]->used == OCUPADO;
				fatent[setores[j]]->next == NULO;
				fatent[setores[j]]->eof == NULO;
				disco_clustersOcupados++;
		
				if(primeiro == -1) primeiro = setores[j];
				else fatent[anterior]->next == setores[j];
				anterior = setores[j];
			}
			contaSetores = 0;
			printf("\n");
		}
	}
	
	printf("Atualizando FAT\n");
	if(fatlist == NULL){
		fatlist = (fatlist_s *)malloc(sizeof(fatlist));
		strcpy(fatlist->file_name, nome);
		fatlist->next = NULL;
		fatlist->first_sector = primeiro;
	}
	else {
		novo = (fatlist_s *)malloc(sizeof(fatlist));
		strcpy(novo->file_name, nome);
		novo->next = NULL;
		novo->first_sector = primeiro;
		
		tmp = fatlist;
		while(tmp->next != NULL) tmp = tmp->next;
		tmp->next = novo;
	}
	
	printf("Arquivo salvo\n");
	return;
}
