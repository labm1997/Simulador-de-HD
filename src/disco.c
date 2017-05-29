#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/disco.h"

track_array cylinders[numCylinders];

fatlist_s *fatlist = NULL;
fatent_s **fatent = NULL;

unsigned int disco_setoresOcupados = 0;


/* 
  Obtem o índice do vetor de endereços a partir da coordenada no disco.
  x é o cilíndro
  z é o disco
  t é o setor na trilha
*/
unsigned int getIndex(unsigned int x, unsigned int z, unsigned int t){
	return x*tracksByCylinder*sectorByTrack+z*sectorByTrack+t;
}

int iniciar_disco(){
	unsigned int i=0, j[4], limites[4] = {numCylinders, tracksByCylinder, sectorByTrack, blockSize};
	
	fatent = (fatent_s **)malloc(sizeof(fatent_s *)*(numCylinders*tracksByCylinder*sectorByTrack));
	if(fatent == NULL) return 0;
	
	for(i=0;i<numCylinders*tracksByCylinder*sectorByTrack;i++) {
		fatent[i] = (fatent_s *)malloc(sizeof(fatent_s));
		fatent[i]->used = LIVRE;
		fatent[i]->eof = LIVRE;
		fatent[i]->next = LIVRE;
		if(fatent[i] == NULL) return 0;
	}
	
	/* Preenchemos o disco de 0 */
	for(j[0]=0;j[0]<numCylinders;j[0]++)
		for(j[1]=0;j[1]<tracksByCylinder;j[1]++)
			for(j[2]=0;j[2]<sectorByTrack;j[2]++)
				for(j[3]=0;j[3]<blockSize;j[3]++)
					cylinders[j[0]].track[j[1]].sector[j[2]].bytes_s[j[3]] = (char)0;
	
	
	return 1;
}

void disco_escreverArquivo(FILE *arquivo, char *nome){
	unsigned int clustersNecessarios, i, j, contaSetores = 0, posCabecaLeitura = 0, setor;
	int primeiro = -1, anterior, arquivoTamanho;
	fatlist_s *novo, *tmp;
	
	/* Contador de tempo de seek */
	double tempo = 0;
	
	/* 
	  Variáveis de coordenada do setor
	  x, cilíndro; z, disco e t setor na trilha
	*/
	unsigned int x = 0, z = 0, t = 0;
	
	/* Espaço total no disco */
	unsigned int espaco = numCylinders*tracksByCylinder*sectorByTrack;
	
	/* Verificamos se o nome já não existe na FAT */
	/*tmp = fatlist;
	if(tmp != NULL){
		do {
			if(strcmp(nome, tmp->file_name) == 0) {
				printf("Já há um arquivo com este nome\n");
				return;
			}
			tmp=tmp->next;
		} while(tmp != NULL);
	}*/
	
	/* Aqui obtemos o tamanho do arquivo a ser inserido */
	fseek(arquivo, 0, SEEK_END);
	arquivoTamanho = ftell(arquivo);
	
	printf("Inserindo arquivo de %d bytes\n", arquivoTamanho);
	
	/* Calculamos quantos clusters serão necessários para armazenar este arquivo */
	clustersNecessarios = arquivoTamanho/(blockSize*clusterSize);
	if(arquivoTamanho%(blockSize*clusterSize)) clustersNecessarios++;
	
	/* A partir do número de clusters, vemos se o arquivo cabe no disco */	
	if(disco_setoresOcupados+clustersNecessarios*clusterSize>espaco) {
		printf("Não há espaço no disco, o disco tem %u clusters disponíveis e precisa-se de %u\n", (espaco-disco_setoresOcupados)/clusterSize, clustersNecessarios);
		return;
	}
	
	/* Voltamos o arquivo a posição original para ser lido */
	rewind(arquivo);
	
	/* Percorremos o disco */
	printf("Gravando:\nTRILHA\tDISCO\tSETOR\n");
	while(arquivoTamanho > 0){
		i = getIndex(x,z,t);
		printf("%u\n", i);
		/* 
		  Caso o setor esteja livre, somamos ao contador de setores sequenciais,
		  serve para encontrar setores consecutivos e formar um cluster
		*/
		if(fatent[i]->used == LIVRE) {
			/* Verificamos se o novo setor está na mesma trilha do setor anterior */
			if(contaSetores && t+1 >= sectorByTrack) contaSetores = 0;
			else contaSetores++;
		}
		else contaSetores = 0;
		
		/*
		  Completamos um cluster, logo inserimos os dados nesse cluster
		*/
		if(contaSetores == clusterSize){
			
			/* Inserimos no cluster, setor a setor */
			for(j=0;j<clusterSize;j++){
				printf("%u\t%u\t%u\n", x, z, (t-(clusterSize-1-j)));
				/* Calculamos o setor a partir de sua coordenada */
				setor = (i-(clusterSize-1-j));
				
				if(arquivoTamanho > 0) {
					/* Gravamos na memória primária o setor em questão */
					fread(cylinders[x].track[z].sector[(t-(clusterSize-1-j))].bytes_s, (arquivoTamanho/blockSize)?blockSize:(arquivoTamanho%blockSize+1), 1, arquivo);
					//printf("GRAVADO NO SETOR %u:\n********\n%s\n********\n", setor, cylinders[x].track[z].sector[t].bytes_s);
				}
				
				/* Reduzimos o arquivoTamanho para o novo tamanho que resta a ser inserido */
				arquivoTamanho-=(arquivoTamanho/blockSize)?blockSize:(arquivoTamanho%blockSize+1);
				
				/* Atualiamos os endereços*/
				fatent[setor]->used = OCUPADO;
				fatent[setor]->next = NULO;
				fatent[setor]->eof = NULO;
				
				/* Variável de controle de espaço */
				disco_setoresOcupados++;
				
				/* 
				  Caso seja o primeiro setor, salvamos o valor do setor inicial,
				  será usado na construção da tabela FAT.
				*/
				
				if(primeiro == -1) primeiro = setor;
				else fatent[anterior]->next = setor;
				
				/* Trata-se do setor anterior, será usado na criação do EOF */
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
			
			/* Zeramos a contagem dos setores */
			contaSetores = 0;
			
			/* Não mudamos de cilíndro, apenas de trilha */
			z++;
			t++;
			
			/* Verificações das coordenadas */
		
			/* Se chegamos ao último disco, então voltamos ao primeiro e vamos ao próximo cilíndro */
			if(z >= tracksByCylinder) {
				z = 0;
				x++;
			}
		}
		
		/* Trilha cheia, ir para o próximo cilíndro */
		else if(primeiro == -1 && t+1 >= sectorByTrack){
			
			printf("%u %u %u\n", x, z, t);
			int tmp_pos = i+1-sectorByTrack*(tracksByCylinder*(numCylinders-1)+1);
			
			/* Verificamos pelo final de um disco, (fim de trilha e último cilíndro) */
			if(x == numCylinders-1 && t+1 >= sectorByTrack) {
				/* Pulamos um disco */
				z++;
				/* Zeramos o resto */
				x = 0; t = 0;
				//printf("Retirando de i: %u\n", sectorByTrack*(tracksByCylinder)*(numCylinders-1)-1);
			}
			else {
				/* Rebobinamos a posição na trilha */
				t = 0;
				/* Pulamos um cilíndro */
				x++;
			}
		}
		
		/* Andamos um setor */
		else t++;
		
		/* Rebobinamos t e vamos para o próximo cilíndro */
		if(t >= sectorByTrack){
			t = 0;
			x++;
		}
		
	
	}
	
	/* 
	  Caso já tenhamos inserido todo o arquivo no disco,
	  paramos e definimos o último setor como último setor 
	*/
	fatent[anterior]->next = NULO;
	fatent[anterior]->eof = ULTIMOSETOR;
	
	/* Atualizamos a tabela fat */
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
	unsigned int *tmp_file_setores, *new;
	tmp = fatlist;
	unsigned int warning = 0;
	
	if(tmp != NULL) {
		do {
			if(strlen(tmp->file_name)>maiorNome) maiorNome = strlen(tmp->file_name);
			tmp = tmp->next;
			if(warning++>1000000) exit(0);
		} while(tmp != NULL);
		
		printf("NOME:");
		for(int i=0;i<maiorNome-5;i++) printf(" ");
		printf("\tTAMANHO EM DISCO\tLOCALIZAÇÃO\n");
		
		tmp = fatlist;
		do {
			if(warning++>1000000) exit(0);
			int j=0;
			tmp_file_setores = (unsigned int *)malloc(sizeof(unsigned int));
			tmp_file_setores[0] = tmp->first_sector;
			
			tmp_file = fatent[tmp->first_sector];
			
			while(tmp_file->eof != ULTIMOSETOR) {
				if(warning++>1000000) exit(0);
				new = (unsigned int *)realloc(tmp_file_setores, (++j+1)*sizeof(unsigned int));
				if(new == NULL){
					printf("Problema ao realocar");
					exit(1);
				}
				else tmp_file_setores = new;
				tmp_file_setores[j] = tmp_file->next;
				tmp_file = fatent[tmp_file->next];
			}
			
			printf("%s\t%10u bytes\t", tmp->file_name, blockSize*(j+1));
			int k=0;
			while(k<=j) {
			if(warning++>1000000) exit(0);
				printf("%u", tmp_file_setores[k]);
				if(k++!=j) printf(",");
			}
			printf("\n");
			tmp = tmp->next;
			free(tmp_file_setores);
			tmp_file_setores = NULL;
		} while(tmp != NULL);
	}
	else printf("A tabela FAT está vazia\n");
}
