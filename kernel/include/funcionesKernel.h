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
#include<semaphore.h>
#include<pthread.h>
#include<stdbool.h>

#define IP_KERNEL "127.0.0.1"
#define PUERTO_KERNEL "8000"


int socketServidor;
int socketCpuDispatch;
int socketCpuInterrupt;
char* ipCpu;
int puertoCpuDispatch;
int puertoCpuInterrupt;
char* puertoKernel;

int pidKernel;
char* algoritmoPlanificacion;
int estimacionInicial;
int alfa;
int gradoMultiprogramacionTotal;

bool procesoEjecutando;
int tiempoMaximoBloqueado;

int socketMemoria;
char* ipMemoria;
int puertoMemoria;

int tamanioTotalIdentificadores;
int contadorInstrucciones;
int desplazamiento;
int tamanioDelProceso;
bool ejecucionActiva;
int min (int x, int y);
int tiempoBloqueo;

t_queue* colaNew;
t_queue* colaReady;
t_queue* colaSuspendedReady;
t_queue* colaExe;
t_queue* colaBlocked;
t_queue* colaSuspendedBlocked;
t_queue* colaExit;

t_log* logger;
t_list* listaInstrucciones;

typedef enum estado { NEW, READY, BLOCKED, EXEC, SUSP_READY, SUSP_BLOCKED, TERMINATED } t_estado;

typedef struct
{
	int idProceso;
	int tamanioProceso;
	t_list* instrucciones;
	int program_counter;
	int tabla_paginas; // Esto se lo pasa memoria
	float estimacion_rafaga;
	clock_t rafagaMs; //pasar a int
	clock_t horaDeIngresoAExe;
	t_estado estado;
} t_pcb;

typedef struct
{
	char* identificador;
	int parametros[2];
} instruccion;

typedef struct
{
	int socket;
	int pid;
} t_pidXsocket;


typedef enum
{	MENSAJE,
	RECIBIR_INSTRUCCIONES,
	RECIBIR_TAMANIO_DEL_PROCESO
}op_code_consola;

typedef enum
{
	MENSAJE_A_CPU,
	ENVIAR_PCB,
	I_O,
	EXIT,
	INTERRUPT,
	MENSAJE_INTERRUPT
}op_code_cpu;

typedef enum
{
	MENSAJE_A_MEMORIA,
	NRO_TP1,
	MENSAJE_LIBERAR_POR_TERMINADO,
	MENSAJE_LIBERAR_POR_SUSPENDIDO,
	MENSAJE_CONFIRMACION_SUSPENDIDO
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
int conexionConConsola(int);
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

int crear_conexion(char*,int);
void enviar_mensaje(char*, int ,int);
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
int conexionConCpu(t_pcb*);

void cargar_pcb(int);
void crear_colas();
void generar_conexiones();

//-------------PLANIFICADOR---------------



typedef enum{
	SRT,
	FIFO
}t_algoritmo_planificacion;


t_algoritmo_planificacion algoritmoPlanificacionActual;
t_pcb* procesoEnEjecucion;
t_pcb* obtenerSiguienteFIFO();
t_pcb* obtenerSiguienteSRT();

//--------------------TRANSICIONES---------------

void inicializar_colas();

void agregarANew(t_pcb* proceso);
t_pcb* sacarDeNew();
void agregarAReady(t_pcb* proceso);
void agregarABlocked(t_pcb* proceso);
void sacarDeBlocked();
void agregarASuspendedBlocked(t_pcb* proceso);
void sacarDeSuspendedBlocked(t_pcb* proceso);
void agregarASuspendedReady(t_pcb* proceso);
t_pcb* sacarDeReadySuspended();
t_pcb* procesoAEjecutar;
void estimarRafaga();


bool supera_tiempo_maximo_bloqueado(t_pcb* proceso);
int obtenerTiempoDeBloqueo(t_pcb* proceso);
t_pcb* obtenerSiguienteDeReady();

//------------------HILOS--------------------
//MULTIHILOS DE EJECUCION PARA ATENDER N CONSOLAS: esperan a recibir una consola y sus
 //          instrucciones para generar el pcb y asignar el proceso a NEW
void recibir_consola(int servidor) ;
void atender_consola(int cliente);

t_list * listaDeConsolas;

/*
ASIGNAR_MEMORIA(): si el grado de multiprogramacion lo permite, pasa el primer proceso de colaNew a READY, envia
un mensaje al módulo Memoria para que inicialice sus estructuras necesarias y obtener el valor de la tabla de páginas
del PCB. Además, si el algoritmo es SRT envía una interrupcion a cpu.
 */
void asignar_memoria();

/*
  * FINALIZAR_PROCESO_Y_AVISAR_A_CPU(): Recibe un PCB con motivo de finalizar el mismo, pasa al proceso al estado EXIT y
	da aviso al módulo Memoria para que éste libere sus estructuras. La idea sería tener un semaforo o algo que
	controle que la ejecucion del proceso sea la última
 */
void atender_interrupcion_de_ejecucion();

int recibir_tiempo();

t_pcb * procesoDesalojado;
t_pcb * procesoAFinalizar;
t_pcb * procesoABloquear;

void readyAExe();

void terminarEjecucion();

void atenderDesalojo();

void atenderIO();

void ejecutar(t_pcb*);

/*
 * FINALIZAR_PROCESO_Y_AVISAR_A_CONSOLA(): Una vez liberadas las estructuras de CPU, se dará aviso a la Consola
	           de la finalización del proceso. La idea seria que espere el pcb de cpu con un mensaje que avise la liberación
	           de las estructuras
 */

void finalizar_proceso_y_avisar_a_consola();



/*
 * SUSPENDER(): si un proceso está bloqueado por un tiempo mayor al límite se llamará a una transición para
	           suspenderlo y se enviará un mensaje a memoria con la informacion necesaria.
 */

void suspender();

/*
 * DESBLOQUEAR_SUSPENDIDO(): espera a que termine la entrada/salida de un proceso SUSPENDED-BLOCKED y llama
	a las transiciones necesarias para que pase a ser SUSPENDED-READY
 */

void desbloquear_suspendido();


//----------------------------SEMAFOROS--------------------------
void inicializar_semaforos();

sem_t kernelSinFinalizar;

sem_t pcbEnNew;
sem_t pcbEnReady;
sem_t gradoDeMultiprogramacion;
sem_t cpuDisponible;
sem_t desalojarProceso;
sem_t procesoEjecutandose;
sem_t procesoDesalojadoSem;
sem_t pcbInterrupt;
sem_t pcbExit;
sem_t pcbBlocked;
sem_t suspAReady;

pthread_mutex_t asignarMemoria;
pthread_mutex_t obtenerProceso;
pthread_mutex_t ejecucion;
pthread_mutex_t procesoExit;
pthread_mutex_t consolasExit;
pthread_mutex_t desalojandoProceso;
pthread_mutex_t consolaNueva;
pthread_mutex_t encolandoPcb;
pthread_mutex_t mutexExit;
pthread_mutex_t mutexInterrupt;
pthread_mutex_t mutexIO;
pthread_mutex_t bloqueandoProceso;

//Funciones propias del Kernel como cliente

#endif /* FUNCIONES_KERNEL_H_ */
