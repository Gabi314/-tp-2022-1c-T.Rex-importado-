#ifndef FUNCIONES_CPU_H_
#define FUNCIONES_CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/string.h>
#include<commons/config.h>
#include<string.h>
#include<assert.h>

#define IP_CPU "127.0.0.1"
#define PUERTO_CPU_DISPATCH 8001 // por ahora este faltan los otros puertos para conectar a kernel

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

t_log* logger;

//-------------- Funciones para Cpu como servidor de Kernel ---------
void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia de cpu como servidor
void iterator(char* value);
int conexionConKernel(void);
//Funcion propia de cpu como servidor

//-------------- Funciones para Cpu como cliente de Memoria -------------
typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

int crear_conexion(char* ip, int puertoCpuDispatch);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//Funciones propias de Cpu como cliente
t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void paquete(int,char*);// aca iria en vez de un char la estructura pcb
void terminar_programa(int, t_log*, t_config*);

void conexionConMemoria(void);
//Funciones propias de Cpu como cliente


#endif /*FUNCIONES_CPU_H_*/
