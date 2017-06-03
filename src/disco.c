#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../include/disco.h"
#include "../include/menu.h"

/* Alocação do HD */
track_array *cylinders;

fatlist_s *fatlist = NULL;
fatent_s **fatent = NULL;

/* Espaço total no disco */
unsigned int espaco = numCylinders*tracksByCylinder*sectorByTrack;

/* Setores ocupados do disco */
unsigned int disco_setoresOcupados = 0;

/* Posição da cabeça de leitura e escrita */
coordenadas posCabecaLeitura = {.x = 0, .z = 0, .t = 0};

/* 
  Obtem o índice do vetor de endereços a partir da coordenada no disco.
  x é o cilíndro
  z é o disco
  t é o setor na trilha
*/
unsigned int getIndex(coordenadas *c){
	return c->x*tracksByCylinder*sectorByTrack+c->z*sectorByTrack+c->t;
}

/*
  Converte o valor do índice para as coordenadas do disco
*/
void getCoord(unsigned int index, coordenadas *c){
	c->x = (index/sectorByTrack/tracksByCylinder)%numCylinders;
	c->z = (index/sectorByTrack)%tracksByCylinder;
	c->t = index%sectorByTrack;
}

/* 
  Inicia o disco, aloca memória para os endereços dos setores e preenche o disco com 0 
*/
discoRet iniciar_disco(){
	unsigned int i=0, j[4], limites[4] = {numCylinders, tracksByCylinder, sectorByTrack, blockSize};
	
	fatent = (fatent_s **)malloc(sizeof(fatent_s *)*(espaco));
	if(fatent == NULL) return ERRO_SEMMEMORIA;
	
	for(i=0;i<espaco;i++) {
		fatent[i] = (fatent_s *)malloc(sizeof(fatent_s));
		fatent[i]->used = LIVRE;
		fatent[i]->eof = LIVRE;
		fatent[i]->next = LIVRE;
		if(fatent[i] == NULL) return ERRO_SEMMEMORIA;
	}
	
	/* Criamos o disco por alocação dinâmica */
	cylinders = (track_array *)malloc(numCylinders*sizeof(track_array));
	
	if(cylinders == NULL) return ERRO_SEMMEMORIA;
		
	return SUCESSO;
}

/*
  Verifica se o cluster passado está livre
*/
discoRet clusterLivre(coordenadas *c){
	unsigned int contador = 0;
	coordenadas C = {.x = c->x, .z = c->z, .t = c->t};
	while(fatent[getIndex(&C)]->used == LIVRE){
		if(C.t >= sectorByTrack) return CLUSTER_OCUPADO;
		C.t++;
		contador++;
		if(contador == clusterSize) return CLUSTER_LIVRE;
	}
	return  CLUSTER_OCUPADO;
}

/*
  Encontra o primeiro cluster disponível segundo o percorrimento da especificação
  recebe a posição (x,z,t) e retorna na própria variável os valores para o cluster achado
*/

discoRet encontraCluster(coordenadas *c){
	unsigned int T[2] = {c->z, c->t}, X[3] = {c->x, c->z, c->t};
	
	/* Verifica se há setores */
	if(disco_setoresOcupados >= espaco) return ERRO_SEMESPACO;
	
	while(1){
			
		if(clusterLivre(c) == CLUSTER_LIVRE){
			return SUCESSO;
		}
		else {
			/* Pulamos trilha */
			c->z++;
			
			/* Overflow de z */
			if(c->z >= tracksByCylinder){
				c->z = 0; /* Voltamos ao disco no topo */
				c->t += clusterSize; /* Movemos um cluster */
				/* Overflow de t */
				if(c->t >= sectorByTrack) c->t = 0;
			}
		
			/* Uma volta foi dada no cilíndro, vamos ao próximo */
			if(T[0] == c->z && T[1] == c->t) {
				c->x++;
				c->t = 0;
				c->z = 0;
				T[0] = T[1] = 0;
			}
			
			/* Chega-se ao fim dos cilíndros, voltamos ao primeiro */
			if(c->x >= numCylinders) {
				c->x = 0;
				c->t = 0;
				c->z = 0;
			}
			
		}	
	}
	return SUCESSO;
}

/* 
  Grava dados no cluster, precisa do tamanho do arquivo para alterá-lo
  retorna o setor inicial do cluster gravado
*/

unsigned int gravarCluster(FILE *arquivo, coordenadas *c, int *arquivoTamanho, unsigned int *primeiro, unsigned int *anterior){
	unsigned int j = 0, setor, i = getIndex(c), k, blocodegravacao;
	/* Grava-se setor a setor */
	for(;j<clusterSize;j++){		
		/* Calculamos o setor a partir de sua coordenada */
		setor = (i+j);
		
		/* Gravamos na memória primária o setor em questão */
		blocodegravacao = (*arquivoTamanho/blockSize)?blockSize:(*arquivoTamanho%blockSize+1);
		for(k=0;k<blockSize;k++) cylinders[c->x].track[c->z].sector[(c->t+j)].bytes_s[k] = (char)0; /* O setor deve ser vazio inicialmente */
		fread(cylinders[c->x].track[c->z].sector[(c->t+j)].bytes_s, blocodegravacao, 1, arquivo);
		
		/* Reduzimos o arquivoTamanho para o novo tamanho que resta a ser inserido */
		*arquivoTamanho-=blocodegravacao;
	
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
	
		if(*primeiro == -1) *primeiro = setor;
		else fatent[*anterior]->next = setor;
	
		/* Trata-se do setor anterior, será usado na criação do EOF */
		*anterior = setor;
	}
	return i;
}

/*
  Calcula o tempo de seek + transferência da gravação e movimentação da cabeça e leitura
*/
double calculaTempo(unsigned int cluster){
	double tempo = 0;
	coordenadas c;
	
	getCoord(cluster, &c);
	
	/* Contamos o tempo */
	/* Seek */
	if(c.x != posCabecaLeitura.x){
		tempo += seekTime; /* Somamos o tempo médio de seek */
		print_leftVerbose("I/O", COR_AMARELO);
		printf("Mudando de cilíndro! (%u %u %u) -> (%u -- --)\n", c.x, c.z, c.t, posCabecaLeitura.x);
	}
		
	/* Latência, ocorre se a leitura não for sequencial */
	if(abs(c.t-posCabecaLeitura.t) > 2*clusterSize){
		tempo += delayTime; /* Somamos o tempo médio de latência */
		print_leftVerbose("I/O", COR_AMARELO);
		printf("Leitura/gravação não sequencial! (%u %u %u) -> (-- -- %u)\n", c.x, c.z, c.t, posCabecaLeitura.t);
	}
	
	/* Atualizamos a posição da cabeça de leitura e escrita */
	posCabecaLeitura.x = c.x;
	posCabecaLeitura.t = c.t;
		
	/* Tempo de transferência de um cluster */
	tempo += clusterSize*(double)transferTime/(sectorByTrack);
	return tempo;
}

/*
  Mede o quanto realmente um cluster foi ocupado
*/
unsigned int calculaTamanhoSetor(coordenadas *c){
	int j=blockSize-1, tamanho=blockSize;
	
	/* Byte a byte, começando do último */
	for(;j>=0;j--){
		if(cylinders[c->x].track[c->z].sector[c->t].bytes_s[j] == 0) tamanho--;
		else return tamanho;
	}
	
	return 0;
}

/*
  Função que escreve arquivo no disco
*/

void disco_escreverArquivo(FILE *arquivo, char *nome){
	unsigned int clustersNecessarios, i, j, contaSetores = 0, setor;
	int primeiro = -1, anterior, arquivoTamanho;
	fatlist_s *novo, *tmp;
	
	/* Contador de tempo de seek */
	double tempo = 0;
	
	/* 
	  Variáveis de coordenada do setor
	  x, cilíndro; z, disco e t setor na trilha
	*/
	coordenadas c = {.x = 0, .z = 0, .t = 0};
		
	/* Verificamos se o nome já não existe na FAT (caso esteja no modo de teste isso é ignorado) */
	tmp = fatlist;
	if(tmp != NULL && !modoTeste){
		do {
			if(strcmp(nome, tmp->file_name) == 0) {
				print_leftVerbose("FALHA", COR_VERMELHO);
				printf("Já há um arquivo com este nome\n");
				return;
			}
			tmp=tmp->next;
		} while(tmp != NULL);
	}
	
	/* Aqui obtemos o tamanho do arquivo a ser inserido */
	fseek(arquivo, 0, SEEK_END);
	arquivoTamanho = ftell(arquivo);
	
	print_leftVerbose("OK!", COR_VERDE);
	printf("Inserindo arquivo de %d bytes\n", arquivoTamanho);
	
	/* Calculamos quantos clusters serão necessários para armazenar este arquivo */
	clustersNecessarios = arquivoTamanho/(blockSize*clusterSize);
	if(arquivoTamanho%(blockSize*clusterSize)) clustersNecessarios++;
	print_leftVerbose("OK!", COR_VERDE);
	printf("Serão usados %d clusters\n", clustersNecessarios);
	
	/* A partir do número de clusters, vemos se o arquivo cabe no disco */	
	if(disco_setoresOcupados+clustersNecessarios*clusterSize>espaco) {
		print_leftVerbose("FALHA", COR_VERMELHO);
		printf("Não há espaço no disco, o disco tem %u clusters disponíveis e precisa-se de %u\n", (espaco-disco_setoresOcupados)/clusterSize, clustersNecessarios);
		return;
	}
	
	/* Voltamos o arquivo a posição original para ser lido */
	rewind(arquivo);
	
	/* Percorremos para achar o primeiro cluster */
	while(primeiro == -1){
		i = getIndex(&c);
		/* Setor livre */
		if(fatent[i]->used == LIVRE) {
			/* Verificamos se o novo setor está na mesma trilha do setor anterior */
			if(contaSetores && c.t >= sectorByTrack) {
				contaSetores = 0;
			}
			else contaSetores++;
		
		}
		
		/* Se contamos o número certo de setores para um cluster, inserimos nele */
		if(contaSetores == clusterSize){
			c.t -= clusterSize-1;
			
			/* Gravamos no cluster */
			setor = gravarCluster(arquivo, &c, &arquivoTamanho, &primeiro, &anterior);
			
			/* Contamos o tempo */
			tempo += calculaTempo(setor);
		
			/* Zeramos a contagem dos setores */
			contaSetores = 0;
		
			/* Não mudamos de cilíndro, apenas de trilha, possivelmente problemático */
			c.z++;
			c.t+=clusterSize-1;
		}
		
		/* Andamos um setor */
		c.t++;
	
		/* Fim da trilha, vamos ao próximo cilíndro */
		if(c.t >= sectorByTrack){
			c.x++; c.t=0;
		}
	
		/* Fim cilíndros, vamos para o próximo disco (próxima trilha) */
		if(c.x >= numCylinders){
			c.x=0; c.z++; c.t=0;
		}
		
		
	}
	
	/* Percorremos o resto do disco e inserimos conforme regra de percorrimento */
	while(arquivoTamanho > 0){
		i = getIndex(&c);
			
		if(encontraCluster(&c) == SUCESSO){
			/* Gravamos no cluster */
			setor = gravarCluster(arquivo, &c, &arquivoTamanho, &primeiro, &anterior);
			/* Contamos o tempo */
			tempo += calculaTempo(setor);
		}
		else {
			print_leftVerbose("FALHA", COR_VERMELHO);
			printf("Problema ao percorrer o disco\n");
			exit(1);
		}
		
	
	}
	
	/* 
	  Caso já tenhamos inserido todo o arquivo no disco,
	  paramos e definimos o último setor como último setor 
	*/
	fatent[anterior]->next = NULO;
	fatent[anterior]->eof = ULTIMOSETOR;
	
	/* Atualizamos a tabela FAT */
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
	
	print_leftVerbose("INFOR", COR_VERDE);
	printf("%u/%u do disco ocupado\n", disco_setoresOcupados, espaco);
	print_leftVerbose("TEMPO", COR_VERDE);
	printf("Arquivo salvo em %.2lf ms\n", tempo);
	return;
}

void disco_lerArquivo(char *nomeArquivo){
	fatlist_s *tmp = fatlist;
	fatent_s *tmp_file;
	FILE *saida;
	double tempo = 0;
	coordenadas c;
	if(tmp != NULL){
		/* Buscamos por arquivos na tabela FAT */
		do {
			/* Encontramos o nome do arquivo na tabela FAT */
			if(strcmp(nomeArquivo, tmp->file_name) == 0) {
				/* Arquivo de saída */
				saida = fopen("SAIDA.TXT", "w");
				
				/* Pegamos o primeiro setor do arquivo */
				tmp_file = fatent[tmp->first_sector];
				
				/* Obtemos as coordenadas do setor */
				getCoord(tmp->first_sector, &c);	
			
				/* Percorremos os setores */
				while(1) {
				
					/* Gravamos o primeiro setor de acordo com o seu tamanho real */
					fwrite(cylinders[c.x].track[c.z].sector[c.t].bytes_s, calculaTamanhoSetor(&c), 1, saida);
					
					tempo += calculaTempo(getIndex(&c));	
					getCoord(tmp_file->next, &c);
					
					/* Paramos */
					if(tmp_file->eof == ULTIMOSETOR) break;
					
					tmp_file = fatent[tmp_file->next];
				}
				
				print_leftVerbose("INFOR", COR_VERDE);
				printf("Arquivo lido em %.2lf ms\n", tempo);
				fclose(saida);
				return;
			}
			tmp = tmp->next;
		} while(tmp != NULL);
	}
	print_leftVerbose("FALHA", COR_VERMELHO);
	printf("Arquivo inválido\n");
	return;
}

void disco_removerArquivo(char *nomeArquivo){
	fatlist_s *tmp = fatlist, *tmp_anterior = NULL;
	fatent_s *tmp_file;
	coordenadas c;
	unsigned int i;
	if(tmp != NULL){
		/* Buscamos por arquivos na tabela FAT */
		do {
			/* Encontramos o nome do arquivo na tabela FAT */
			if(strcmp(nomeArquivo, tmp->file_name) == 0) {
				/* Pegamos o primeiro setor do arquivo */
				tmp_file = fatent[tmp->first_sector];
							
				/* Liberamos os setores */
				while(1) {
					tmp_file->used = LIVRE;
					disco_setoresOcupados--;
					
					/* Paramos */
					if(tmp_file->eof == ULTIMOSETOR) break;
					
					tmp_file = fatent[tmp_file->next];
				}
				
				
				/* Removemos da tabela FAT */
				if(tmp_anterior != NULL) tmp_anterior->next = tmp->next;
				else fatlist = tmp->next;
				
				free(tmp);
				print_leftVerbose("INFOR", COR_VERDE);
				printf("Arquivo removido com sucesso!\n");
				
				return;
			}
			tmp_anterior = tmp;
			tmp = tmp->next;
		} while(tmp != NULL);
	}
	print_leftVerbose("FALHA", COR_VERMELHO);
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
		} while(tmp != NULL);
		
		printf("NOME:");
		for(int i=0;i<maiorNome-4;i++) printf(" ");
		printf("\tTAMANHO EM DISCO\tLOCALIZAÇÃO\n");
		
		tmp = fatlist;
		do {
			int j=0;
			tmp_file_setores = (unsigned int *)malloc(sizeof(unsigned int));
			tmp_file_setores[0] = tmp->first_sector;
			
			tmp_file = fatent[tmp->first_sector];
			
			while(tmp_file->eof != ULTIMOSETOR) {
				new = (unsigned int *)realloc(tmp_file_setores, (++j+1)*sizeof(unsigned int));
				if(new == NULL){
					print_leftVerbose("FALHA", COR_VERMELHO);
					printf("Problema ao realocar\n");
					exit(1);
				}
				else tmp_file_setores = new;
				tmp_file_setores[j] = tmp_file->next;
				tmp_file = fatent[tmp_file->next];
			}
			
			printf("%-10s\t%10u bytes\t", tmp->file_name, blockSize*(j+1));
			int k=0;
			while(k<=j) {
				printf("%u", tmp_file_setores[k]);
				if(k++!=j) printf(",");
			}
			printf("\n");
			tmp = tmp->next;
			free(tmp_file_setores);
			tmp_file_setores = NULL;
		} while(tmp != NULL);
	}
	else{
		print_leftVerbose("INFOR", COR_VERDE);
		printf("A tabela FAT está vazia\n");
	}
}
