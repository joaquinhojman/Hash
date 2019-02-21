//TIPO DE DATO ABSTRACTO TABLA DE HASH (Joaquin Hojman)

#define _POSIX_C_SOURCE 200809L
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TAMANIO_INICIAL 50
#define VACIO 0
#define OCUPADO 1
#define BORRADO 2
#define FACTOR_REDIMENSION_SUPERIOR 0.7
#define FACTOR_REDIMENSION_INFERIOR 0.5

//HASH ELEGIDO: CERRADO

typedef struct hash_campo{
	char *clave;
	void *valor;
	size_t estado;
}hash_campo_t;

struct hash{
	size_t cantidad;
	size_t largo;
	hash_campo_t *tabla;
	hash_destruir_dato_t destruir_dato;
};

struct hash_iter{
	const hash_t* hash;
	size_t pos_actual;
	size_t recorrido;
};

bool hash_redimensionar(hash_t *hash, size_t redimension);

//Funcion de hashing elegida
size_t hash_clave(size_t largo, const char *key){
	size_t hash, i;
	for(hash = i = 0; i < strlen(key); ++i){
		size_t aux = (size_t)(key[i]);
		hash += aux;
		hash += (hash << 10);
	    hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	hash %= largo;
	return hash;
}

size_t hash_obtener_posicion(const hash_campo_t *tabla, const char *clave,size_t pos ,size_t largo){
	while (true){
		if (tabla[pos].estado==VACIO) break;
		if (tabla[pos].estado==OCUPADO&&strcmp(tabla[pos].clave,clave)==0) break;
		pos=(pos+1)%largo;
	}
	return pos;
}

hash_t *hash_crear(hash_destruir_dato_t destruir_dato){
	hash_t* hash=malloc(sizeof(hash_t));
	if (!hash) return NULL;
	
	hash->cantidad=0;
	hash->largo=TAMANIO_INICIAL;
	hash->destruir_dato=destruir_dato;
	
	hash_campo_t* tabla=calloc(3,TAMANIO_INICIAL*sizeof(hash_campo_t));
	if (!tabla){
		free(hash);
		return NULL;
	}
	hash->tabla=tabla;
	
	return hash;
}

bool hash_guardar(hash_t *hash, const char *clave, void *dato){
	if (clave==NULL) return false;
	if (hash->cantidad/hash->largo>FACTOR_REDIMENSION_SUPERIOR){
		if (!hash_redimensionar(hash,hash->largo*2)) return false;
	}
	size_t pos=hash_clave(hash->largo,clave);
	pos=hash_obtener_posicion(hash->tabla,clave,pos,hash->largo);
	
	while (hash->tabla[pos].estado!=VACIO){		
		if (strcmp(hash->tabla[pos].clave,clave)==0){
			if (hash->destruir_dato!=NULL){
				if (hash->tabla[pos].valor!=NULL) hash->destruir_dato(hash->tabla[pos].valor);
			}
		hash->tabla[pos].valor=dato;
		return true;
		}
		pos++;
	}
	
	char* clave_aux=strdup(clave);
	hash->tabla[pos].clave=clave_aux;
	hash->cantidad++;
	hash->tabla[pos].valor=dato;
	hash->tabla[pos].estado = OCUPADO;
	return true;
}

void *hash_borrar(hash_t *hash, const char *clave){
	if(!hash_pertenece(hash,clave)) return NULL;
	if (hash->cantidad/hash->largo>FACTOR_REDIMENSION_INFERIOR){
		if(!hash_redimensionar(hash,hash->largo/2)) return false;
	}
	size_t pos=hash_clave(hash->largo,clave);	
	pos=hash_obtener_posicion(hash->tabla,clave,pos,hash->largo);
	
	if (hash->tabla[pos].estado!=OCUPADO) return NULL;	
		
	free(hash->tabla[pos].clave);
	hash->cantidad--;
	hash->tabla[pos].estado=BORRADO;
	return hash->tabla[pos].valor;
}

void *hash_obtener(const hash_t *hash, const char *clave){
	if (!hash_pertenece(hash,clave)) return NULL;
	size_t pos=hash_clave(hash->largo,clave);
	pos=hash_obtener_posicion(hash->tabla,clave,pos,hash->largo);
	return hash->tabla[pos].valor;
}

bool hash_pertenece(const hash_t *hash, const char *clave){
	size_t pos=hash_clave(hash->largo,clave);
	pos=hash_obtener_posicion(hash->tabla,clave,pos,hash->largo);
	if (hash->tabla[pos].estado==OCUPADO) return true;
	return false;
}

size_t hash_cantidad(const hash_t *hash){
	return hash->cantidad;
}

void hash_destruir(hash_t *hash){
	for (int i=0;i<hash->largo;i++){
		if (hash->tabla[i].estado==OCUPADO){
			if (hash->destruir_dato!=NULL){
				if (hash->tabla[i].valor!=NULL) 
					hash->destruir_dato(hash->tabla[i].valor);
			}
			free(hash->tabla[i].clave);
		}
	}
	
	free(hash->tabla);
	free(hash);
}                 
             
hash_iter_t *hash_iter_crear(const hash_t *hash){
	hash_iter_t* iter = malloc(sizeof(hash_iter_t));
	if (!iter) return NULL;	
	
	iter->hash = hash;
	iter->pos_actual=0;
	iter->recorrido = hash->cantidad;
	
	while (iter->pos_actual<iter->hash->largo){
		if (iter->hash->tabla[iter->pos_actual].estado!=OCUPADO)
			iter->pos_actual++;
		else break;
	}	
	
	return iter;
}

bool hash_iter_al_final(const hash_iter_t *iter){
	if (iter->recorrido!=0) return false;
	return true;
}

bool hash_iter_avanzar(hash_iter_t *iter){
	if(hash_iter_al_final(iter)) return false;
	
	iter->pos_actual++;	
	while (iter->pos_actual<iter->hash->largo){
		if (iter->hash->tabla[iter->pos_actual].estado!=OCUPADO) iter->pos_actual++;
		else break;
	}
	
	iter->recorrido--;
	return true;
}

const char *hash_iter_ver_actual(const hash_iter_t *iter){
	if (hash_iter_al_final(iter)) return NULL;
	return iter->hash->tabla[iter->pos_actual].clave;
}

void hash_iter_destruir(hash_iter_t* iter){
	free(iter);	
}                    

bool hash_redimensionar(hash_t* hash,size_t tamanio_nuevo){
	hash_campo_t* tabla=calloc(3,tamanio_nuevo*sizeof(hash_campo_t));
	if(!tabla) return false;	
	
	size_t largo_viejo=hash->largo;
	hash_campo_t* tabla_vieja=hash->tabla;
	hash->tabla=tabla;
	hash->largo=tamanio_nuevo;
	
	for(int i=0;i<largo_viejo;i++){	
		if (tabla_vieja[i].estado==OCUPADO){
			size_t pos=hash_clave(tamanio_nuevo,tabla_vieja[i].clave);
			pos=hash_obtener_posicion(hash->tabla,tabla_vieja[i].clave,pos,tamanio_nuevo);
			hash->tabla[pos].clave=tabla_vieja[i].clave;	
			hash->tabla[pos].valor=tabla_vieja[i].valor;
			hash->tabla[pos].estado=OCUPADO;
		}	
	}
	
	free(tabla_vieja);
	return true;
}
