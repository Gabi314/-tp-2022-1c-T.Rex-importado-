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
#include<stdbool.h>
#include<semaphore.h>
#include<time.h>
#include<pthread.h>

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"



int pidKernel;
int socketServidor;
int socketCpuDispatch;
int socketCpuInterrupt;
char* ipCpu;
char* puertoCpuDispatch;
char* puertoCpuInterrupt;
char* puertoKernel;
char* algoritmoPlanificacion;
float estimacionInicial;
int alfa;
int gradoMultiprogramacionTotal;
int gradoMultiprogramacionActual;
bool procesoEjecutando;
int tiempoMaximoBloqueado;
int socketMemoria;
char* ipMemoria;
char* puertoMemoria;
int tamanioTotalIdentificadores;
int contadorInstrucciones;
int desplazamiento;
char* tamanioDelProceso;

typedef enum estado { NEW, READY, BLOCKED, EXEC, SUSP_READY, SUSP_BLOCKED, TERMINATED } t_estado;


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
	int tabla_paginas; // el tipo ???
	float estimacion_rafaga;
	float estimacion_anterior;
	clock_t rafagaAnterior;
	clock_t horaDeIngresoAExe;
	clock_t horaDeIngresoAReady;
	int tiempoEspera;
	t_estado estado;
	int socket_cliente;
	int socketMemoria;
	float tiempoEjecucionRealInicial;
	float tiempoEjecucionAcumulado;

} t_pcb;

typedef enum
{
	I_O,
	EXIT,
	INTERRUPT
}op_code_cpu;

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

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
t_pcb* tomar_pcb(int);
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


t_paquete* paquete;

int crear_conexion(char* ip, int puertoCpuDispatch);
void enviar_mensaje(char* mensaje, int socket_cliente);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(void);
t_paquete* crear_super_paquete(void);
//void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void eliminar_paquete_mensaje(t_paquete* paqueteMensaje);
void obtenerTamanioIdentificadores(instrucciones* instruccion);
void agregar_instrucciones_a_paquete(instrucciones* instruccion);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void eliminar_paquete_mensaje(t_paquete* paqueteMensaje);

//Funciones propias del Kernel como cliente
t_log* iniciar_logger(void);
t_config* iniciar_config(void);
t_config* config;
//void paquete(int,char*);// aca iria en vez de un char la estructura pcb
//Revisar esta funcion
void terminar_programa(int, t_log*, t_config*);
void conexionConCpu(void);
//Funciones propias del Kernel como cliente

//-------------PLANIFICADOR---------------



typedef enum{
	SRT,
	FIFO
}t_algoritmo_planificacion;


t_algoritmo_planificacion algoritmoPlanificacionActual;

t_pcb* obtenerSiguienteFIFO();
t_pcb* obtenerSiguienteSRT();

//--------------------TRANSICIONES---------------

void agregarANew(t_pcb* proceso);
t_pcb* sacarDeNew();
void agregarAReady(t_pcb* proceso);
void agregarABlocked(t_pcb* proceso);
void sacarDeBlocked(t_pcb* proceso);
void agregarASuspendedBlocked(t_pcb* proceso);
void sacarDeSuspendedBlocked(t_pcb* proceso);
void agregarAReadySuspended(t_pcb* proceso);
t_pcb* sacarDeReadySuspended();

//------------------HILOS--------------------
//MULTIHILOS DE EJECUCION PARA ATENDER N CONSOLAS: esperan a recibir una consola y sus
 //          instrucciones para generar el pcb y asignar el proceso a NEW
void recibir_consola(int servidor) ;
void atender_consola(int cliente);

/*
ASIGNAR_MEMORIA(): si el grado de multiprogramacion lo permite, pasa el primer proceso de colaNew a READY, envia
un mensaje al módulo Memoria para que inicialice sus estructuras necesarias y obtener el valor de la tabla de páginas
del PCB. Además, si el algoritmo es SRT envía una interrupcion a cpu.
 */
void asignar_memoria(t_pcb proceso);



#endif /* FUNCIONES_KERNEL_H_ */





