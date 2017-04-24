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
	unsigned int clustersNecessarios, espaco = numCylinders*tracksByCylinder*sectorByTrack, i, j, contaSetores = 0, posCabecaLeitura = 0;
	double tempo = 0;
	int primeiro = -1, anterior, arquivoTamanho;
	fatlist_s *novo, *tmp;
	
	/* Verificamos se o nome já não existe na FAT */
	tmp = fatlist;
	if(tmp != NULL){
		do {
			if(strcmp(nome, tmp->file_name) == 0) {
				printf("Já há um arquivo com este nome\n");
				return;
			}
			tmp=tmp->next;
		} while(tmp != NULL);
	}
	
	fseek(arquivo, 0, SEEK_END);
	arquivoTamanho = ftell(arquivo);
	
	printf("Inserindo arquivo de %d bytes\n", arquivoTamanho);
	clustersNecessarios = arquivoTamanho/(blockSize*clusterSize);
	if(arquivoTamanho%(blockSize*clusterSize)) clustersNecessarios++;
	
	//printf("Precisaremos de %u cluster(s)\n", clustersNecessarios);
	//printf("%u/%u do disco ocupado\n", disco_setoresOcupados, espaco);
	
	if(disco_setoresOcupados+clustersNecessarios*clusterSize>espaco) {
		printf("Não há espaço no disco, o disco tem %u clusters disponíveis e precisa-se de %u\n", (espaco-disco_setoresOcupados)/clusterSize, clustersNecessarios);
		return;
	}
	
	rewind(arquivo);
	//printf("Buscando setores vazios\n");
	
	for(i=0;i<espaco;){
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
					fread(cylinders[(setor/sectorByTrack/tracksByCylinder)%numCylinders].track[(setor/sectorByTrack)%tracksByCylinder].sector[setor%sectorByTrack].bytes_s, (arquivoTamanho/blockSize)?blockSize:(arquivoTamanho%blockSize+1), 1, arquivo);
					printf("GRAVADO NO SETOR %u:\n********\n%s\n********\n", setor, cylinders[(setor/sectorByTrack/tracksByCylinder)%numCylinders].track[(setor/sectorByTrack)%tracksByCylinder].sector[setor%sectorByTrack].bytes_s);
				}
				
				arquivoTamanho-=(arquivoTamanho/blockSize)?blockSize:(arquivoTamanho%blockSize+1);
				
				fatent[setor]->used = OCUPADO;
				fatent[setor]->next = NULO;
				fatent[setor]->eof = NULO;
				disco_setoresOcupados++;
		
				if(primeiro == -1) primeiro = setor;
				else fatent[anterior]->next = setor;
				anterior = setor;
				
				/* Contamos o tempo */
				/* Seek */
				if(setor/(sectorByTrack*tracksByCylinder) != posCabecaLeitura/(sectorByTrack*tracksByCylinder))	{
					tempo += seekTime; /* Somamos o tempo médio de seek */
				}
				/* Atualizamos a posição da cabeça de leitura e escrita */
				posCabecaLeitura = setor;
			}
			/* Latência */
			tempo += delayTime; /* Somamos o tempo médio de latência */
			/* Tempo de transferência */
			tempo += clusterSize*(double)transferTime/(sectorByTrack);
			
			contaSetores = 0;
			
			/* Não mudamos de cilíndro, apenas de trilha */
			i+=sectorByTrack-clusterSize+1;
		}
		/* Trilha cheia, ir para o próximo cilíndro */
		else if(primeiro == -1){
			if((i+1)%sectorByTrack == 0) {
				if((i+1)%(sectorByTrack*tracksByCylinder*numCylinders)) i-=sectorByTrack*(tracksByCylinder-1)*numCylinders+1;
				else i+=(sectorByTrack-1)*tracksByCylinder+1;
			}
			else i++;
		}
		else {
			i++;
		}
	}
	
	//printf("Atualizando FAT\n");
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
	printf("Arquivo salvo em %.2lf ms\n", tempo);
	fclose(arquivo);
	return;
}

void disco_lerArquivo(char *nomeArquivo){
	fatlist_s *tmp = fatlist;
	fatent_s *tmp_file;
	FILE *saida;
	if(tmp != NULL){
		do {
			if(strcmp(nomeArquivo, tmp->file_name) == 0) {
				saida = fopen("SAIDA.TXT", "w");
				
				tmp_file = fatent[tmp->first_sector];
				fwrite(cylinders[(tmp->first_sector/sectorByTrack/tracksByCylinder)%numCylinders].track[(tmp->first_sector/sectorByTrack)%tracksByCylinder].sector[tmp->first_sector%sectorByTrack].bytes_s, blockSize, 1, saida);
			
				while(tmp_file->eof != ULTIMOSETOR) {
					fwrite(cylinders[(tmp_file->next/sectorByTrack/tracksByCylinder)%numCylinders].track[(tmp_file->next/sectorByTrack)%tracksByCylinder].sector[tmp_file->next%sectorByTrack].bytes_s, blockSize, 1, saida);
					tmp_file = fatent[tmp_file->next];
				}
				fclose(saida);
				return;
			}
			tmp = tmp->next;
		} while(tmp != NULL);
	}
	printf("Arquivo inválido\n");
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
