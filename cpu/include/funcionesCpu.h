#ifndef FUNCIONES_CPU_H_
#define FUNCIONES_CPU_H_

#include "funcionesCpu.h"
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
#include<math.h>
#include <time.h>

#define IP_CPU "127.0.0.1"
#define PUERTO_CPU_DISPATCH 8001 // por ahora este faltan los otros puertos para conectar a kernel
#define PUERTO_CPU_INTERRUPT 8005

typedef enum
{
	MENSAJE_CPU_MEMORIA,
	TAM_PAGINAS_Y_CANT_ENTRADAS,
	PRIMER_ACCESO,
	SEGUNDO_ACCESO,
	READ,
	WRITE,
	COPY
}op_code_memoria;

typedef enum
{
	MENSAJE_A_KERNEL,
	RECIBIR_PCB,
	I_O,
	EXIT,
	INTERRUPT,
	MENSAJE_INTERRUPT
}op_code_kernel;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code_memoria codigo_operacion_memoria;
	op_code_kernel codigo_operacion_kernel;
	t_buffer* buffer;
} t_paquete;

t_log* logger;
t_config* config;
t_paquete* paquete;

//--------------  Cpu como servidor de Kernel ---------
int clienteKernel;
int server_fd;

int tamanioTotalIdentificadores;
int contadorInstrucciones;
int desplazamiento;
char* tamanioDelProceso;
//Estructuras

typedef enum {
    NO_OP1,
    IO1,
    WRITE1,
    COPY1,
    READ1,
    EXIT1
} op_code;


typedef enum estado { NEW, READY, BLOCKED, EXEC, SUSP_READY, SUSP_BLOCKED, TERMINATED } t_estado;


typedef struct
{
	uint32_t idProceso;
	uint32_t tamanioProceso;
	t_list* instrucciones;
	t_estado estado;
	uint32_t programCounter;
	uint32_t nroTabla1erNivel;
	uint32_t estimacionRafaga;
	uint32_t rafagaMs; //pasar a int
	uint32_t horaDeIngresoAExe;

	//int socket_cliente;
} t_pcb;

typedef struct
{
	char* identificador;
	uint32_t parametros[2];
} instruccion;


void* recibir_buffer(int*, int);

int iniciar_servidor(int);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

//Funcion propia de cpu como servidor
void iterator(instruccion*);
int conexionConKernel(void);
void obtenerTamanioIdentificadores(instruccion*);
t_pcb* recibir_pcb();
void agregarInstruccionesAlPaquete(instruccion*);
t_paquete* agregar_a_paquete_kernel_cpu(t_pcb*,int);
//Funcion propia de cpu como servidor

//--------------  Cpu como cliente de Memoria -------------
int conexionMemoria;
//Variables globales de config
int cantidadEntradasTlb;
char* algoritmoReemplazoTlb;
int retardoDeNOOP;
char* ipMemoria;
int puertoMemoria;
int puertoDeEscuchaDispath;
int puertoDeEscuchaInterrupt;

//Variables direccion logica
int numeroDePagina;
int entradaTabla1erNivel;
int entradaTabla2doNivel;
int desplazamiento;

t_list* tamanioDePagYEntradas;
int tamanioDePagina;
int entradasPorTabla;

//Estructuras
t_list* tlb;

typedef struct
{
	int nroDePagina;
	int nroDeMarco;
	time_t instanteGuardada;
	time_t ultimaReferencia;
} entradaTLB;

int crear_conexion(char* ip, int puertoCpuDispatch);
t_paquete* crear_paquete(int cod_op);

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente,int cod_op);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//Funciones propias de Cpu como cliente
t_log* iniciar_logger(void);
t_config* iniciar_config(void);

void enviarEntradaTabla1erNivel();
void enviarEntradaTabla2doNivel();
void enviarDireccionFisica(int,int,int);
void enviarValorAEscribir(uint32_t);
void enviarDireccionesFisicasParaCopia(int,int,int,int);

void enviarPcb(t_pcb*,int);

void terminar_programa(int, t_log*, t_config*);

int conexionConMemoria(void);
void inicializarConfiguraciones(char*);

t_list* inicializarTLB();
void generarListaCamposTLB(t_list*);
void reiniciarTLB();
int chequearMarcoEnTLB(int);
void agregarEntradaATLB(int,int);
void algoritmosDeReemplazoTLB(int,int);
void reemplazarPagina(int,int,int);

void calculosDireccionLogica(int);
void leerTamanioDePaginaYCantidadDeEntradas(t_list*);
int buscarDireccionFisica(int);
int accederAMemoria(int);

//Ciclo de instruccion
instruccion* buscarInstruccionAEjecutar(t_pcb*);
void decode(instruccion*);
void ejecutar(instruccion*);


bool send_PCB_mas_int (int, t_pcb*, uint32_t);
op_code generarCode(char *);

#endif /*FUNCIONES_CPU_H_*/
