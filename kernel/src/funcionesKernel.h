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

t_list* listaProcesosASuspender;

t_log* logger;

typedef struct
{
	int idProceso;
	int tamanioProceso;
	t_list* instrucciones;
	int program_counter;
	int tabla_paginas;
	float estimacion_rafaga;
	float estimacion_anterior;
	clock_t rafagaMs;
	clock_t horaDeIngresoAExe;
	t_estado estado;
	bool suspendido;
	int socket_cliente;

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

typedef enum{
	NO_OP,
	I_O_, //despues le sacamos un guion
	WRITE,
	COPY ,
	READ
}nroDeInstruccion;
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


//Funciones para Kernel como cliente de Memoria

//Funciones que usan los hilos

void enviarMensajeAMemoria(char* mensaje); // falta desarrollar
void enviarProcesoAMemoria(t_pcb* proceso); // falta desarrollar
int obtenerValorDeTP();  // falta desarrollar
void enviarInterrupcionACpu(); // falta desarrollar
t_pcb * recibirProcesoAFinalizar(); // falta desarrollar
void enviarMensajeAConsola(char* mensaje); // falta desarrollar



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


bool supera_tiempo_maximo_bloqueado(t_pcb* proceso);
int obtenerTiempoDeBloqueo(t_pcb* proceso);
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
void asignar_memoria();

/*
  * FINALIZAR_PROCESO_Y_AVISAR_A_CPU(): Recibe un PCB con motivo de finalizar el mismo, pasa al proceso al estado EXIT y
	da aviso al módulo Memoria para que éste libere sus estructuras. La idea sería tener un semaforo o algo que
	controle que la ejecucion del proceso sea la última
 */

void finalizar_proceso_y_avisar_a_memoria();

/*
 * FINALIZAR_PROCESO_Y_AVISAR_A_CONSOLA(): Una vez liberadas las estructuras de CPU, se dará aviso a la Consola
	           de la finalización del proceso. La idea seria que espere el pcb de cpu con un mensaje que avise la liberación
	           de las estructuras
 */

void finalizar_proceso_y_avisar_a_consola();

//* PLANIFICAR(): Llama constantemente al planificador.

void planificar();

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

sem_t pcbEnNew;
sem_t pcbEnReady;
sem_t gradoDeMultiprogramacion;

pthread_mutex_t asignarMemoria;




//-------------VARIABLES Y FUNCIONES DE PRUEBA -------------------------

// int clientesDePrueba[4]={4,50,60,270};
int servidorPrueba;
t_list * lista3;
t_list * lista50;
t_list * lista60;
t_list * lista270;
instrucciones * instruccion1;
instrucciones * instruccion2;
instrucciones * instruccion3;
instrucciones * instruccion4;
instrucciones * instruccion5;
instrucciones * instruccion6;
instrucciones * instruccion7;
instrucciones * instruccion8;
instrucciones * instruccion9;
instrucciones * instruccion10;
instrucciones * instruccion11;
instrucciones * instruccion12;
instrucciones * instruccion13;
instrucciones * instruccion14;
instrucciones * instruccion15;
instrucciones * instruccion16;
instrucciones * instruccion17;
instrucciones * instruccion18;
instrucciones * instruccion19;
instrucciones * instruccion20;

/*
instrucciones instruccionesDePrueba[20] = {
{"EXIT",{-1,-1}},
{"NO_OP",{5,-1}},{"NO_OP",{1,-1}},{"NO_OP",{2,-1}},
{"I/O",{3000,-1}},{"I/O",{10000,-1}},{"I/O",{1000,-1}},{"I/O",{6000,-1}},
{"WRITE",{4,42}},{"WRITE",{2,70}},{"WRITE",{1,20}},{"WRITE",{5,10}},{"WRITE",{12,102}},
{"COPY",{3,4}},{"COPY",{5,2}},{"COPY",{8,1}},{"COPY",{5,15}},{"COPY",{30,12}},
{"READ",{3,-1}},{"READ",{5,-1}}
};
*/

void recibir_consola_prueba(int servidor);
int esperar_cliente_prueba(int i);
void atender_consola_prueba(int nuevo_cliente);
void inicializar_lista_de_prueba();
void inicializar_instrucciones_de_prueba();

#endif /* FUNCIONES_KERNEL_H_ */





