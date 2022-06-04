#ifndef FUNCIONES_MEMORIA_H_
#define FUNCIONES_MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>

#define IP_MEMORIA "127.0.0.1"
#define PUERTO_MEMORIA 8002

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia de Memoria como servidor
void iterator(char* value);
int conexionConCpu(void);

#endif /* FUNCIONES_MEMORIA_H_*/