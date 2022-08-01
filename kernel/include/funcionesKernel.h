#ifndef FUNCIONES_KERNEL_H_
#define FUNCIONES_KERNEL_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/string.h>
#include<commons/config.h>
#include<string.h>
#include<assert.h>

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"


int socketServidor;
int socketCpuDispatch;
char* ipCpu;
int puertoCpuDispatch;
char* puertoCpuInterrupt;
char* puertoKernel;

int pidKernel;
char* algoritmoPlanificacion;
int estimacionInicial;
int alfa;
int gradoDeMultiprogramacion;
int gradoMultiprogramacionActual;
bool procesoEjecutando;
char* tiempoMaximoBloqueado;

int socketMemoria;
char* ipMemoria;
int puertoMemoria;

int tamanioTotalIdentificadores;
int contadorInstrucciones;
int desplazamiento;
int tamanioDelProceso;

t_queue* colaNew;
t_queue* colaReady;
t_queue* colaSuspendedReady;
t_queue* colaExe;
t_queue* colaBlocked;
t_queue* colaSuspendedBlocked;
t_queue* colaExit;

t_log* logger;

typedef struct
{
	int idProceso;
	int tamanioProceso;
	t_list* instrucciones;
	int program_counter;
	int tabla_paginas; // Esto se lo pasa memoria
	float estimacion_rafaga;
	int socket_cliente;
} t_pcb;

typedef struct
{
	char* identificador;
	int parametros[2];
} instruccion;


typedef enum
{	MENSAJE,
	RECIBIR_INSTRUCCIONES,
	RECIBIR_TAMANIO_DEL_PROCESO
}op_code_consola;

typedef enum
{
	MENSAJE_A_CPU,
	ENVIAR_PCB
}op_code_cpu;

typedef enum
{
	MENSAJE_A_MEMORIA,
	NRO_TP1
}op_code_memoria;

t_pcb* pcb;

//Funciones como cliente de Memoria
int conexionConMemoria();
void enviarPID();
void inicializarConfiguraciones(char*);
t_list* recibir_paquete_int(int);

//-------------- Funciones para Kernel como servidor de consola ---------

void* recibir_buffer(int*, int);
int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
t_list* recibir_paqueteInt(int);
t_pcb* tomar_pcb(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia del Kernel como servidor
void iterator(int);
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
	op_code_memoria codigo_operacion_memoria;
	op_code_cpu codigo_operacion_cpu;
	
	t_buffer* buffer;
} t_paquete;


t_paquete* paquete;

int crear_conexion(char* ip, int puertoCpuDispatch);
void enviar_mensaje(char* mensaje, int socket_cliente);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(int);
void eliminar_paquete_mensaje(t_paquete* paqueteMensaje);

void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_a_paquete_kernel_cpu(t_pcb*);
void obtenerTamanioIdentificadores(instruccion*);
void agregar_instrucciones_al_paquete(instruccion*);


//Funciones propias del Kernel como cliente
t_log* iniciar_logger(void);
t_config* iniciar_config(void);

//void paquete(int,char*);// aca iria en vez de un char la estructura pcb
//Revisar esta funcion

void terminar_programa(int, t_log*, t_config*);
int conexionConCpu(void);

void cargar_pcb(int);
void crear_colas();
void generar_conexiones();// Todas las funciones que agreguen, hagan el prototipo sino rompe los huevos los warning cuando compila

//Funciones propias del Kernel como cliente

#endif /* FUNCIONES_KERNEL_H_ */
