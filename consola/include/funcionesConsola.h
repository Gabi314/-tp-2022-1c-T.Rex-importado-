#ifndef FUNCIONES_CONSOLA_H_
#define FUNCIONES_CONSOLA_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<readline/readline.h>

#include <sys/stat.h>
#include <fcntl.h>

typedef enum
{
	MENSAJE,
	INSTRUCCIONES,
	TAMANIO_PROCESO
}op_code;

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

typedef struct
{
	char* identificador;
	int parametros[2];
} instrucciones;

// Esto para levantar la conexion de la consola----------------------
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(int);
void agregar_a_paquete(t_paquete* paquete, instrucciones* instruccion, int identificador_length);
void agregar_a_paquete_tamanioProceso(t_paquete*,void*,int);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

t_log* logger;
t_config* config;

int conexion;
char* ip;
char* puerto;

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void leer_consola(t_log*);
void enviarTamanioDelProceso(int);
void dividirInstruccionesAlPaquete(t_log* logger,t_paquete* paquete,char** lineasDeInstrucciones,instrucciones* instruccion);
void terminar_programa(int, t_log*, t_config*);

#endif /*FUNCIONES_CONSOLA_H_*/
