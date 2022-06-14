#ifndef FUNCIONES_KERNEL_H_
#define FUNCIONES_KERNEL_H_

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

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

t_log* logger;

//-------------- Funciones para Kernel como servidor de consola ---------
typedef struct
{
	char* identificador;
	int parametros[2];
} instrucciones;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia del Kernel como servidor
void iterator(char* value);
int conexionConConsola(void);
//Funcion propia del Kernel como servidor

//-------------- Funciones para Kernel como cliente de cpu -------------
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

//Funciones propias del Kernel como cliente
t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void paquete(int,char*);// aca iria en vez de un char la estructura pcb
void terminar_programa(int, t_log*, t_config*);

void conexionConCpu(void);
//Funciones propias del Kernel como cliente

#endif /* FUNCIONES_KERNEL_H_ */
